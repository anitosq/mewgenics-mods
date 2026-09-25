"""Check generated SWF text against the installed game's actual font metrics.

Reads assets only. No game process, save edits, or copied game font output.
"""
import argparse
import json
from pathlib import Path
import struct
import zlib

from probe_inventory import Gpak


def unpack_swf(raw):
    assert raw[:3] in (b'FWS', b'CWS')
    data = raw[8:] if raw[:3] == b'FWS' else zlib.decompress(raw[8:])
    assert len(data) + 8 == struct.unpack_from('<I', raw, 4)[0]
    start = (5 + 4 * (data[0] >> 3) + 7) // 8 + 4
    result = []
    while start < len(data):
        header = struct.unpack_from('<H', data, start)[0]
        start += 2
        code, size = header >> 6, header & 63
        if size == 63:
            size = struct.unpack_from('<I', data, start)[0]
            start += 4
        assert start + size <= len(data)
        result.append((code, data[start:start + size]))
        start += size
    return result


def font_advances(tags):
    fonts, exports = {}, {}
    for code, body in tags:
        if code == 76:
            at = 2
            for _ in range(struct.unpack_from('<H', body)[0]):
                ident = struct.unpack_from('<H', body, at)[0]
                end = body.index(0, at + 2)
                exports[body[at + 2:end].decode()] = ident
                at = end + 1
        if code == 75:
            ident = struct.unpack_from('<H', body)[0]
            assert body[2] & 0x80, 'Font needs advance metrics'
            at = 5 + body[4]
            count = struct.unpack_from('<H', body, at)[0]
            at += 2
            offset = 'I' if body[2] & 8 else 'H'
            codes_at = at + struct.unpack_from('<' + offset, body, at + count * struct.calcsize(offset))[0]
            code_type = 'H' if body[2] & 4 else 'B'
            codes = struct.unpack_from('<' + code_type * count, body, codes_at)
            widths = struct.unpack_from('<' + 'h' * count, body, codes_at + count * struct.calcsize(code_type) + 6)
            fonts[ident] = dict(zip(codes, widths))
    return {name: fonts[ident] for name, ident in exports.items() if ident in fonts}


def bounds(body):
    pos = 16
    def read(n):
        nonlocal pos
        value = 0
        for _ in range(n):
            value = value * 2 + ((body[pos // 8] >> (7 - pos % 8)) & 1)
            pos += 1
        return value
    n = read(5)
    values = []
    for _ in range(4):
        value = read(n)
        values.append(value - (1 << n) if value & (1 << (n - 1)) else value)
    return values, (pos + 7) // 8


def validate(game, asset):
    archive = Gpak(game / 'resources.gpak')
    fonts = font_advances(unpack_swf(archive.read('swfs/fonts.swf')))
    validate_rarity_art(unpack_swf(archive.read('swfs/catparts.swf')))
    modlist = game / 'mods/modlist.txt'
    if modlist.exists():
        for name in modlist.read_text(encoding='utf-8-sig').splitlines():
            if not name.strip() or name.startswith('#'):
                continue
            override = game / 'mods' / name.strip() / 'swfs/catparts.swf'
            if override.is_file():
                validate_rarity_art(unpack_swf(override.read_bytes()))
    tags = unpack_swf(asset.read_bytes())
    assert not any(code in (10, 48, 75, 91) for code, _ in tags), 'Unexpected embedded font'
    assert (71, b'fonts.swf\0\1\0\0\0') in tags, 'Missing native font dependency'
    fields, failures = 0, []
    for code, body in tags:
        if code != 37:
            continue
        fields += 1
        box, at = bounds(body)
        flags = struct.unpack_from('>H', body, at)[0]
        at += 2
        assert flags & 128 and not flags & 256, 'Expected a font class reference'
        end = body.index(0, at)
        font = body[at:end].decode()
        assert font in fonts, f'Native font unavailable: {font}'
        size = struct.unpack_from('<H', body, end + 1)[0] / 20
        at = end + 3
        if flags & 1024:
            at += 4
        if flags & 512:
            at += 2
        if flags & 32:
            at += 9
        at = body.index(0, at) + 1
        text = body[at:].rstrip(b'\0').decode() if flags & 32768 else ''
        missing = [ch for ch in text if ord(ch) not in fonts[font]]
        advance = sum(fonts[font].get(ord(ch), 0) for ch in text) * size / 20480
        available = (box[1] - box[0]) / 20
        if missing or advance > available:
            failures.append({'text': text, 'advance': advance, 'available': available, 'missing': missing})
    assert fields > 0
    assert not failures, json.dumps(failures, indent=2)
    print(f'Native assets: {fields} text fields resolve game fonts; initial labels fit; no bundled fonts.')


def validate_rarity_art(tags):
    """Verify the native symbol/frame contract used by the runtime controls."""
    exports, sprites = {}, {}
    for code, body in tags:
        if code == 39:
            sprites[struct.unpack_from('<H', body)[0]] = body
        if code == 76:
            at = 2
            for _ in range(struct.unpack_from('<H', body)[0]):
                ident = struct.unpack_from('<H', body, at)[0]
                end = body.index(0, at + 2)
                exports[body[at + 2:end].decode()] = ident
                at = end + 1
    body = sprites[exports['HeadItemIcon']]
    at, frame, children = 4, 1, {}
    rarity = None
    while at < len(body):
        header = struct.unpack_from('<H', body, at)[0]
        at += 2
        code, size = header >> 6, header & 63
        if size == 63:
            size = struct.unpack_from('<I', body, at)[0]
            at += 4
        payload = body[at:at + size]
        at += size
        if code == 26 and payload[0] & 2:
            depth, ident = struct.unpack_from('<HH', payload, 1)
            children[depth] = ident
            if payload.endswith(b'rarity\0'):
                rarity = ident
        elif code == 28:
            children.pop(struct.unpack_from('<H', payload)[0], None)
        elif code == 1:
            if frame == 4:
                break
            frame += 1
    assert frame == 4 and set(children) == {1, 8}, 'Native blank icon frame changed'
    assert rarity == children[1], 'Native rarity child changed'
    for label in (b'common', b'uncommon', b'rare', b'very_rare'):
        assert label + b'\0' in sprites[rarity], f'Missing rarity label: {label}'
    print('Native rarity artwork: installed symbol, blank frame and four labels verified.')


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--game', type=Path, required=True)
    cli.add_argument('--asset', type=Path, default=Path(__file__).resolve().parents[1] / 'work/native-assets/swfs/improved_inventory.swf')
    args = cli.parse_args()
    validate(args.game, args.asset)
