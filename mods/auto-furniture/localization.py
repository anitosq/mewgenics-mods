"""Validate the editable game CSV and emit English keys for native UI lookup."""
import csv
import json
import re
from pathlib import Path

SOURCE = Path(__file__).with_name('translations.csv')
LANGUAGES = ('en', 'sp', 'fr', 'de', 'it', 'pt-br', 'ru', 'ko', 'ja', 'zh-cn')
TEXT_BYTES = 512


def read_catalog(path=SOURCE):
    with path.open(encoding='utf-8-sig', newline='') as stream:
        next(stream)  # The game's append files reserve the first row.
        reader = csv.DictReader(stream)
        if reader.fieldnames != ['KEY', 'en', 'notes', *LANGUAGES[1:]]:
            raise ValueError('Unexpected game language columns')
        rows = list(reader)
    if not rows:
        raise ValueError('Empty translation catalog')
    keys = set()
    for row in rows:
        key = row['KEY']
        if not key or not re.fullmatch(r'AUTO_FURNITURE_[A-Z0-9_]+', key) or key in keys:
            raise ValueError(f'Invalid or duplicate key: {key}')
        keys.add(key)
        if None in row or any(value is None for value in row.values()) or not row['en']:
            raise ValueError(f'Malformed row: {key}')
        placeholders = sorted(re.findall(r'%[12]', row['en']))
        for language in LANGUAGES:
            text = row[language]
            if len(text.encode('utf-8')) >= TEXT_BYTES or any(c in text for c in '\0\r\n'):
                raise ValueError(f'Invalid or oversized text: {key}/{language}')
            if text and (sorted(re.findall(r'%[12]', text)) != placeholders or
                         '%' in re.sub(r'%[12]', '', text) or '[' in text or ']' in text):
                raise ValueError(f'Invalid placeholders or markup: {key}/{language}')
    return rows


def build(output):
    rows = read_catalog()
    text_dir = output / 'data-mod/data/text'
    text_dir.mkdir(parents=True, exist_ok=True)
    # Fill untranslated languages with English before handing the table to the game.
    with (text_dir / 'combined.csv.append').open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow([f'field{i}' for i in range(1, 13)])
        writer.writerow(['KEY', 'en', 'notes', *LANGUAGES[1:]])
        for row in rows:
            writer.writerow([row['KEY'], row['en'], row['notes'],
                             *(row[language] or row['en'] for language in LANGUAGES[1:])])
    entries = ',\n'.join('{' + json.dumps(row['KEY']) + ',' + json.dumps(row['en']) + '}' for row in rows)
    names = ','.join(row['KEY'].replace('AUTO_FURNITURE_', 'AF_') for row in rows)
    (output / 'translations.h').write_text(
        f'#define AF_TEXT_BYTES {TEXT_BYTES}\nenum {{{names},AF_TEXT_COUNT}};\n'
        'static const struct {const char* key;const char* english;} af_catalog[]={\n' + entries + '\n};\n',
        encoding='ascii')
