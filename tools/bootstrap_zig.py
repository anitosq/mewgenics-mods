"""Fetch a pinned portable C compiler into work/, verifying the publisher hash."""
import hashlib
import json
from pathlib import Path
import urllib.request
import zipfile

VERSION = '0.15.2'
root = Path(__file__).resolve().parents[1] / 'work' / 'toolchains'
root.mkdir(parents=True, exist_ok=True)
compiler = root / f'zig-x86_64-windows-{VERSION}' / 'zig.exe'
if not compiler.exists():
    with urllib.request.urlopen('https://ziglang.org/download/index.json', timeout=30) as response:
        manifest = json.load(response)
    entry = manifest[VERSION]['x86_64-windows']
    url = entry['tarball']
    if not url.startswith('https://ziglang.org/download/'):
        raise RuntimeError('Unexpected compiler download host')
    archive = root / f'zig-{VERSION}.zip'
    urllib.request.urlretrieve(url, archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != entry['shasum']:
        raise RuntimeError('Compiler checksum mismatch; archive will not be extracted')
    with zipfile.ZipFile(archive) as source:
        for member in source.infolist():
            target = (root / member.filename).resolve()
            if root.resolve() not in target.parents:
                raise RuntimeError('Unexpected archive path')
        source.extractall(root)
    (root / 'zig-download.json').write_text(json.dumps(entry, indent=2), encoding='utf-8')
print(compiler)
