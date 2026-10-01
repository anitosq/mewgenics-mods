"""Build exact-version guarded release and disposable-test DLLs."""
import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import assets

EXPECTED = "4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea"
ROOT = Path(__file__).resolve().parent
VERSION = (ROOT / "VERSION").read_text(encoding="ascii").strip()


def rva_bytes(data, rva, size=64):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    sections, optional_size = struct.unpack_from("<H12xH", data, pe + 6)
    table = pe + 24 + optional_size
    for index in range(sections):
        _, address, raw_size, raw_offset = struct.unpack_from("<IIII", data, table + index * 40 + 8)
        if address <= rva and rva + size <= address + raw_size:
            return data[raw_offset + rva - address:raw_offset + rva - address + size]
    raise ValueError(f"RVA {rva:x} is outside the image")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--zig", type=Path, required=True)
    args = parser.parse_args()
    data = args.exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise SystemExit("Unsupported executable. No DLL built.")
    out = ROOT / "build"
    out.mkdir(exist_ok=True)
    assets.build()
    arrays = {"EXPECTED_SHA256": bytes.fromhex(EXPECTED),
              "EXPECTED_UI_SHA256": hashlib.sha256((out / "data-mod/swfs/auto_furniture.swf").read_bytes()).digest(),
              "EXPECTED_APPEND_SHA256": hashlib.sha256((out / "data-mod/swfs/swflist.gon.append").read_bytes()).digest(),
              "UPDATE_BYTES": rva_bytes(data, 0x1A5AB0),
              "INPUT_BYTES": rva_bytes(data, 0xC36110),
              "HIT_BYTES": rva_bytes(data, 0x97F0E0),
              "HOVER_BYTES": rva_bytes(data, 0x1A0880)}
    (out / "guard.h").write_text("\n".join(
        f"static const unsigned char {name}[]={{" + ",".join(map(str, value)) + "};"
        for name, value in arrays.items()) + f'\n#define AF_VERSION "{VERSION}"\n', encoding="ascii")
    for name, flags in [("AutoFurnitureProbe", []), ("AutoFurniture", ["-DAF_RELEASE"])]:
        subprocess.run([str(args.zig), "cc", "-target", "x86_64-windows-gnu", "-shared", "-O2", *flags,
                    "-Wall", "-Wextra", "-Werror", "-I" + str(out), str(ROOT / "native.c"),
                    "-o", str(out / f"{name}.dll"), "-lbcrypt", "-luser32", "-lshell32"], check=True)
        print(out / f"{name}.dll")
    subprocess.run([str(args.zig), "cc", "-O2", "-UNDEBUG", "-DAF_RELEASE",
                    "-Wall", "-Wextra", "-Werror", "-I" + str(out), str(ROOT / "test_ui.c"),
                    "-o", str(out / "test_ui.exe"), "-lbcrypt", "-luser32", "-lshell32"], check=True)
    subprocess.run([str(out / "test_ui.exe")], check=True)


if __name__ == "__main__":
    main()
