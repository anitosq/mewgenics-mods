"""Temporary native probe session; back up saves and restore loader config.

Passive by default; --layout-test enables experimental presentation changes.
Never writes item containers directly. Run with the game closed. Vortex deployment must
not run during the session. The original loader bytes are restored after exit,
but only if they still match this tool's temporary configuration.
"""
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def game_running():
    processes = subprocess.check_output(['tasklist', '/FI', 'IMAGENAME eq Mewgenics.exe', '/FO', 'CSV'], text=True)
    return '"Mewgenics.exe"' in processes


def run(game, profile, layout_test=False, control=False, test_slot=False):
    if game_running():
        raise RuntimeError('Close Mewgenics normally before starting an observational session')
    root = Path(__file__).resolve().parents[1]
    dll = root / 'work/native-build/ImprovedInventoryProbe.dll'
    if not control and not dll.is_file():
        raise RuntimeError('Build the diagnostic DLL first')
    report = json.loads((root / 'work/inventory-probe/probe-report.json').read_text())
    if sha(game / 'Mewgenics.exe') != report['binary']['sha256']:
        raise RuntimeError('Game executable changed since baseline inspection')
    backup = root / 'work/backups' / ('inventory-observation-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True)
    shutil.copytree(profile / 'saves', backup / 'saves')
    shutil.copy2(profile / 'settings.txt', backup / 'settings.txt')
    before = {p.name: sha(p) for p in (profile / 'saves').iterdir() if p.is_file()}
    config = game / 'chainloader.ini'
    original = config.read_bytes()
    (backup / 'chainloader.ini').write_bytes(original)
    lines = original.decode('utf-8-sig').splitlines(keepends=True)
    indices = [i for i, line in enumerate(lines) if line.strip() == '[LoadOrder]']
    if len(indices) != 1 or any('ImprovedInventoryProbe' in line for line in lines):
        raise RuntimeError('Unexpected loader configuration; no changes made')
    # Absolute DLL path leaves Vortex's deployed mod collection untouched.
    section = indices[0] + 1
    end = next((i for i in range(section, len(lines)) if lines[i].lstrip().startswith('[')), len(lines))
    import re
    numbers = [int(m.group(1)) for line in lines[section:end] if (m := re.match(r'Mod(\d+)\s*=', line))]
    newline = '\r\n' if b'\r\n' in original else '\n'
    lines.insert(end, f'{newline}Mod{max(numbers, default=0)+1}={dll}{newline}')
    temporary = ''.join(lines).encode('utf-8')
    if control:
        temporary = original
    (backup / 'temporary-chainloader.ini').write_bytes(temporary)
    print(f'Backups: {backup}', flush=True)
    mod_names = (game / 'mods/modlist.txt').read_text(encoding='utf-8-sig').splitlines()
    mods = [str(game / 'mods' / name.strip()) for name in mod_names if name.strip() and not name.startswith('#')]
    if layout_test:
        assets = root / 'work/native-assets'
        if not (assets / 'swfs/improved_inventory.swf').is_file() or not (assets / 'swfs/swflist.gon.append').is_file():
            raise RuntimeError('Build the native filter assets first')
        mods.append(str(assets))
    env = os.environ.copy()
    marker = dll.with_name('ImprovedInventoryProbe.session')
    if marker.exists():
        raise RuntimeError('A diagnostic session marker already exists; inspect before proceeding')
    test_save = profile / 'saves/steamcampaign03.sav'
    if test_slot:
        if test_save.exists():
            raise RuntimeError('Test slot 3 already exists; it will not be overwritten')
        shutil.copy2(profile / 'saves/steamcampaign01.sav', test_save)
        print('Temporary COPY of campaign 1 is in slot 3. Select the RIGHTMOST Home.', flush=True)
    if not control:
        marker.write_bytes(b'layout-test' if layout_test else b'observe-only')
    try:
        config.write_bytes(temporary)
        steam = game.parents[2] / 'steam.exe'
        if not steam.is_file():
            raise RuntimeError('Expected Steam executable not found')
        process = subprocess.Popen([str(steam), '-applaunch', '686060', '-modpaths', *mods], cwd=game, env=env)
        print(f'Steam launch request PID: {process.pid}', flush=True)
        print('Waiting up to 120 seconds for Steam to start the game.', flush=True)
        deadline = time.monotonic() + 120
        while not game_running() and time.monotonic() < deadline:
            time.sleep(0.5)
        while game_running():
            time.sleep(2)
    finally:
        if not control:
            marker.unlink(missing_ok=True)
        if config.read_bytes() == temporary:
            config.write_bytes(original)
            print('Original loader configuration restored byte-for-byte.', flush=True)
        else:
            print(f'Loader config changed externally; restore was withheld. Backup: {backup}', flush=True)
        after = {p.name: sha(p) for p in (profile / 'saves').iterdir() if p.is_file()}
        comparison = {'before': before, 'after': after,
                      'changed': sorted(k for k in before.keys() | after.keys() if before.get(k) != after.get(k))}
        (backup / 'save-comparison.json').write_text(json.dumps(comparison, indent=2))
        log = game / 'mod_logs/chainloader.log'
        if log.exists():
            shutil.copy2(log, backup / 'chainloader.log')
        print('Save-file changes during ordinary game session: ' + ', '.join(comparison['changed']), flush=True)
        if test_slot and not game_running() and test_save.is_file():
            shutil.copy2(test_save, backup / 'test-campaign-after.sav')
            test_save.unlink()
            print('Temporary slot 3 archived in the session backup and removed.', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', required=True, type=Path)
    parser.add_argument('--profile', required=True, type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--layout-test', action='store_true', help='Enable experimental presentation changes; no direct item writes')
    mode.add_argument('--control', action='store_true', help='Launch the existing mod configuration without our DLL')
    parser.add_argument('--test-slot', action='store_true', help='Copy campaign 1 to absent slot 3; archive/remove it after exit')
    args = parser.parse_args()
    run(args.game.resolve(), args.profile.resolve(), args.layout_test, args.control, args.test_slot)
