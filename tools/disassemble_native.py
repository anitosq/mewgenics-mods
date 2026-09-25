"""Inspect selected functions in the local game; generated reports stay in work/."""
import argparse
from pathlib import Path
import struct
import sys
import bisect
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'work/python-libs'))
import capstone
import pefile

cli = argparse.ArgumentParser(description=__doc__)
cli.add_argument('addresses', nargs='+', type=lambda s: int(s, 16))
cli.add_argument('--exe', type=Path, default=Path('C:/Program Files (x86)/Steam/steamapps/common/Mewgenics/Mewgenics.exe'))
cli.add_argument('--vtable', action='store_true')
cli.add_argument('--calls', action='store_true')
cli.add_argument('--size', type=lambda s: int(s, 0), help='Explicit byte range, including split unwind fragments')
args = cli.parse_args()
pe = pefile.PE(str(args.exe))
image = pe.get_memory_mapped_image()
base = pe.OPTIONAL_HEADER.ImageBase
dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
out = Path('work/inventory-native')
out.mkdir(exist_ok=True, parents=True)
for address in args.addresses:
    if args.calls:
        functions = sorted((e.struct.BeginAddress, e.struct.EndAddress) for e in pe.DIRECTORY_ENTRY_EXCEPTION)
        starts = [f[0] for f in functions]
        index = image.find(b'\xe8')
        while index >= 0 and index+5 <= len(image):
            if index+5+struct.unpack_from('<i', image, index+1)[0] == address:
                owner = bisect.bisect_right(starts, index)-1
                print(f'call {index:x}: containing entry candidate {starts[owner]:x}')
            index = image.find(b'\xe8', index+1)
        continue
    if args.vtable:
        for offset in range(0, 0x90, 8):
            pointer = struct.unpack_from('<Q', image, address + offset)[0]
            print(f'{address:x}+{offset:x}: {pointer-base:x}')
        continue
    function = next((e.struct for e in pe.DIRECTORY_ENTRY_EXCEPTION
                     if e.struct.BeginAddress <= address < e.struct.EndAddress), None)
    if args.size:
        start, end = address, address + args.size
    elif function is not None:
        start, end = function.BeginAddress, function.EndAddress
    else:
        print(f'No function at {address:x}')
        continue
    lines = []
    for addr, size, mnemonic, operands in dis.disasm_lite(image[start:end], base+start):
        lines.append(f'{addr-base:08x}  {mnemonic:8} {operands}')
    path = out / f'function-{start:x}.asm'
    path.write_text('\n'.join(lines))
    print(f'{path}: {len(lines)} instructions')
