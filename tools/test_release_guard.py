"""Check the release DLL in a separate non-game process, without installing hooks."""
import ctypes
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    dll = ctypes.WinDLL(str(ROOT / 'work/release-build/ImprovedInventory.dll'))
    assert dll.ImprovedInventoryBuildKind() == 1
    dll.ImprovedInventoryVersion.restype = ctypes.c_char_p
    assert dll.ImprovedInventoryVersion().decode() == (ROOT / 'mods/improved-inventory/VERSION').read_text().strip()
    image = dll.InventoryProbeValidateImageW
    image.argtypes = [ctypes.c_wchar_p]
    assert image(str(Path(sys.argv[1]) / 'Mewgenics.exe')) == 1
    assert image(sys.executable) == 0
    assert image(str(ROOT / 'work/nonexistent.exe')) == 0
    gate = dll.ImprovedInventoryValidateAssetsW
    gate.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p]
    status = dll.ImprovedInventoryAssetStatusW
    status.argtypes = gate.argtypes
    assert status(None, 'Mewgenics.exe') == 0
    assert status('C:\\ImprovedInventory.dll', None) == 0
    with tempfile.TemporaryDirectory(prefix='iq-release-') as temp:
        root = Path(temp) / 'Path with spaces and ñ'
        assets = root / 'ImprovedInventory'
        shutil.copytree(ROOT / 'work/native-assets', assets)
        for folder in (root, assets):
            module = str(folder / 'ImprovedInventory.dll')

            def check(args, expected):
                command = subprocess.list2cmdline(['Mewgenics.exe', *args])
                assert status(module, command) == expected, (module, args, expected)
                assert gate(module, command) == (expected == 1), (module, args)

            check(['-modpaths', str(assets)], 1)
            check(['-modpaths', str(assets).upper()], 1)
            check(['-modpaths', str(assets).replace('\\', '/') + '/'], 1)
            check(['-modpaths', str(root / 'Other'), str(assets), '-debug'], 1)
            check(['-modpaths', str(assets), str(assets)], 1)
            check([], 2)
            check([str(assets)], 2)
            check(['-modpaths', str(assets) + '-disabled'], 2)
            check(['-modpaths', '-another-option', str(assets)], 2)
            check(['-modpaths', str(root / 'Other/ImprovedInventory')], 2)
            check(['-modpaths', str(folder), str(folder / 'ImprovedInventory')], 4)
            for name in ('improved_inventory.swf', 'swflist.gon.append'):
                path = assets / 'swfs' / name
                original = path.read_bytes()
                path.write_bytes(original + b'changed')
                check(['-modpaths', str(assets)], 3)
                path.unlink()
                check(['-modpaths', str(assets)], 3)
                path.write_bytes(original)
            check(['-modpaths', str(assets)], 1)
    print('Release identity, executable guard, both layouts, disabled/ambiguous paths and missing/mismatched assets passed.')


if __name__ == '__main__':
    main()
