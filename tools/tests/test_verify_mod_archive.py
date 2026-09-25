import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import warnings
import zipfile


spec = importlib.util.spec_from_file_location(
    'verify_mod_archive', Path(__file__).resolve().parents[1] / 'verify_mod_archive.py')
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)


class ArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.archive = self.root / 'download-renamed-by-host.zip'
        self.manifest = self.root / 'release.manifest.json'
        self.files = {'Example.dll': b'fixture dll',
                      'Example/description.json': b'{"version":"0.1.0"}'}
        self.write_zip(self.files)
        self.data = {'archive': 'Example-0.1.0.zip', 'version': '0.1.0',
                     'sha256': hashlib.sha256(self.archive.read_bytes()).hexdigest(),
                     'files': {name: hashlib.sha256(data).hexdigest()
                               for name, data in self.files.items()}}
        self.write_manifest()

    def write_zip(self, files, comment=b''):
        with zipfile.ZipFile(self.archive, 'w') as archive:
            for name, data in files.items():
                archive.writestr(name, data)
            archive.comment = comment

    def write_manifest(self):
        self.manifest.write_text(json.dumps(self.data), encoding='utf-8')

    def verify(self, allow_repacked=False):
        return verifier.verify_archive(self.manifest, self.archive, allow_repacked)

    def test_original_bytes_with_renamed_download(self):
        result = self.verify()
        self.assertTrue(result['archive_matches'])
        self.assertTrue(result['payload_matches'])
        self.assertEqual(result['file_count'], 2)

    def test_repacked_requires_explicit_option(self):
        self.write_zip(self.files, comment=b'hosting metadata')
        with self.assertRaisesRegex(ValueError, 'ZIP hash differs'):
            self.verify()
        self.assertFalse(self.verify(True)['archive_matches'])

    def test_changed_payload_rejected_even_with_repack_option(self):
        self.write_zip({**self.files, 'Example.dll': b'wrong build'})
        with self.assertRaisesRegex(ValueError, 'Payload hash differs'):
            self.verify(True)

    def test_missing_payload(self):
        self.write_zip({'Example.dll': self.files['Example.dll']})
        with self.assertRaisesRegex(ValueError, 'missing='):
            self.verify(True)

    def test_extra_payload(self):
        self.write_zip({**self.files, 'Unexpected.dll': b'extra'})
        with self.assertRaisesRegex(ValueError, 'extra='):
            self.verify(True)

    def test_duplicate_entry(self):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            with zipfile.ZipFile(self.archive, 'a') as archive:
                archive.writestr('Example.dll', b'duplicate')
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.verify(True)

    def test_invalid_paths(self):
        for name in ('../Example.dll', 'C:/Example.dll', '/Example.dll',
                     'Example//file'):
            with self.subTest(name=name):
                self.write_zip({name: b'bad path'})
                with self.assertRaisesRegex(ValueError, 'invalid payload path'):
                    self.verify(True)

    def test_manifest_backslash_path(self):
        # ZipFile normalizes Windows separators when writing a fixture.
        # Check the manifest path directly instead.
        self.data['files'] = {'Example\\file': hashlib.sha256(b'data').hexdigest()}
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, 'invalid payload path'):
            self.verify()

    def test_symlink(self):
        with zipfile.ZipFile(self.archive, 'a') as archive:
            entry = zipfile.ZipInfo('link')
            entry.external_attr = 0o120777 << 16
            archive.writestr(entry, b'Example.dll')
        with self.assertRaisesRegex(ValueError, 'symbolic link'):
            self.verify(True)

    def test_invalid_manifest(self):
        self.data['sha256'] = 'not-a-hash'
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, 'SHA-256'):
            self.verify()

    def test_invalid_zip(self):
        self.archive.write_bytes(b'not a zip')
        with self.assertRaises(zipfile.BadZipFile):
            self.verify()


if __name__ == '__main__':
    unittest.main()
