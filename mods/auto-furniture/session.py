"""Disposable slot-3 test session. Does not edit the main campaign."""
import argparse
from contextlib import closing
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import sqlite3
import struct
import subprocess
import time

from build import EXPECTED, ROOT

MARKER_ID = 0x4155464E
ACTIVE = ROOT / "work" / "active.json"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def running():
    return '"Mewgenics.exe"' in subprocess.check_output(
        ["tasklist", "/FI", "IMAGENAME eq Mewgenics.exe", "/FO", "CSV"], text=True)


def decode(data):
    offset = 4
    if len(data) < 4 or struct.unpack_from("<I", data)[0] != 1:
        raise ValueError("Unsupported furniture version")

    def string():
        nonlocal offset
        length = struct.unpack_from("<Q", data, offset)[0]
        offset += 8
        if length > 1024 or offset + length > len(data):
            raise ValueError("Invalid furniture string")
        result = data[offset:offset + length].decode("utf-8")
        offset += length
        return result

    name = string()
    variant = data[offset:offset + 8]
    offset += 8
    room_start = offset
    room = string()
    if offset + 20 != len(data) or len(variant) != 8:
        raise ValueError("Unsupported furniture payload length")
    return name, variant, room, struct.unpack_from("<5i", data, offset), room_start, offset


def records(path):
    with closing(sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)) as db:
        db.execute("PRAGMA query_only=ON")
        if db.execute("PRAGMA quick_check").fetchone() != ("ok",):
            raise ValueError("Save integrity check failed")
        result = dict(db.execute("SELECT key,data FROM furniture"))
        for data in result.values():
            decode(data)
        return result


def fixture(source, destination):
    if destination.exists():
        raise ValueError("Fixture destination must be new")
    with closing(sqlite3.connect(source.resolve().as_uri() + "?mode=ro", uri=True)) as src:
        with closing(sqlite3.connect(destination)) as dst:
            src.backup(dst)
    items = records(destination)
    if MARKER_ID in items:
        raise ValueError("Source already contains the diagnostic marker")
    item = next((v for v in items.values() if decode(v)[2] == "" and
                 decode(v)[0] not in ("poop", "autofeeder")), None)
    if item is None:
        raise ValueError("Test requires one owned inventory piece to duplicate as its marker")
    with closing(sqlite3.connect(destination)) as db, db:
        db.execute("INSERT INTO furniture(key,data) VALUES (?,?)", (MARKER_ID, item))
    assert records(destination) == {**items, MARKER_ID: item}


def loader_config(original, dll):
    lines = original.decode("utf-8-sig").splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines) if line.strip() == "[LoadOrder]"]
    if len(starts) != 1 or any("AutoFurnitureProbe" in line for line in lines):
        raise ValueError("Unexpected loader configuration")
    start = starts[0] + 1
    end = next((i for i in range(start, len(lines)) if lines[i].lstrip().startswith("[")), len(lines))
    numbers = [int(m[1]) for line in lines[start:end] if (m := re.match(r"Mod(\d+)\s*=", line))]
    newline = "\r\n" if b"\r\n" in original else "\n"
    lines.insert(end, f"{newline}Mod{max(numbers, default=0) + 1}={dll}{newline}")
    return "".join(lines).encode("utf-8")


def prepare(game, profile, reload_fixture=None, release=False):
    if running() or ACTIVE.exists():
        raise RuntimeError("Close the game and clean up any existing diagnostic session first")
    if sha(game / "Mewgenics.exe") != EXPECTED:
        raise ValueError("Unsupported executable")
    dll = ROOT / ("build/AutoFurniture.dll" if release else "build/AutoFurnitureProbe.dll")
    if not dll.exists():
        raise ValueError("Build the DLL first")
    backup = ROOT / "work" / datetime.now().strftime("%Y%m%d-%H%M%S")
    backup.mkdir(parents=True)
    shutil.copytree(profile / "saves", backup / "saves")
    shutil.copy2(profile / "settings.txt", backup / "settings.txt")
    config = game / "chainloader.ini"
    original = config.read_bytes()
    temporary = loader_config(original, dll)
    (backup / "chainloader.ini").write_bytes(original)
    (backup / "temporary-chainloader.ini").write_bytes(temporary)
    installed = game / "mods/AutoFurniture.dll"
    if installed.exists():
        shutil.copy2(installed, backup / "installed-AutoFurniture.dll")
    if reload_fixture is None:
        fixture(profile / "saves/steamcampaign01.sav", backup / "fixture-before.sav")
    else:
        reload_fixture=reload_fixture.resolve()
        if not reload_fixture.is_relative_to((ROOT / "work").resolve()) or MARKER_ID not in records(reload_fixture):
            raise ValueError("Reload requires a marked disposable fixture from this workspace")
        shutil.copy2(reload_fixture, backup / "fixture-before.sav")
    # Journal exists before any deployment. Cleanup refuses unmarked external changes.
    ACTIVE.write_text(json.dumps({"game": str(game), "profile": str(profile), "backup": str(backup)}, indent=2))
    if installed.exists():
        installed.unlink()  # Avoid loading both the installed and test DLLs.
    shutil.copy2(backup / "fixture-before.sav", profile / "saves/steamcampaign03.sav")
    (ROOT / "build/AutoFurnitureProbe.session").write_text("disposable-slot-3", encoding="ascii")
    config.write_bytes(temporary)
    print(f"Backups: {backup}\nTest campaign: RIGHTMOST Home, slot 3. One extra inventory piece marks the copy.", flush=True)
    return backup


