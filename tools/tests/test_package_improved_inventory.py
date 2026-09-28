"""Guard the distinct discovery layouts and identical runtime payloads."""
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from package_improved_inventory import package_variants, write_archive


class PackageVariantsTests(unittest.TestCase):
    def test_only_dll_location_changes(self):
        files = {
            'ImprovedInventory.dll': b'native build',
            'ImprovedInventory/description.json': b'{}',
            'ImprovedInventory/swfs/improved_inventory.swf': b'UI build',
        }
        variants = package_variants(files)
        with tempfile.TemporaryDirectory() as temp:
            for suffix, payload in variants.items():
                archive = Path(temp) / f'candidate{suffix}.zip'
                write_archive(archive, payload)
                with zipfile.ZipFile(archive) as packed:
                    dlls = [n for n in packed.namelist() if n.endswith('.dll')]
                    self.assertEqual(dlls, [
                        'ImprovedInventory/ImprovedInventory.dll' if suffix else 'ImprovedInventory.dll'
                    ])
                    self.assertEqual(packed.read(dlls[0]), files['ImprovedInventory.dll'])
                    for name, data in files.items():
                        if not name.endswith('.dll'):
                            self.assertEqual(packed.read(name), data)
        self.assertIn('ImprovedInventory.dll', files)
        self.assertNotIn('ImprovedInventory/ImprovedInventory.dll', files)


if __name__ == '__main__':
    unittest.main()
