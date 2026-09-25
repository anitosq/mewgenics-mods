"""Build and verify one candidate archive from a clean commit; never deploy/publish."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / 'mods/improved-inventory'


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(data):
    return (json.dumps(data, indent=2, sort_keys=True) + '\n').encode()


def payload(version, commit, game_hash):
    compatibility = json.loads((MOD / 'compatibility.json').read_text())
    files = {
        'ImprovedInventory.dll': (ROOT / 'work/release-build/ImprovedInventory.dll').read_bytes(),
        'ImprovedInventory/swfs/improved_inventory.swf': (ROOT / 'work/native-assets/swfs/improved_inventory.swf').read_bytes(),
        'ImprovedInventory/swfs/swflist.gon.append': (ROOT / 'work/native-assets/swfs/swflist.gon.append').read_bytes(),
        'ImprovedInventory/INSTALL.md': (MOD / 'INSTALL.md').read_bytes(),
        'ImprovedInventory/LICENSE': (ROOT / 'LICENSE').read_bytes(),
        'ImprovedInventory/THIRD_PARTY_NOTICES.md': (ROOT / 'THIRD_PARTY_NOTICES.md').read_bytes(),
        'ImprovedInventory/CHANGELOG.md': (MOD / 'CHANGELOG.md').read_bytes(),
        'ImprovedInventory/description.json': json_bytes({
            'title': 'Improved Inventory', 'author': 'anitosq', 'version': version,
            'description': 'Scrolling Storage/Trash, search and filters. Requires Mewjector API v3; see INSTALL.md.',
            'url': 'https://github.com/anitosq/mewgenics-mods',
        }),
        'ImprovedInventory/build-info.json': json_bytes({
            'mod': 'improved-inventory', 'version': version, 'source_commit': commit,
            'game_sha256': game_hash, 'steam_build_id': compatibility['steam_build_id'],
            'compiler': 'zig 0.15.2', 'loader_api_minimum': compatibility['loader_api_minimum'],
            'status': 'candidate; see release checklist for runtime validation',
        }),
    }
    return files


def write_archive(path, files):
    # Explicit allowlist above; never zip a workspace or a deployed mod folder.
    with zipfile.ZipFile(path, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() or set(archive.namelist()) != set(files):
            raise RuntimeError('Archive integrity or file list mismatch')
        for name, data in files.items():
            if archive.read(name) != data:
                raise RuntimeError(f'Archive content mismatch: {name}')


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--game', required=True, type=Path)
    cli.add_argument('--output', type=Path, help='New output directory (default: outputs/releases/improved-inventory/VERSION)')
    args = cli.parse_args()
    if git('status', '--porcelain'):
        raise SystemExit('Commit project changes before packaging; working tree must be clean.')
    version = (MOD / 'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-z0-9.]+)?', version):
        raise SystemExit('Invalid version')
    commit = git('rev-parse', 'HEAD')
    output = (args.output or ROOT / 'outputs/releases/improved-inventory' / version).resolve()
    # Keep private builds under the project's ignored output/scratch directories.
    if not any(base in output.parents for base in (ROOT / 'outputs', ROOT / 'work')):
        raise SystemExit('Choose a new directory inside work/ or outputs/.')
    if output.exists():
        raise SystemExit('Output directory already exists; choose a new candidate directory.')
    game = args.game.resolve()
    game_hash = digest((game / 'Mewgenics.exe').read_bytes())
    for script, extra in [
        ('test_improved_inventory.py', []),
        ('build_filter_assets.py', []),
        ('validate_native_assets.py', ['--game', str(game)]),
        ('build_native_probe.py', ['--game', str(game), '--release']),
        ('test_release_guard.py', [str(game)]),
    ]:
        subprocess.run([sys.executable, str(ROOT / 'tools' / script), *extra], cwd=ROOT, check=True)
    if git('status', '--porcelain') or git('rev-parse', 'HEAD') != commit:
        raise SystemExit('Source changed during build; candidate withheld.')
    if digest((game / 'Mewgenics.exe').read_bytes()) != game_hash:
        raise SystemExit('Game changed during build; candidate withheld.')
    files = payload(version, commit, game_hash)
    output.mkdir(parents=True)
    archive = output / f'ImprovedInventory-{version}.zip'
    write_archive(archive, files)
    checksum = digest(archive.read_bytes())
    archive.with_suffix('.zip.sha256').write_text(f'{checksum}  {archive.name}\n', encoding='ascii')
    manifest = {'version': version, 'source_commit': commit, 'archive': archive.name,
                'sha256': checksum, 'files': {name: digest(data) for name, data in sorted(files.items())}}
    archive.with_suffix('.manifest.json').write_bytes(json_bytes(manifest))
    print(f'Candidate (not published): {archive}')
    print(f'SHA-256: {checksum}')


if __name__ == '__main__':
    main()
