"""Create a private-data-free Vortex archive from the tested release build."""
import hashlib
import json
import subprocess
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

from build import EXPECTED, ROOT, VERSION


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def main():
    if subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True).strip():
        raise SystemExit("Commit release source before packaging.")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    # Explicit allowlist: never package work/, diagnostic DLLs, saves, or logs.
    files = {
        "AutoFurniture/AutoFurniture.dll": (ROOT / "build/AutoFurniture.dll").read_bytes(),
        "AutoFurniture/swfs/auto_furniture.swf": (ROOT / "build/data-mod/swfs/auto_furniture.swf").read_bytes(),
        "AutoFurniture/swfs/swflist.gon.append": (ROOT / "build/data-mod/swfs/swflist.gon.append").read_bytes(),
        "AutoFurniture/data/text/combined.csv.append": (ROOT / "build/data-mod/data/text/combined.csv.append").read_bytes(),
        "AutoFurniture/README.md": (ROOT / "README.md").read_bytes(),
        "AutoFurniture/LICENSE": (ROOT / "LICENSE").read_bytes(),
        "AutoFurniture/THIRD_PARTY_NOTICES.md": (ROOT / "THIRD_PARTY_NOTICES.md").read_bytes(),
        "AutoFurniture/description.json": json_bytes({
            "title": "Auto Furniture", "author": "anitosq", "version": VERSION,
            "description": "Arrange rooms using furniture you own, with optional stat targets, pins and Undo. Requires Mewjector API 3.",
        }),
        "AutoFurniture/build-info.json": json_bytes({
            "version": VERSION, "source_commit": commit, "game_sha256": EXPECTED, "steam_build_id": 25143593,
            "loader_api_minimum": 3, "compiler": "zig 0.15.2",
        }),
    }
    # Reuse Improved Inventory's tested nested payload / Vortex mapping convention.
    config = ET.Element("config", {"xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance",
        "xsi:noNamespaceSchemaLocation": "http://qconsulting.ca/fo3/ModConfig5.0.xsd"})
    ET.SubElement(config, "moduleName").text = "Auto Furniture"
    required = ET.SubElement(config, "requiredInstallFiles")
    for name in sorted(files):
        target = "AutoFurniture.dll" if name.endswith("/AutoFurniture.dll") else name
        ET.SubElement(required, "file", source=name.replace("/", "\\"),
                      destination=("mods/" + target).replace("/", "\\"), priority="0")
    ET.indent(config)
    files["fomod/ModuleConfig.xml"] = ET.tostring(config, encoding="utf-8")
    metadata = "vortex_override_instructions.json"
    files[metadata] = json_bytes([{"type": "attribute", "source": metadata,
                                   "key": "modName", "value": "AutoFurniture"}])
    output = ROOT / "dist"
    output.mkdir(exist_ok=True)
    archive = output / f"AutoFurniture-{VERSION}.zip"
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            z.writestr(info, data)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None and set(z.namelist()) == set(files)
        assert all(z.read(name) == data for name, data in files.items())
    mapping = ET.fromstring(files["fomod/ModuleConfig.xml"]).find("requiredInstallFiles")
    assert len(mapping) == 9
    assert {entry.attrib["source"].replace("\\", "/") for entry in mapping} == {
        name for name in files if name.startswith("AutoFurniture/")}
    assert len({entry.attrib["destination"] for entry in mapping}) == 9
    digest = lambda data: hashlib.sha256(data).hexdigest()
    manifest = {"version": VERSION, "source_commit": commit, "sha256": digest(archive.read_bytes()),
                "files": {name: digest(data) for name, data in sorted(files.items())}}
    archive.with_suffix(".manifest.json").write_bytes(json_bytes(manifest))
    archive.with_suffix(".zip.sha256").write_text(f"{manifest['sha256']}  {archive.name}\n", encoding="ascii")
    print(f"Verified {len(files)} files: {archive}\nSHA-256: {manifest['sha256']}")


if __name__ == "__main__":
    main()
