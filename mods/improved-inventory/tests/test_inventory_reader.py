import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from probe_inventory import parse_inventory
from build_preview import parse_gon, normalized, Repeated


def record(name='CatFood', charges=2, sequence=0):
    text = name.encode()
    return b'\x01' + struct.pack('<II', len(text), 0) + text + struct.pack('<IIiIII', 0, 0, charges, 0, 4, sequence) + b'\xff'


def container(*records):
    if not records:
        return struct.pack('<I', 0)
    return struct.pack('<II', len(records), 5) + b'\x00\x05\x00\x00\x00'.join(records) + b'\x00'


class InventoryReaderTests(unittest.TestCase):
    def test_counts_and_duplicate_identity(self):
        result = parse_inventory(container(record(), record(sequence=0)), 'storage')
        self.assertEqual(len(result), 2)
        self.assertNotEqual(result[0]['instanceId'], result[1]['instanceId'])
        self.assertEqual(result[0]['condition'], 'unknown')

    def test_signed_charges_and_subname(self):
        result = parse_inventory(container(record(charges=-1)), 'trash')[0]
        self.assertEqual(result['charges'], -1)
        self.assertEqual(result['subname'], '')
        self.assertEqual(result['raw']['field2'], 4)

    def test_empty(self):
        self.assertEqual(parse_inventory(container(), 'storage'), [])

    def test_rejects_every_truncation(self):
        blob = container(record(), record('LimitedUseWeapon'))
        for end in range(len(blob)):
            with self.subTest(end=end), self.assertRaises(ValueError):
                parse_inventory(blob[:end], 'storage')

    def test_rejects_unknown_versions_and_trailing_data(self):
        blob = container(record())
        for invalid in (blob + b'\x00', blob[:4] + struct.pack('<I', 6) + blob[8:]):
            with self.assertRaises(ValueError):
                parse_inventory(invalid, 'storage')


class GonProjectionTests(unittest.TestCase):
    def test_multi_set_and_comments(self):
        item = parse_gon('Thing { /* note */ kind head set [Bone, Meat] name "a // b" }')['Thing']
        self.assertEqual(normalized(item)['setIds'], ['Bone', 'Meat'])
        self.assertEqual(item['name'], 'a // b')

    def test_charges_do_not_imply_consumable(self):
        self.assertEqual(normalized({'kind':'weapon','durability':'3'})['category'], 'equipment')
        self.assertEqual(normalized({'kind':'trinket','consumable':'true','rarity':'consumable_common'})['rarity'], 'common')

    def test_duplicate_metadata_not_guessed(self):
        parsed = parse_gon('X { set Bone set Meat }')['X']
        self.assertIsInstance(parsed['set'], Repeated)
        with self.assertRaises(ValueError):
            normalized(parsed)

    def test_rejects_unclosed_input(self):
        for text in ('X { set [Bone', 'X { kind head', 'X'):
            with self.assertRaises(ValueError):
                parse_gon(text)


if __name__ == '__main__':
    unittest.main()