def cleanup():
    if running():
        raise RuntimeError("Close the game normally before cleanup")
    state = json.loads(ACTIVE.read_text())
    game, profile, backup = (Path(state[k]) for k in ("game", "profile", "backup"))
    slot = profile / "saves/steamcampaign03.sav"
    original_slot = backup / "saves/steamcampaign03.sav"
    if slot.exists():
        if MARKER_ID not in records(slot) and (not original_slot.exists() or sha(slot) != sha(original_slot)):
            raise RuntimeError("Slot 3 changed externally. Restoration withheld; backups retained")
        shutil.copy2(slot, backup / "fixture-after.sav")
    config = game / "chainloader.ini"
    if config.read_bytes() == (backup / "temporary-chainloader.ini").read_bytes():
        config.write_bytes((backup / "chainloader.ini").read_bytes())
    elif config.read_bytes() != (backup / "chainloader.ini").read_bytes():
        raise RuntimeError("Loader configuration changed externally. Restoration withheld")
    installed, saved_dll = game / "mods/AutoFurniture.dll", backup / "installed-AutoFurniture.dll"
    if saved_dll.exists():
        if installed.exists() and sha(installed) != sha(saved_dll):
            raise RuntimeError("Installed mod changed externally. Restoration withheld")
        shutil.copy2(saved_dll, installed)
    if original_slot.exists():
        shutil.copy2(original_slot, slot)
    else:
        slot.unlink(missing_ok=True)
    for suffix in ("session", "command"):
        (ROOT / f"build/AutoFurnitureProbe.{suffix}").unlink(missing_ok=True)
    log = game / "mod_logs/chainloader.log"
    if log.exists():
        shutil.copy2(log, backup / "chainloader.log")
    comparisons = {}
    for p in (backup / "saves").iterdir():
        if p.is_file():
            now = profile / "saves" / p.name
            comparisons[p.name] = now.exists() and sha(now) == sha(p)
    (backup / "restoration.json").write_text(json.dumps(comparisons, indent=2))
    ACTIVE.unlink()
    print(f"Restored loader and original slot 3. Save hashes unchanged: {comparisons}\nEvidence: {backup}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    run = commands.add_parser("run")
    run.add_argument("--game", type=Path, required=True)
    run.add_argument("--profile", type=Path, required=True)
    run.add_argument("--fixture", type=Path, help="Reload a marked disposable test save")
    run.add_argument("--release", action="store_true", help="Test the release DLL on the disposable slot")
    commands.add_parser("cleanup")
    command = commands.add_parser("command")
    command.add_argument("text")
    args = parser.parse_args()
    if args.action == "cleanup":
        cleanup()
    elif args.action == "command":
        if not ACTIVE.exists() or not running():
            raise RuntimeError("No active test session")
        if (args.text not in ("dump", "undo", "apply") and not re.fullmatch(r"return [A-Za-z0-9_]+", args.text)
                and not re.fullmatch(r"preview [A-Za-z0-9_]+ \d+ \d+(?: [0-9.]+){4,5}", args.text)):
            raise ValueError("Expected dump, undo, return ROOM, or preview ROOM SELECTED TARGETS C S H M")
        with (ROOT / "build/AutoFurnitureProbe.command").open("x", encoding="ascii") as f:
            f.write(args.text)
    else:
        game, profile = args.game.resolve(), args.profile.resolve()
        prepare(game, profile, args.fixture, args.release)
        try:
            mods = [str(game / "mods" / n.strip()) for n in
                    (game / "mods/modlist.txt").read_text(encoding="utf-8-sig").splitlines()
                    if n.strip() and not n.startswith("#") and n.strip().casefold() != "autofurniture"]
            mods.append(str(ROOT / "build/data-mod"))
            subprocess.run([str(game.parents[2] / "steam.exe"), "-applaunch", "686060", "-modpaths", *mods], cwd=game, check=True)
            deadline = time.monotonic() + 120
            while not running() and time.monotonic() < deadline:
                time.sleep(0.5)
            if not running():
                raise RuntimeError("Game did not start")
            while running():
                time.sleep(2)
        finally:
            if not running():
                cleanup()
            else:
                print("Game still running. Journal retained; run cleanup after normal exit.")


if __name__ == "__main__":
    main()
