"""Compile the native selector. Never load the DLL or run tests."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import assets

ROOT = Path(__file__).resolve().parent
VERSION = (ROOT / "VERSION").read_text(encoding="ascii").strip()
EXPECTED = "4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea"
SOURCE_FILES = ("VERSION", "native.c", "selector.h", "parts.h", "policy.h", "breeding.h", "startup.h",
                "assets.py", "build.py", "test_policy.c", "test_hook_layout.py",
                "../auto-furniture/assets.py", "../auto-furniture/localization.py",
                "../../tools/probe_inventory.py", "../../tools/validate_native_assets.py")
OUTPUT_FILES = {"dll_sha256": "GoodGenes.dll", "ui_asset_sha256": "data-mod/swfs/good_genes.swf",
                "append_sha256": "data-mod/swfs/swflist.gon.append"}


def rva_bytes(data, rva, size):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    sections, optional_size = struct.unpack_from("<H12xH", data, pe + 6)
    table = pe + 24 + optional_size
    for index in range(sections):
        _, address, raw_size, raw_offset = struct.unpack_from("<IIII", data, table + index * 40 + 8)
        if address <= rva and rva + size <= address + raw_size:
            return data[raw_offset + rva - address:raw_offset + rva - address + size]
    raise ValueError(f"RVA {rva:x} is outside the image")


def byte_list(data):
    return ",".join(map(str, data))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--zig", type=Path, required=True)
    parser.add_argument("--tools", type=Path,
                        default=ROOT.parents[1] / "tools")
    args = parser.parse_args()
    if args.tools.resolve() != (ROOT.parents[1] / "tools").resolve():
        raise SystemExit("Release builds require the repository's recorded asset tools.")
    source_hashes = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in SOURCE_FILES}
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    data = args.exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise SystemExit("Unsupported executable. Nothing built.")
    out = ROOT / "build"
    out.mkdir(exist_ok=True)
    (out / "build-info.json").unlink(missing_ok=True)
    arrays = {"EXPECTED_SHA256": bytes.fromhex(EXPECTED)}
    for name, rva in {"RANDOM_BYTES": 0xcc3f0, "MUTATE_BYTES": 0xcc970, "SET_BYTES": 0xcd080,
                      "PANEL_BYTES": 0x97bdf0, "CLOSE_BYTES": 0x77dc70,
                      "DESTROY_BYTES": 0x77f5f0, "CAT_DESTROY_BYTES": 0x5d630,
                      "BUTTON_UPDATE_BYTES": 0x97e330, "INHERIT_PART_BYTES": 0xa7820,
                      "INHERIT_STAT_BYTES": 0xa7a00}.items():
        arrays[name] = rva_bytes(data, rva, {"SET_BYTES": 28, "BUTTON_UPDATE_BYTES": 17,
                                         "INHERIT_PART_BYTES": 23, "INHERIT_STAT_BYTES": 17}.get(name, 16))
    assets.build(args.exe.parent, args.tools)
    for name, relative in {"EXPECTED_UI_SHA256": "swfs/good_genes.swf",
                           "EXPECTED_APPEND_SHA256": "swfs/swflist.gon.append"}.items():
        arrays[name] = hashlib.sha256((out / "data-mod" / relative).read_bytes()).digest()
    header = "\n".join(f"static const unsigned char {name}[] = {{{byte_list(value)}}};"
                       for name, value in arrays.items())
    header += "\nstatic const struct { uintptr_t rva; unsigned char bytes[5]; } CALL_GUARDS[] = {\n"
    for rva in (0x60b1cf, 0xcc679, 0xcc8c9, 0xccb4a, 0x932bf8, 0x933030, 0x9338a7):
        header += f"    {{0x{rva:x}, {{{byte_list(rva_bytes(data, rva, 5))}}}}},\n"
    header += "};\n"
    header += "\nstatic const struct { uintptr_t rva; unsigned size; unsigned char sha256[32]; } BREEDING_GUARDS[] = {\n"
    for start, end in ((0xa7820, 0xa79f5), (0xa7a00, 0xa7ad1), (0xa89a0, 0xaa007)):
        digest = hashlib.sha256(rva_bytes(data, start, end-start)).digest()
        header += f"    {{0x{start:x}, {end-start}, {{{byte_list(digest)}}}}},\n"
    header += "};\n"
    header += "static const struct { uintptr_t rva; unsigned size; unsigned char before[13], after[13]; } BIRTH_GATES[] = {\n"
    # Skip only new disorder creation, part inheritance loss, and new defects.
    # Parent disorder inheritance and all ordinary house mutation paths remain.
    for start, end, size in ((0xa9580, 0xa9636, 9), (0xa9728, 0xa97c1, 6),
                             (0xa9e42, 0xa9fad, 13)):
        before = rva_bytes(data, start, size)
        after = bytes([0xe9]) + struct.pack("<i", end-start-5) + bytes([0x90])*(size-5)
        header += f"    {{0x{start:x}, {size}, {{{byte_list(before)}}}, {{{byte_list(after)}}}}},\n"
    header += "};\n"
    (out / "guard.h").write_text(header, encoding="ascii")
    dll = out / "GoodGenes.dll"
    subprocess.run([str(args.zig), "cc", "-target", "x86_64-windows-gnu", "-std=c11", "-shared", "-O2",
                    "-Wall", "-Wextra", "-Werror", "-I" + str(out), str(ROOT / "native.c"),
                    "-o", str(dll), "-lbcrypt", "-lshell32"], check=True)
    info = {
        "version": VERSION,
        "source_commit": commit,
        "game_sha256": EXPECTED,
        "mewjector_api_minimum": 3,
        "tests_run": False,
        "game_launched": False,
        "installed": False,
        "source_sha256": source_hashes,
        **{key: hashlib.sha256((out / path).read_bytes()).hexdigest() for key, path in OUTPUT_FILES.items()},
    }
    if source_hashes != {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in SOURCE_FILES}:
        raise SystemExit("Source changed during compilation. Rebuild before packaging.")
    manifest = out / "build-info.json"
    manifest.write_text(json.dumps(info, indent=2) + "\n", encoding="ascii")
    print(f"Compiled: {dll}\nNo tests, DLL loading, installation, or game launch performed.")


if __name__ == "__main__":
    main()
