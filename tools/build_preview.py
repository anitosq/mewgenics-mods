"""Build a local read-only browser from a saved inventory and installed resources.

This is a development harness, not an in-game mod or save editor. Item definition
merging is not emulated: metadata changed by active mods is marked uncertain.
"""
import argparse
from collections import Counter
import csv
from datetime import datetime
import io
import json
from pathlib import Path
import re
import shutil
from probe_inventory import Gpak, read_save


TOKEN = re.compile(r'\s+|//[^\r\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|[{}\[\],;]|[^\s{}\[\],;"/]+|/(?![/*])', re.S)


class Repeated(list):
    """Preserve duplicate fields without inventing their runtime precedence."""


def parse_gon(text):
    tokens = []
    pos = 0
    while pos < len(text):
        match = TOKEN.match(text, pos)
        if not match:
            raise ValueError(f'Unsupported GON token at {pos}')
        token = match.group()
        pos = match.end()
        if not (token.isspace() or token.startswith('//') or token.startswith('/*') or token in (',', ';')):
            tokens.append(token)
    index = 0

    def scalar(token):
        if token.startswith('"'):
            # Preserve unrecognized game escape sequences rather than interpreting them.
            return re.sub(r'\\(["\\])', r'\1', token[1:-1])
        return token

    def value():
        nonlocal index
        if index >= len(tokens):
            raise ValueError('Missing GON value')
        token = tokens[index]
        index += 1
        if token == '{':
            return obj('}')
        if token == '[':
            result = []
            while index < len(tokens) and tokens[index] != ']':
                result.append(value())
            if index == len(tokens):
                raise ValueError('Unclosed GON array')
            index += 1
            return result
        if token in ('}', ']'):
            raise ValueError('Unexpected GON closing delimiter')
        return scalar(token)

    def obj(end=None):
        nonlocal index
        result = {}
        while index < len(tokens) and tokens[index] != end:
            key = tokens[index]
            index += 1
            if key in '{}[]':
                raise ValueError('Invalid GON key')
            key = scalar(key)
            val = value()
            if key in result:
                if result[key] != val:
                    if not isinstance(result[key], Repeated):
                        result[key] = Repeated([result[key]])
                    result[key].append(val)
            else:
                result[key] = val
        if end:
            if index == len(tokens):
                raise ValueError('Unclosed GON object')
            index += 1
        return result

    return obj()


def normalized(definition):
    if any(isinstance(definition.get(k), Repeated) for k in ('kind', 'rarity', 'consumable', 'set')):
        raise ValueError('Ambiguous metadata field')
    memberships = definition.get('set', [])
    if isinstance(memberships, str):
        memberships = [memberships]
    return {'slot': definition.get('kind', 'unknown'),
            'rarity': definition.get('rarity', 'unknown').removeprefix('consumable_'),
            'category': 'consumable' if definition.get('consumable') == 'true' else 'equipment',
            'setIds': memberships}


