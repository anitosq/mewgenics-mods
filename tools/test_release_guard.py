"""Check the release DLL in a separate non-game process, without installing hooks."""
import ctypes
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    dll = ctypes.WinDLL(str(ROOT / 'work/release-build/InventoryQoL.dll'))
    assert dll.InventoryQoLBuildKind() == 1
    dll.InventoryQoLVersion.restype = ctypes.c_char_p
    assert dll.InventoryQoLVersion().decode() == (ROOT / 'mods/inventory-qol/VERSION').read_text().strip()
    image = dll.InventoryProbeValidateImageW
    image.argtypes = [ctypes.c_wchar_p]
    assert image(str(Path(sys.argv[1]) / 'Mewgenics.exe')) == 1
    assert image(sys.executable) == 0
    assert image(str(ROOT / 'work/nonexistent.exe')) == 0
    gate = dll.InventoryQoLValidateAssetsW
    gate.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p]
    with tempfile.TemporaryDirectory(prefix='iq-release-') as temp:
        root = Path(temp) / 'Path with spaces and ñ'
        assets = root / 'InventoryQoL'
        shutil.copytree(ROOT / 'work/native-assets', assets)
        module = str(root / 'InventoryQoL.dll')

        def check(args, expected):
            assert gate(module, subprocess.list2cmdline(['Mewgenics.exe', *args])) == expected, args

        check(['-modpaths', str(assets)], 1)
        check(['-modpaths', str(assets).upper()], 1)
        check(['-modpaths', str(assets).replace('\\', '/') + '/'], 1)
        check(['-modpaths', str(root / 'Other'), str(assets), '-debug'], 1)
        check([], 0)
        check([str(assets)], 0)
        check(['-modpaths', str(assets) + '-disabled'], 0)
        check(['-modpaths', '-another-option', str(assets)], 0)
        check(['-modpaths', str(root / 'Other/InventoryQoL')], 0)
        for name in ('inventory_qol.swf', 'swflist.gon.append'):
            path = assets / 'swfs' / name
            original = path.read_bytes()
            path.write_bytes(original + b'changed')
            check(['-modpaths', str(assets)], 0)
            path.unlink()
            check(['-modpaths', str(assets)], 0)
            path.write_bytes(original)
        check(['-modpaths', str(assets)], 1)
    print('Release identity, executable guard, enabled paths and missing/mismatched assets passed.')


if __name__ == '__main__':
    main()
