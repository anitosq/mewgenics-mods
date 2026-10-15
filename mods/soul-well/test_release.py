"""Check source data and frozen installation layouts without launching the game."""
import argparse
import copy
import ctypes
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

from build import ROOT, EXPECTED, ITEM, ABILITY, SHOP

sys.path.insert(0, str(ROOT.parents[1] / 'tools'))
from build_preview import parse_gon
from check_boss_loot import merge
from probe_inventory import Gpak
from verify_mod_archive import verify_archive


def data_check(game):
    assert hashlib.sha256((game / 'Mewgenics.exe').read_bytes()).hexdigest() == EXPECTED
    archive = Gpak(game / 'resources.gpak')
    item = parse_gon((ROOT / ITEM).read_text())
    ability = parse_gon((ROOT / ABILITY).read_text())
    assert set(item) == {'SoulWell'} and item['SoulWell']['ability'] == 'tk_SoulWell'
    assert item['SoulWell']['aux'] == '-1'
    assert set(ability) == {'tk_SoulWell'}
    tree = ability['tk_SoulWell']
    assert tree['template'] == 'spawn' and tree['graphics']['lob'] == 'false'
    assert tree['spawn']['catdata'] == 'item_aux_catid'
    assert tree['spawn']['object'] == 'PlayerCat_Shade'
    assert tree['spawn']['redirect_location_if_blocked'] == 'true'
    assert tree['cost']['once_per_fight'] == 'true'
    for name, overlay in ((ITEM, item), (ABILITY, ability)):
        before = parse_gon(archive.read(name.removesuffix('.merge')).decode('utf-8-sig'))
        assert not set(overlay).intersection(before)
        after = copy.deepcopy(before); merge(after, overlay)
        for key in overlay: after.pop(key)
        assert after == before
    original = parse_gon(archive.read(SHOP.removesuffix('.patch')).decode('utf-8-sig'))
    patched = copy.deepcopy(original)
    merge(patched, parse_gon((ROOT / SHOP).read_text()), True)
    for name in ('TracyHouseShop', 'TracyHouseShopSpecialDay'):
        offer = patched[name]['item_groups']['consumable']['Item']
        assert offer['pool'] == ['SoulWell'] and offer['cost'] == '1' and offer['allow_duplicates'] == 'true'
        for field in ('breakdown', 'breakdown2', 'breakdown3', 'breakdown4'):
            assert int(patched[name][field]['consumable']) >= 1
        patched[name]['item_groups']['consumable']['Item'] = copy.deepcopy(original[name]['item_groups']['consumable']['Item'])
    assert patched == original, 'Unrelated shop fields changed'
    print('PASS: new item/ability only; marker-free item; Tracy offer in all eight stock tiers; other data preserved.')