def build(game, save, output):
    for forbidden in (game.resolve(), save.resolve().parent):
        if output.resolve() == forbidden or forbidden in output.resolve().parents:
            raise ValueError('Output must be outside game/save folders')
    inventory, blobs = read_save(save)
    archive = Gpak(game / 'resources.gpak')
    localization = {row['KEY']: row['en'] for row in csv.DictReader(io.StringIO(
        archive.read('data/text/combined.csv').decode('utf-8-sig')))}
    definitions, failures = {}, []
    for path in archive.entries:
        if path.startswith('data/items/') and path.endswith('.gon'):
            try:
                parsed = parse_gon(archive.read(path).decode('utf-8-sig'))
                for key, value in parsed.items():
                    if key in definitions and definitions[key] != value:
                        raise ValueError(f'Duplicate item across files: {key}')
                    definitions[key] = value
            except ValueError as error:
                failures.append(f'{path}: {error}')
    sets = parse_gon(archive.read('data/item_setbonuses.gon').decode('utf-8-sig'))

    def resolve(name, visited=()):
        if name in visited:
            raise ValueError('Cyclic item variant')
        current = definitions.get(name)
        if not isinstance(current, dict):
            raise ValueError(f'Unknown item definition: {name}')
        if 'variant_of' in current and not isinstance(current['variant_of'], str):
            raise ValueError('Ambiguous item variant')
        parent = resolve(current['variant_of'], (*visited, name)) if 'variant_of' in current else {}
        return {**parent, **current}

    # Flag relevant overrides rather than claiming to reproduce the engine loader.
    modified = set()
    active = (game / 'mods/modlist.txt').read_text(encoding='utf-8-sig').splitlines()
    metadata_keys = {'rarity', 'kind', 'consumable', 'set', 'variant_of'}
    for name in active:
        if not name.strip() or name.startswith('#'):
            continue
        root = game / 'mods' / name.strip()
        for path in root.rglob('*'):
            relative = path.relative_to(root).as_posix().lower()
            if not path.is_file() or not relative.startswith('data/items/') or '.gon' not in path.name:
                continue
            try:
                mod = parse_gon(path.read_text(encoding='utf-8-sig'))
                for key, fields in mod.items():
                    if not isinstance(fields, dict):
                        continue
                    for field, val in fields.items():
                        basefield = field.split('.')[0]
                        if basefield in metadata_keys and resolve(key).get(basefield) != val:
                            modified.add(key)
            except (ValueError, UnicodeError) as error:
                # Unknown files might alter any item; fail closed for the preview.
                failures.append(f'Active mod metadata not resolved: {name}/{relative}: {error}')
                modified.update(definitions)
    for item in inventory:
        try:
            definition = resolve(item['itemId'])
            item.update(normalized(definition))
            key = definition.get('name', item['itemId'])
            item['name'] = localization.get(key, key)
            item['metadataStatus'] = 'base metadata; no explicit override detected'
        except ValueError:
            item.update(name=item['itemId'], slot='unknown', rarity='unknown', category='unknown', setIds=[])
            item['metadataStatus'] = 'unknown definition'
        if item['itemId'] in modified:
            item.update(slot='unknown', rarity='unknown', category='unknown', setIds=[])
            item['metadataStatus'] = 'mod override requires runtime verification'
        item['sets'] = [dict(id=s, name=localization.get(sets.get(s, {}).get('name', s), s)) for s in item['setIds']]
        item['setsKnown'] = item['metadataStatus'] == 'base metadata; no explicit override detected'
    data = {'source': save.name, 'savedAt': datetime.fromtimestamp(save.stat().st_mtime).isoformat(timespec='seconds'),
            'generatedAt': datetime.now().isoformat(timespec='seconds'),
            'mode': 'saved snapshot; not live game state', 'items': inventory,
            'containerCounts': {c: sum(i['container'] == c for i in inventory) for c in ('storage', 'backpack', 'trash')},
            'limitations': ['Condition flags are not decoded; condition remains unknown.',
                            'Mod replacement/deletion and runtime merge semantics are not fully emulated.',
                            'Metadata altered by mods is withheld pending runtime verification.',
                            'Names use the installed base English localization; numeric mod prefixes are omitted.',
                            'Instance IDs are snapshot-local; not persistent identities.'],
            'metadataFailures': failures, 'blobChecksums': blobs}
    output.mkdir(parents=True, exist_ok=True)
    for path in (Path(__file__).resolve().parents[1] / 'mods/improved-inventory/src/browser').iterdir():
        if path.is_file():
            shutil.copyfile(path, output / path.name)
    (output / 'inventory.json').write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps({'items': len(inventory), 'categories': dict(Counter(i['category'] for i in inventory)),
                      'uncertain': sum(not i['setsKnown'] for i in inventory), 'parseFailures': failures}, indent=2))


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--game', required=True, type=Path)
    cli.add_argument('--save', required=True, type=Path)
    cli.add_argument('--output', required=True, type=Path)
    args = cli.parse_args()
    build(args.game, args.save, args.output)
