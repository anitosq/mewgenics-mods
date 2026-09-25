"""Read-only PE string-reference map. Findings are candidates, not a hook ABI."""
import argparse
import bisect
import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'work/python-libs'))
import capstone
import pefile


def inspect(executable, output):
    pe = pefile.PE(str(executable))
    image = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    words = ('StorageMenu', 'TrashMenu', 'inventory_storage', 'inventory_trash',
             'Inventory', 'filters', 'sort_type', 'sort_rarity', '-savesuffix',
             'storageplacement', 'StorageItems', '-user', '-save',
             'campaign01.sav', 'saves/', 'Glaiel Games', '-inheritsave')
    strings = {m.start(): m.group()[:-1].decode('ascii') for m in re.finditer(rb'[ -~]{4,}\x00', image)
               if any(w in m.group()[:-1].decode('ascii') for w in words)
               and len(m.group()) < 160}
    functions = sorted((entry.struct.BeginAddress, entry.struct.EndAddress)
                       for entry in pe.DIRECTORY_ENTRY_EXCEPTION)
    starts = [a for a, b in functions]
    refs = []
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        data = section.get_data()
        # RIP-relative LEA is how this MSVC build loads literal addresses.
        for match in re.finditer(rb'[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]', data):
            offset = match.start()
            if offset + 7 > len(data):
                continue
            rva = section.VirtualAddress + offset
            target = rva + 7 + struct.unpack_from('<i', data, offset + 3)[0]
            if target not in strings:
                continue
            index = bisect.bisect_right(starts, rva) - 1
            func = functions[index] if index >= 0 and rva < functions[index][1] else None
            refs.append(dict(text=strings[target], stringRva=hex(target), referenceRva=hex(rva),
                             functionRva=hex(func[0]) if func else None,
                             functionEnd=hex(func[1]) if func else None))
    output.mkdir(parents=True, exist_ok=True)
    (output / 'references.json').write_text(json.dumps(refs, indent=2))
    for start, end in sorted(set((int(r['functionRva'], 16), int(r['functionEnd'], 16))
                                for r in refs if r['functionRva'])):
        lines = []
        for address, size, mnemonic, operands in dis.disasm_lite(image[start:end], base + start):
            lines.append(f'{address-base:08x}  {mnemonic:8} {operands}')
        (output / f'function-{start:x}.asm').write_text('\n'.join(lines))
    print(json.dumps(refs, indent=2))


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--exe', required=True, type=Path)
    cli.add_argument('--output', required=True, type=Path)
    args = cli.parse_args()
    inspect(args.exe, args.output)
