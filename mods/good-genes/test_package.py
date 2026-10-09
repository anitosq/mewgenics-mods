"""Deferred packaging regression checks. Does not build, install or load a DLL."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from build import EXPECTED, OUTPUT_FILES, SOURCE_FILES
from package import verified_outputs


class BuildProvenanceTests(unittest.TestCase):
    def test_current_and_stale_builds(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "mods/good-genes"
            root.mkdir(parents=True)
            for name in SOURCE_FILES:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(name.encode())
            outputs = {path: path.encode() for path in OUTPUT_FILES.values()}
            for name, data in outputs.items():
                path = root / "build" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            info = {"version": "candidate", "source_commit": "commit", "game_sha256": EXPECTED,
                    "source_sha256": {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in SOURCE_FILES},
                    **{key: hashlib.sha256(outputs[path]).hexdigest() for key, path in OUTPUT_FILES.items()}}
            manifest = root / "build/build-info.json"
            manifest.write_text(json.dumps(info))
            self.assertEqual(verified_outputs(root, "candidate", "commit"), outputs)
            for key in ("version", "source_commit", "game_sha256", "source_sha256", *OUTPUT_FILES):
                broken = dict(info)
                broken.pop(key)
                manifest.write_text(json.dumps(broken))
                with self.subTest(missing=key), self.assertRaises(SystemExit):
                    verified_outputs(root, "candidate", "commit")
            manifest.write_text(json.dumps(info))
            for name in (*SOURCE_FILES, *("build/"+name for name in outputs)):
                path = root / name
                original = path.read_bytes()
                path.write_bytes(original + b"changed")
                with self.subTest(changed=name), self.assertRaises(SystemExit):
                    verified_outputs(root, "candidate", "commit")
                path.write_bytes(original)
            manifest.unlink()
            with self.assertRaises(SystemExit):
                verified_outputs(root, "candidate", "commit")


if __name__ == "__main__":
    unittest.main()
