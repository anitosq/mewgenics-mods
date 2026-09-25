"""Read-only feasibility probe. No writes to the game or source save.

Inventory format reference: michael-trinity/mewgenics-savegame-editor,
app/utils/parse/inventory.ts (MIT; see THIRD_PARTY_NOTICES.md).
Unknown fields deliberately remain uninterpreted.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import struct
import zlib


class Reader:
    def __init__(self, data: bytes):
        self.data, self.pos = data, 0

    def take(self, size: int) -> bytes:
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError(f"Truncated data at {self.pos}, requested {size}")
        value = self.data[self.pos:self.pos + size]
        self.pos += size
        return value

    def u32(self):
        return struct.unpack('<I', self.take(4))[0]

    def string(self):
        length = self.u32()
        if length > 16384 or self.u32() != 0:
            raise ValueError('Unsupported string length/padding')
        return self.take(length).decode('utf-8')


def parse_inventory(data: bytes, container: str) -> list[dict]:
    r = Reader(data)
    count = r.u32()
    if count > 100000:
        raise ValueError('Implausible item count')
    if count == 0:
        if r.pos != len(data):
            raise ValueError('Unexpected empty-container trailer')
        return []
    if r.u32() != 5:
        raise ValueError('Unsupported inventory record version')
    result = []
    for index in range(count):
        start = r.pos
        flag = r.take(1)[0]
        name, subname = r.string(), r.string()
        charges = struct.unpack('<i', r.take(4))[0]
        field1, field2, sequence = r.u32(), r.u32(), r.u32()
        tail = r.take(1)[0]
        record_hash = hashlib.sha256(data[start:r.pos]).hexdigest()[:12]
        result.append(dict(instanceId=f'{container}:{index}:{record_hash}',
                           itemId=name, subname=subname, charges=charges,
                           container=container, ordinal=index,
                           raw=dict(flag=flag, field1=field1, field2=field2,
                                    sequence=sequence, tail=tail),
                           condition='unknown'))
        # Separator prefix/trailer is observed, but its semantics are not known.
        r.take(1)
        if index + 1 < count and r.u32() != 5:
            raise ValueError('Unexpected inter-item version')
    if r.pos != len(data):
        raise ValueError(f'Unconsumed inventory bytes: {len(data) - r.pos}')
    return result


def read_save(path: Path):
    # SQLite backup supplies a consistent in-memory snapshot, even with WAL.
    # mode=ro prevents the connection from writing the source database.
    with sqlite3.connect(path.resolve().as_uri() + '?mode=ro', uri=True) as source:
        with sqlite3.connect(':memory:') as snapshot:
            source.backup(snapshot)
            inventory, blobs = [], {}
            for container in ('backpack', 'storage', 'trash'):
                row = snapshot.execute('SELECT data FROM files WHERE key=?',
                                       ('inventory_' + container,)).fetchone()
                if row is None:
                    raise ValueError(f'Missing inventory_{container}')
                blob = bytes(row[0])
                blobs[container] = dict(bytes=len(blob), sha256=hashlib.sha256(blob).hexdigest())
                inventory.extend(parse_inventory(blob, container))
    return inventory, blobs


class Gpak:
    def __init__(self, path: Path):
        self.path, self.entries = path, {}
        with path.open('rb') as file:
            count = struct.unpack('<i', file.read(4))[0]
            if not 0 < count < 1000000:
                raise ValueError('Invalid GPAK count')
            entries = []
            for _ in range(count):
                length = struct.unpack('<h', file.read(2))[0]
                if not 0 < length < 4096:
                    raise ValueError('Invalid GPAK path length')
                name = file.read(length).decode('utf-8')
                size = struct.unpack('<i', file.read(4))[0]
                if size < 0:
                    raise ValueError('Negative GPAK entry size')
                entries.append((name, size))
            offset = file.tell()
        for name, size in entries:
            if name in self.entries:
                raise ValueError('Duplicate GPAK entry')
            self.entries[name] = (offset, size)
            offset += size
        if offset != path.stat().st_size:
            raise ValueError('GPAK index does not cover archive exactly')

    def read(self, name: str) -> bytes:
        offset, size = self.entries[name]
        with self.path.open('rb') as file:
            file.seek(offset)
            result = file.read(size)
        if len(result) != size:
            raise ValueError('Truncated GPAK payload')
        return result


def swf_names(data):
    if data[:3] == b'CWS':
        body = zlib.decompress(data[8:])
    elif data[:3] == b'FWS':
        body = data[8:]
    else:
        return {'format': data[:3].decode('ascii', errors='replace'), 'candidates': []}
    if len(body) + 8 != struct.unpack_from('<I', data, 4)[0]:
        raise ValueError('Unexpected decompressed SWF length')
    names = sorted(set(s.decode('ascii') for s in re.findall(rb'[A-Za-z_][A-Za-z_0-9]{3,90}', body)
                       if re.search(rb'invent|storage|trash|item|scroll|filter|sort', s, re.I)))
    return {'format': data[:3].decode(), 'candidates': names}


def inspect_exe(path, reference):
    binary = path.read_bytes()
    pe = struct.unpack_from('<I', binary, 0x3c)[0]
    if binary[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not a PE image')
    count = struct.unpack_from('<H', binary, pe + 6)[0]
    optional = struct.unpack_from('<H', binary, pe + 20)[0]
    sections = []
    for index in range(count):
        start = pe + 24 + optional + index * 40
        virtual_size, rva, size, offset = struct.unpack_from('<IIII', binary, start + 8)
        sections.append((rva, size, offset))
    checks = []
    if reference.exists():
        source = reference.read_text(encoding='utf-8-sig')
        for address, values in re.findall(r'(0x[0-9A-Fa-f]+), new ByteKey64\(new byte\[64\] \{(.*?)\}', source, re.S):
            rva = int(address, 16)
            expected = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', values))
            section = next((s for s in sections if s[0] <= rva and rva + len(expected) <= s[0] + s[1]), None)
            actual = binary[section[2] + rva - section[0]:section[2] + rva - section[0] + len(expected)] if section else b''
            checks.append({'rva': address, 'matchesReference': len(expected) == 64 and actual == expected})
    return {'sha256': hashlib.sha256(binary).hexdigest(), 'referenceChecks': checks,
            'matched': sum(c['matchesReference'] for c in checks), 'checked': len(checks)}


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--game', required=True, type=Path)
    cli.add_argument('--save', required=True, type=Path)
    cli.add_argument('--output', required=True, type=Path)
    cli.add_argument('--reference', type=Path, default=Path('work/modding-sources/catstable/expectedBinaryData.cs'))
    args = cli.parse_args()
    # Outputs must not land inside the game or the source save directory.
    output = args.output.resolve()
    for forbidden in (args.game.resolve(), args.save.resolve().parent):
        if output == forbidden or forbidden in output.parents:
            raise ValueError('Choose a project output folder, outside game/save folders')
    inventory, blobs = read_save(args.save)
    archive = Gpak(args.game / 'resources.gpak')
    report = {'saveName': args.save.name, 'sourceModified': args.save.stat().st_mtime,
              'containers': dict(Counter(i['container'] for i in inventory)), 'blobs': blobs,
              'itemCount': len(inventory), 'distinctItemTypes': len(set(i['itemId'] for i in inventory)),
              'binary': inspect_exe(args.game / 'Mewgenics.exe', args.reference),
              'uiAssets': {name: swf_names(archive.read(name)) for name in
                           ('swfs/house.swf', 'swfs/house_interstitials.swf', 'swfs/ui.swf')}}
    output.mkdir(parents=True, exist_ok=True)
    (output / 'inventory-raw.json').write_text(json.dumps(inventory, indent=2), encoding='utf-8')
    (output / 'probe-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    # Small, local references only. No game assets in the distributable prototype.
    for name in ('data/items/consumables.gon', 'data/items/armor_sets.gon', 'data/item_setbonuses.gon'):
        (output / Path(name).name).write_bytes(archive.read(name))
    print(json.dumps({k: report[k] for k in ('containers', 'itemCount', 'distinctItemTypes')}, indent=2))
    print(f"Reference bytes matched: {report['binary']['matched']}/{report['binary']['checked']}")


if __name__ == '__main__':
    main()
