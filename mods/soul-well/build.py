"""Build Soul Well and run native checks without touching the installed game."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parent
VERSION = (ROOT / 'VERSION').read_text(encoding='ascii').strip()
EXPECTED = '4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea'
RVAS = (0x43350, 0x475c0, 0x7a2ba0, 0x48fb0, 0xd7220, 0x119e20, 0x43a50)
ITEM = 'data/items/trinkets.gon.merge'
ABILITY = 'data/abilities/item_abilities.gon.merge'
SHOP = 'data/shops/tracy_house_shop.gon.patch'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--zig', required=True, type=Path)
    args = parser.parse_args()
    data = args.exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise SystemExit('Unsupported executable; no DLL built.')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, size = struct.unpack_from('<H12xH', data, pe + 6)
    sections = [struct.unpack_from('<IIII', data, pe + 24 + size + 40 * i + 8) for i in range(count)]

    def at(rva):
        for _, address, raw_size, raw_offset in sections:
            if address <= rva and rva + 64 <= address + raw_size:
                return data[raw_offset + rva - address:raw_offset + rva - address + 64]
        raise ValueError('Unmapped RVA')

    out = ROOT / 'build'
    out.mkdir(exist_ok=True)
    arrays = {'EXPECTED_SHA256': bytes.fromhex(EXPECTED)}
    guard = [f'static const unsigned char {name}[] = {{' + ','.join(map(str, value)) + '};'
             for name, value in arrays.items()]
    for name, relative in (('ITEM_TEXT', ITEM), ('SHOP_TEXT', SHOP), ('ABILITY_TEXT', ABILITY)):
        guard.append(f'static const char {name}[] = ' + json.dumps((ROOT / relative).read_text(encoding='utf-8')) + ';')
    guard += ['#define SW_VERSION ' + json.dumps(VERSION),
              'static const struct { unsigned rva; unsigned char bytes[64]; } GUARDS[] = {']
    guard.extend('{0x%x, {%s}},' % (rva, ','.join(map(str, at(rva)))) for rva in RVAS)
    guard.append('};\n')
    (out / 'guard.h').write_text('\n'.join(guard), encoding='ascii')
    common = [str(args.zig), 'cc', '-target', 'x86_64-windows-gnu', '-std=c11', '-O2',
              '-Wall', '-Wextra', '-Werror', '-I' + str(out)]
    subprocess.run([*common, '-shared', str(ROOT / 'native.c'), '-o', str(out / 'SoulWell.dll'),
                    '-lbcrypt', '-lshell32', '-luser32'], check=True)
    subprocess.run([*common, str(ROOT / 'test_native.c'), '-o', str(out / 'test_native.exe'),
                    '-lbcrypt', '-lshell32', '-luser32'], check=True)
    subprocess.run([str(out / 'test_native.exe')], check=True)
    report = {'version': VERSION, 'game_sha256': EXPECTED, 'offline_tests': 'passed',
              'compiler': subprocess.check_output([str(args.zig), 'version'], text=True).strip(),
              'dll_sha256': hashlib.sha256((out / 'SoulWell.dll').read_bytes()).hexdigest(),
              'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
              'dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip())}
    (out / 'build-info.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('Built SoulWell.dll; native offline checks passed.')


if __name__ == '__main__':
    main()