def gate_check(module, assets, other_module):
    dll = ctypes.WinDLL(str(module))
    try:
        gate = dll.SoulWellValidateAssetsW
        gate.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p]
        gate.restype = ctypes.c_int
        status = dll.SoulWellAssetStatusW
        status.argtypes = gate.argtypes
        status.restype = ctypes.c_int
        def check(command, expected, path=module, reason=None):
            assert gate(str(path), subprocess.list2cmdline(command)) == expected, command
            if reason is not None:
                assert status(str(path), subprocess.list2cmdline(command)) == reason, command
        enabled = ['Mewgenics.exe', '-modpaths', str(assets)]
        check(enabled, 1)
        check(enabled, 1, other_module)
        check(['Mewgenics.exe'], 0, reason=2)
        check(['Mewgenics.exe', '-modpaths', str(assets) + '-disabled'], 0)
        check(['Mewgenics.exe', '-other', str(assets)], 0)
        check(['Mewgenics.exe', '-modpaths', str(assets), str(assets.parent)], 0, assets.parent / 'SoulWell.dll', reason=3)
        for relative, error in ((ITEM, 4), (SHOP, 5), (ABILITY, 6)):
            path = assets / relative; original = path.read_bytes()
            path.unlink(); check(enabled, 0, reason=error); path.write_bytes(original)
            path.write_bytes(b'changed'); check(enabled, 0, reason=error); path.write_bytes(original)
            formatted = '// formatting only\n' + original.decode().replace('\r\n', '\n').replace('{\n', '{\n\t')
            path.write_bytes(b'\xef\xbb\xbf' + formatted.replace('\n', '\r\n').encode()); check(enabled, 1, reason=1)
            path.write_bytes(original)
        path = assets / ABILITY; original = path.read_bytes()
        custom = original.decode().replace('InjuryImmunity 1', 'InjuryImmunity 2\n            // User edit: } is harmless in a comment').replace('max_range 1', 'max_range 10')
        assert parse_gon(custom)['tk_SoulWell']['spawn']['additional_passives']['InjuryImmunity'] == '2'
        path.write_text(custom, encoding='utf-8'); check(enabled, 1)
        path.write_bytes(b'\xef\xbb\xbf' + custom.replace('\r\n', '\n').replace('\n', '\r\n').encode()); check(enabled, 1)
        path.write_text(custom.replace('lob false', 'lob true')); check(enabled, 0)
        path.write_text(custom.replace('item_aux_catid', 'other')); check(enabled, 0)
        for value in ('0', '-10', '1.5', '2147483648', '10 max_range 1'):
            path.write_text(custom.replace('max_range 10', 'max_range ' + value)); check(enabled, 0, reason=6)
        path.write_text(custom + '\n tk_SoulWell { target { max_aoe 5 } }'); check(enabled, 0, reason=6)
        path.write_bytes(original)
    finally:
        ctypes.windll.kernel32.FreeLibrary.argtypes = [ctypes.c_void_p]
        ctypes.windll.kernel32.FreeLibrary(dll._handle)
    print('PASS: DLL layouts, exact paths, specific failure reasons, range 10/passives, formatting/CRLF/BOM, invalid range/core override rejection.')


def package_check(archive, manifest):
    verify_archive(manifest, archive)
    with tempfile.TemporaryDirectory(prefix='soul-well-') as temp:
        root = Path(temp) / 'Mods with spaces and ñ'; root.mkdir()
        with zipfile.ZipFile(archive) as z:
            assert not any(name.endswith('.gpak') for name in z.namelist())
            z.extractall(root)
            mappings = ET.fromstring(z.read('fomod/ModuleConfig.xml')).find('requiredInstallFiles')
            payload = {n for n in z.namelist() if n.startswith('SoulWell/')}
            assert {m.attrib['source'].replace('\\', '/') for m in mappings} == payload
            destinations = {m.attrib['destination'].replace('\\', '/') for m in mappings}
            assert destinations == {'mods/' + ('SoulWell.dll' if n.endswith('/SoulWell.dll') else n) for n in payload}
            assert len(mappings) == len(destinations)
            attrs = json.loads(z.read('vortex_override_instructions.json'))
            assert any(x.get('key') == 'modName' and x.get('value') == 'SoulWell' for x in attrs)
            for m in mappings:
                source = m.attrib['source'].replace('\\', '/')
                target = root / 'Vortex game' / m.attrib['destination'].replace('\\', '/')
                target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(z.read(source))
        assets = root / 'SoulWell'
        gate_check(assets / 'SoulWell.dll', assets, root / 'SoulWell.dll')
        deployed = root / 'Vortex game/mods'
        gate_check(deployed / 'SoulWell.dll', deployed / 'SoulWell', deployed / 'SoulWell/SoulWell.dll')
    print('PASS: frozen manifest, loose GON files, FOMOD required-file deployment and Vortex modName association fixture.')


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--game', required=True, type=Path)
    cli.add_argument('--archive', type=Path)
    cli.add_argument('--manifest', type=Path)
    args = cli.parse_args()
    data_check(args.game)
    if args.archive:
        if not args.manifest: cli.error('--archive requires --manifest')
        package_check(args.archive, args.manifest)
    else:
        with tempfile.TemporaryDirectory(prefix='soul-well-source-') as temp:
            assets = Path(temp) / 'SoulWell'
            shutil.copytree(ROOT / 'data', assets / 'data')
            shutil.copyfile(ROOT / 'build/SoulWell.dll', assets / 'SoulWell.dll')
            gate_check(assets / 'SoulWell.dll', assets, Path(temp) / 'SoulWell.dll')
