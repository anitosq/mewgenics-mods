"""Audit the three boss-loot choices and optionally build a frozen release."""
import argparse
import copy
import re
from pathlib import Path

from build_preview import parse_gon
from probe_inventory import Gpak
from package_improved_inventory import digest, git, json_bytes, write_archive

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods/guaranteed-boss-loot"
TARGET = "data/events/quest_events.gon"
OPTIONS = {"Quest_DeadKing": "loot", "Quest_DimensionXRift": "loot", "Quest_DeadGod": "examine"}
POOLS = dict(zip(OPTIONS, ("bloody_items", "glitched_items", "godly_items")))
GAME_HASH = "4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea"


def merge(base, overlay, patch=False):
    # Only object/scalar overlays are modeled; unfamiliar operators fail closed.
    for key, value in overlay.items():
        name, dot, mode = key.rpartition(".") if patch else ("", "", "")
        if dot:
            if mode not in ("merge", "overwrite"):
                raise ValueError(f"Unsupported patch operator: {key}")
        else:
            name, mode = key, "merge"
        if mode == "overwrite" or not isinstance(value, dict) or not isinstance(base.get(name), dict):
            if isinstance(value, list) and isinstance(base.get(name), list) and mode != "overwrite":
                raise ValueError(f"Array merge requires review: {key}")
            base[name] = copy.deepcopy(value)
        else:
            merge(base[name], value, patch)


def option(tree, event):
    return tree[event]["main"]["options"][OPTIONS[event]]


def verify(before, after):
    restored = copy.deepcopy(after)
    for event in OPTIONS:
        original, current = option(before, event), option(after, event)
        if current.get("fixed_chance") != "100%" or current.get("bad") != "null":
            raise ValueError(f"Guarantee overwritten: {event}")
        if current.get("good") != original.get("good") or current["good"].get("get_item_from_pool") != POOLS[event]:
            raise ValueError(f"Good reward changed or missing: {event}")
        if any(current.get(key, "null") != "null" for key in ("fail", "critfail", "copy_results")):
            raise ValueError(f"Additional result path requires review: {event}")
        for key in ("fixed_chance", "bad"):
            if key in original:
                option(restored, event)[key] = copy.deepcopy(original[key])
            else:
                option(restored, event).pop(key, None)
    if restored != before:
        raise ValueError("Unrelated event data changed")


def check(game):
    if digest((game / "Mewgenics.exe").read_bytes()) != GAME_HASH:
        raise ValueError("Executable changed; recheck fixed_chance and null-outcome semantics")
    patch = parse_gon((MOD / (TARGET + ".patch")).read_text())
    expected = {event: {"main": {"options": {name: {"fixed_chance": "100%", "bad.overwrite": "null"}}}}
                for event, name in OPTIONS.items()}
    if patch != expected:
        raise ValueError("Patch differs from the reviewed six-field scope")
    base = parse_gon(Gpak(game / "resources.gpak").read(TARGET).decode("utf-8-sig"))
    modlist = game / "mods/modlist.txt"
    order = [line.strip() for line in modlist.read_text(encoding="utf-8-sig").splitlines()
             if line.strip() and not line.lstrip().startswith("#")] if modlist.exists() else []
    if len(order) != len(set(order)) or any(Path(m).name != m or m in (".", "..") for m in order):
        raise ValueError("Invalid or duplicate modlist entries")
    for label, names in (("vanilla", []), ("current setup", order),
                         ("without old mod", [m for m in order if m != "NeverFailBossLooting"])):
        before = copy.deepcopy(base)
        for name in names:
            if name == "GuaranteedBossLoot":
                continue
            paths = list((game / "mods" / name / "data/events").glob("quest_events.gon*"))
            if len(paths) > 1:
                raise ValueError(f"Multiple event overlays require review: {name}")
            for path in paths:
                data = parse_gon(path.read_text(encoding="utf-8-sig"))
                if path.name == "quest_events.gon":
                    before = data
                elif path.suffix in (".merge", ".patch"):
                    merge(before, data, path.suffix == ".patch")
                else:
                    raise ValueError(f"Event overlay requires review: {path}")
        after = copy.deepcopy(before)
        merge(after, patch, True)
        verify(before, after)
        for field, value in (("bad", {"kill": "cat"}), ("fixed_chance", "50%")):
            broken = copy.deepcopy(after)
            option(broken, "Quest_DeadKing")[field] = value
            try:
                verify(before, broken)
            except ValueError:
                pass
            else:
                raise AssertionError(f"Failed to detect overwritten {field}")
        print(f"PASS: {label}; three rewards preserved, six-field scope, conflict controls.")


def build():
    if git("status", "--porcelain"):
        raise ValueError("Commit changes before packaging; working tree must be clean")
    version = (MOD / "VERSION").read_text().strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Invalid version")
    commit = git("rev-parse", "HEAD")
    files = {"GuaranteedBossLoot/" + name: (MOD / name).read_bytes()
             for name in (TARGET + ".patch", "README.md")}
    files["GuaranteedBossLoot/LICENSE"] = (ROOT / "LICENSE").read_bytes()
    files["GuaranteedBossLoot/description.json"] = json_bytes({
        "title": "Guaranteed Boss Loot", "author": "anitosq", "version": version,
        "description": "Collect the normal act-boss reward without losing the cat who searches. Covers the Dead King, Rift and Dead God. No DLL loader required.",
        "url": "https://github.com/anitosq/mewgenics-mods",
    })
    if git("status", "--porcelain") or git("rev-parse", "HEAD") != commit:
        raise ValueError("Source changed during packaging")
    output = ROOT / "outputs/releases/guaranteed-boss-loot" / version
    output.mkdir(parents=True, exist_ok=False)
    archive = output / f"GuaranteedBossLoot-{version}.zip"
    write_archive(archive, files)
    checksum = digest(archive.read_bytes())
    archive.with_suffix(".zip.sha256").write_text(f"{checksum}  {archive.name}\n", encoding="ascii")
    archive.with_suffix(".manifest.json").write_bytes(json_bytes({
        "version": version, "source_commit": commit, "archive": archive.name,
        "sha256": checksum, "files": {name: digest(data) for name, data in sorted(files.items())},
    }))
    print(f"Built and verified: {archive}\nSHA-256: {checksum}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--build", action="store_true")
    args = parser.parse_args()
    try:
        check(args.game)
        if args.build:
            build()
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"FAIL: {error}\n")
