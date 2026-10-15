"""Freeze an allowlisted Soul Well ZIP from a clean, matching build."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

from build import ROOT, VERSION, EXPECTED, ITEM, ABILITY, SHOP

REPO = ROOT.parents[1]
sys.path.insert(0, str(REPO / 'tools'))
from package_improved_inventory import json_bytes, write_archive
from verify_mod_archive import verify_archive


def main():
    git = lambda *args: subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()
    if git('status', '--porcelain'):
        raise SystemExit('Commit release source before packaging.')
    commit = git('rev-parse', 'HEAD')
    report = json.loads((ROOT / 'build/build-info.json').read_text())
    dll = (ROOT / 'build/SoulWell.dll').read_bytes()
    digest = lambda data: hashlib.sha256(data).hexdigest()
    if (report['source_commit'] != commit or report['dirty'] or report['version'] != VERSION or
            report['dll_sha256'] != digest(dll) or report['offline_tests'] != 'passed'):
        raise SystemExit('Rebuild from the current clean source commit first.')
    files = {'SoulWell/' + name: (ROOT / name).read_bytes() for name in
             (ITEM, ABILITY, SHOP, 'README.md', 'CUSTOMIZE.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md')}
    files['SoulWell/SoulWell.dll'] = dll
    files['SoulWell/description.json'] = json_bytes({
        'title': 'Soul Well', 'author': 'anitosq', 'version': VERSION,
        'description': 'Summon unique cats from Soul Jars in storage, your backpack and adventurers\' equipment. Requires Mewjector API 3.',
        'url': 'https://github.com/anitosq/mewgenics-mods',
    })
    files['SoulWell/build-info.json'] = json_bytes({
        'version': VERSION, 'source_commit': commit, 'game_sha256': EXPECTED,
        'steam_build_id': 25143593, 'loader_api_minimum': 3, 'compiler': report['compiler'],
    })
    config = ET.Element('config', {'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance',
        'xsi:noNamespaceSchemaLocation': 'http://qconsulting.ca/fo3/ModConfig5.0.xsd'})
    ET.SubElement(config, 'moduleName').text = 'Soul Well'
    required = ET.SubElement(config, 'requiredInstallFiles')
    for name in sorted(files):
        target = 'SoulWell.dll' if name.endswith('/SoulWell.dll') else name
        ET.SubElement(required, 'file', source=name.replace('/', '\\'),
                      destination=('mods/' + target).replace('/', '\\'), priority='0')
    ET.indent(config)
    files['fomod/ModuleConfig.xml'] = ET.tostring(config, encoding='utf-8')
    metadata = 'vortex_override_instructions.json'
    files[metadata] = json_bytes([{'type': 'attribute', 'source': metadata,
                                  'key': 'modName', 'value': 'SoulWell'}])
    if git('status', '--porcelain') or git('rev-parse', 'HEAD') != commit:
        raise SystemExit('Source changed during packaging.')
    output = REPO / 'outputs/releases/soul-well' / VERSION
    output.mkdir(parents=True, exist_ok=False)
    archive = output / f'SoulWell-{VERSION}.zip'
    write_archive(archive, files)
    checksum = digest(archive.read_bytes())
    manifest = archive.with_suffix('.manifest.json')
    manifest.write_bytes(json_bytes({'version': VERSION, 'source_commit': commit, 'archive': archive.name,
        'sha256': checksum, 'files': {name: digest(data) for name, data in sorted(files.items())}}))
    archive.with_suffix('.zip.sha256').write_text(f'{checksum}  {archive.name}\n', encoding='ascii')
    print(json.dumps(verify_archive(manifest, archive), indent=2))
    print(archive)


if __name__ == '__main__':
    main()
