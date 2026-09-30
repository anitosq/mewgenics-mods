"""Run offline guards and fixture checks, plus the compiled native snapshot reader."""
import ctypes
from contextlib import closing
from pathlib import Path
import sqlite3
import struct
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import assets
from session import MARKER_ID, ROOT, decode, fixture, loader_config, records


def blob(room="Attic", variant=2):
    name = b"set_80s_table"
    raw_room = room.encode()
    return (struct.pack("<IQ", 1, len(name)) + name + struct.pack("<QQ", variant, len(raw_room)) +
            raw_room + struct.pack("<5i", -3, 4, 900123, -1, 1))


class Checks(unittest.TestCase):
    def test_release_asset_gate(self):
        dll = ctypes.CDLL(str(ROOT / "build/AutoFurniture.dll"))
        gate = dll.AutoFurnitureValidateAssetsW
        gate.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p]
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory) / "External mods with spaces and \u00f1"
            assets_dir = base / "AutoFurniture"
            shutil.copytree(ROOT / "build/data-mod", assets_dir)
            command = subprocess.list2cmdline(["Mewgenics.exe", "-modpaths", str(assets_dir)])
            for module in (base / "AutoFurniture.dll", assets_dir / "AutoFurniture.dll"):
                self.assertEqual(gate(str(module), command), 1)
                self.assertEqual(gate(str(module), "Mewgenics.exe"), 0)
                self.assertEqual(gate(str(module), command + "-disabled"), 0)
            module = str(base / "AutoFurniture.dll")
            self.assertEqual(gate(module, subprocess.list2cmdline(["Mewgenics.exe", "-modpaths", str(base), str(assets_dir)])), 0)
            for name in ("auto_furniture.swf", "swflist.gon.append"):
                file = assets_dir / "swfs" / name
                original = file.read_bytes()
                file.write_bytes(original + b"mismatch")
                self.assertEqual(gate(module, command), 0)
                file.unlink()
                self.assertEqual(gate(module, command), 0)
                file.write_bytes(original)
            self.assertEqual(gate(module, command), 1)

    def test_ui_button_accents_and_wand_inset(self):
        movie = assets.Movie()
        with tempfile.TemporaryDirectory() as directory, \
             patch.object(assets, "__file__", str(Path(directory) / "assets.py")), \
             patch.object(assets, "Movie", return_value=movie), \
             patch.object(movie, "paper", wraps=movie.paper) as paper, \
             patch.object(movie, "polygon", wraps=movie.polygon) as polygon, \
             patch.object(movie, "field", wraps=movie.field) as field:
            assets.build()
            paper.assert_any_call(18, 370, 132, 38, fill=assets.CALCULATE_FILL)
            paper.assert_any_call(162, 370, 132, 38, fill=assets.APPLY_FILL)
            paper.assert_any_call(0, 0, 32, 32)
            labels = [call.args[5] for call in field.call_args_list if len(call.args) > 5]
            self.assertIn("Calculate", labels)
            self.assertNotIn("Preview", labels)
            for i in range(5):
                field.assert_any_call(118+i*94, 476, 72, 28, 18, align=0)
            # Shade and three paper polygons precede the four wand polygons.
            for call in polygon.call_args_list[4:8]:
                for x, y in call.args[0]:
                    self.assertTrue(5 <= x <= 27 and 5 <= y <= 27)

    def test_fixture_preserves_all_owned_records(self):
        with tempfile.TemporaryDirectory() as directory:
            src, dst = (Path(directory) / n for n in ("source.sav", "fixture.sav"))
            items = {1: blob(), 2: blob("", 0)}
            with closing(sqlite3.connect(src)) as db, db:
                db.execute("CREATE TABLE furniture(key INTEGER PRIMARY KEY, data BLOB) STRICT")
                db.executemany("INSERT INTO furniture VALUES (?,?)", items.items())
            original = src.read_bytes()
            fixture(src, dst)
            self.assertEqual(src.read_bytes(), original)
            self.assertEqual(records(dst), {**items, MARKER_ID: items[2]})
            with self.assertRaises(ValueError):
                fixture(src, dst)

    def test_parser_rejects_truncation_and_unknown_tail(self):
        self.assertEqual(decode(blob())[2:4], ("Attic", (-3, 4, 900123, -1, 1)))
        for data in (blob()[:-1], blob() + b"?", b"\x02\x00\x00\x00"):
            with self.assertRaises((ValueError, struct.error)):
                decode(data)

    def test_loader_does_not_replace_existing_mods(self):
        original = b"[Chainloader]\r\nEnabled=1\r\n[LoadOrder]\r\nMod1=Existing.dll\r\n[Other]\r\nx=2\r\n"
        result = loader_config(original, Path("Probe.dll"))
        self.assertIn(b"Mod1=Existing.dll\r\n", result)
        self.assertIn(b"Mod2=Probe.dll\r\n[Other]", result)
        with self.assertRaises(ValueError):
            loader_config(b"No section", Path("Probe.dll"))

    def test_native_snapshot_guards(self):
        for name in ("AutoFurnitureProbe", "AutoFurniture"):
            dll = ctypes.CDLL(str(ROOT / f"build/{name}.dll"))
            dll.AutoFurnitureProbeSelfTest.restype = ctypes.c_int
            self.assertEqual(dll.AutoFurnitureProbeSelfTest(), 0, name)


if __name__ == "__main__":
    unittest.main()
