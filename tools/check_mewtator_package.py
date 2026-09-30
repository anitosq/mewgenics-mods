"""Check a frozen Mewtator archive with an explicitly supplied upstream checkout.

Uses a temporary game directory and external mods folder. Never edits the real
game, launches it, or changes Mewtator's settings. Windows only (loads our DLL).
"""
import argparse
import ctypes
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import zipfile

from verify_mod_archive import verify_archive


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--source', required=True, type=Path)
    cli.add_argument('--archive', required=True, type=Path)
    cli.add_argument('--manifest', required=True, type=Path)
    cli.add_argument('--mod-name', default='ImprovedInventory')
    args = cli.parse_args()
    # Verify before extraction/loading; manifest must come from our trusted build.
    verify_archive(args.manifest, args.archive)
    sys.path.insert(0, str(args.source.resolve()))
    from app.core.services.dll_injection_service import DllInjectionService
    revision = subprocess.check_output(['git', '-C', str(args.source), 'rev-parse', 'HEAD'], text=True).strip()
    service = DllInjectionService()
    with tempfile.TemporaryDirectory(prefix='improved-inventory-mewtator-') as temp:
        root = Path(temp)
        game = root / 'Game'
        game.mkdir()
        mods = root / 'External mods with spaces and ñ'
        with zipfile.ZipFile(args.archive) as archive:
            archive.extractall(mods)
        assets = mods / args.mod_name
        module = assets / f'{args.mod_name}.dll'
        mod = SimpleNamespace(name=args.mod_name, path=str(assets), dll_order=None)
        enabled = SimpleNamespace(enabled_mods=[mod])
        disabled = SimpleNamespace(enabled_mods=[])
        found = service.scan_for_dll_mods(enabled)
        assert found == [(args.mod_name, [module.absolute()])], found
        assert service.scan_for_dll_mods(disabled) == []
        ini = game / 'chainloader.ini'
        original = '[Chainloader]\nScanPath=mods\nMewtatorManifest=\n; retained\n'
        ini.write_text(original, encoding='utf-8')
        assert service.update_chainloader_manifest(str(game), str(mods), found)
        manifest = mods / service.MANIFEST_FILE
        assert manifest.read_text(encoding='utf-8').splitlines() == [str(module.absolute())]
        assert f'MewtatorManifest={manifest}' in ini.read_text(encoding='utf-8')
        assert 'ScanPath=mods' in ini.read_text(encoding='utf-8')
        dll = ctypes.WinDLL(str(module))
        try:
            gate = getattr(dll, f'{args.mod_name}ValidateAssetsW')
            gate.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p]
            command = subprocess.list2cmdline(['Mewgenics.exe', '-modpaths', str(assets)])
            assert gate(str(module), command) == 1
            assert gate(str(module), 'Mewgenics.exe') == 0
        finally:
            ctypes.windll.kernel32.FreeLibrary.argtypes = [ctypes.c_void_p]
            ctypes.windll.kernel32.FreeLibrary(dll._handle)
        assert service.clear_chainloader_manifest(str(game), str(mods))
        assert not manifest.exists()
        assert ini.read_text(encoding='utf-8') == original
    print(f'Mewtator {revision}: external discovery, manifest, enabled/disabled assets and cleanup passed.')


if __name__ == '__main__':
    main()
