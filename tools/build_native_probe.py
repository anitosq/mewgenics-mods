"""Build a guarded research DLL. Does not deploy it or launch the game."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

cli = argparse.ArgumentParser(description=__doc__)
cli.add_argument('--game', type=Path, required=True)
args = cli.parse_args()
root = Path(__file__).resolve().parents[1]
probe = json.loads((root / 'work/inventory-probe/probe-report.json').read_text())
binary = (args.game / 'Mewgenics.exe').read_bytes()
sha = hashlib.sha256(binary).hexdigest()
if sha != probe['binary']['sha256'] or probe['binary']['matched'] != 44:
    raise RuntimeError('Current image does not match the researched baseline')
pe = struct.unpack_from('<I', binary, 0x3c)[0]
optional = struct.unpack_from('<H', binary, pe + 20)[0]
rva = 0x5A3D0  # ABI/address research from cat-table; validated against this image.
signature = None
grid_rva = 0x211700  # ref_grid + InventoryGridBGBox references in this exact build.
grid_signature = None
bootstrap_rva = 0x9B9970  # Actual validated bridge constant; its older comment is stale.
bootstrap_signature = None
extra = {'DRAWER_UPDATE':0x213ac0, 'MOUSE_EVENT':0xc36110, 'ITEM_CLICK':0x213dc0,
         'ITEM_BIND':0x2137e0, 'MOUSE_POSITION':0x9796d0, 'BUTTON_HIT':0x97f0e0}
extra_signatures = {}
for index in range(struct.unpack_from('<H', binary, pe + 6)[0]):
    start = pe + 24 + optional + index * 40
    virtual_size, virtual, size, offset = struct.unpack_from('<IIII', binary, start + 8)
    for key, address in extra.items():
        if virtual<=address and address+64<=virtual+size:
            extra_signatures[key]=binary[offset+address-virtual:offset+address-virtual+64]
    if virtual <= rva and rva + 64 <= virtual + size:
        signature = binary[offset + rva - virtual:offset + rva - virtual + 64]
    if virtual <= grid_rva and grid_rva + 64 <= virtual + size:
        grid_signature = binary[offset + grid_rva - virtual:offset + grid_rva - virtual + 64]
    if virtual <= bootstrap_rva and bootstrap_rva + 64 <= virtual + size:
        bootstrap_signature = binary[offset + bootstrap_rva - virtual:offset + bootstrap_rva - virtual + 64]
if signature is None or grid_signature is None or bootstrap_signature is None or len(extra_signatures)!=len(extra):
    raise RuntimeError('Renderer outside file-backed sections')
build = root / 'work/native-build'
build.mkdir(parents=True, exist_ok=True)
literal = lambda data: ','.join(f'0x{v:02x}' for v in data)
(build / 'build_guard.h').write_text(
    f'#define RENDERER_RVA 0x{rva:x}\n'
    f'#define GRID_RVA 0x{grid_rva:x}\n'
    f'#define BOOTSTRAP_RVA 0x{bootstrap_rva:x}\n'
    f'#define EXPECTED_TIMESTAMP 0x{struct.unpack_from("<I", binary, pe+8)[0]:x}\n'
    f'#define EXPECTED_IMAGE_SIZE 0x{struct.unpack_from("<I", binary, pe+24+56)[0]:x}\n'
    f'static const unsigned char EXPECTED_SHA256[32] = {{{literal(bytes.fromhex(sha))}}};\n'
    f'static const unsigned char EXPECTED_RENDERER_BYTES[64] = {{{literal(signature)}}};\n'
    f'static const unsigned char EXPECTED_GRID_BYTES[64] = {{{literal(grid_signature)}}};\n'
    f'static const unsigned char EXPECTED_BOOTSTRAP_BYTES[64] = {{{literal(bootstrap_signature)}}};\n'+
    ''.join(f'#define {key}_RVA 0x{extra[key]:x}\nstatic const unsigned char EXPECTED_{key}_BYTES[64] = {{{literal(value)}}};\n' for key,value in extra_signatures.items()))
compiler = root / 'work/toolchains/zig-x86_64-windows-0.15.2/zig.exe'
output = build / 'InventoryQoLProbe.dll'
subprocess.run([str(compiler), 'cc', '-target', 'x86_64-windows-gnu', '-shared', '-O2',
                '-Wall', '-Wextra', '-Werror', '-I', str(build), str(root / 'mods/inventory-qol/src/native/inventory_probe.c'),
                '-o', str(output), '-lbcrypt', '-luser32'], check=True)
print(output)
