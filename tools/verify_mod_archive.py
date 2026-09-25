"""Compare a mod ZIP with its trusted build manifest, without extracting it."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import zipfile


def sha256(stream):
    digest = hashlib.sha256()
    for block in iter(lambda: stream.read(1024 * 1024), b''):
        digest.update(block)
    return digest.hexdigest()


def valid_path(name):
    return (isinstance(name, str) and bool(name) and '\\' not in name
            and ':' not in name and not name.startswith('/')
            and all(part not in ('', '.', '..') for part in name.split('/'))
            and PurePosixPath(name).as_posix() == name)


def valid_digest(value):
    return isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)


def verify_archive(manifest_path, archive_path, allow_repacked=False):
    manifest = json.loads(Path(manifest_path).read_text(encoding='utf-8'))
    if not isinstance(manifest, dict):
        raise ValueError('Manifest must be a JSON object.')
    expected = manifest.get('files')
    if not isinstance(expected, dict) or not expected:
        raise ValueError('Manifest must contain a nonempty files/hash mapping.')
    if not valid_digest(manifest.get('sha256')):
        raise ValueError('Manifest must contain a lowercase SHA-256 archive hash.')
    if any(not valid_path(name) or not valid_digest(digest)
           for name, digest in expected.items()):
        raise ValueError('Manifest has an invalid payload path or SHA-256 hash.')

    with Path(archive_path).open('rb') as stream:
        archive_hash = sha256(stream)
        stream.seek(0)
        with zipfile.ZipFile(stream) as archive:
            entries = archive.infolist()
            names = [entry.filename for entry in entries]
            if len(names) != len(set(names)):
                raise ValueError('Archive contains duplicate entries.')
            for entry in entries:
                name = entry.filename[:-1] if entry.is_dir() else entry.filename
                if not valid_path(name):
                    raise ValueError('Archive contains an invalid payload path.')
                if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError('Archive contains a symbolic link.')
            files = {entry.filename for entry in entries if not entry.is_dir()}
            if files != set(expected):
                missing = sorted(set(expected) - files)
                extra = sorted(files - set(expected))
                raise ValueError(f'Payload list differs: missing={missing}, extra={extra}')
            for name, digest in expected.items():
                with archive.open(name) as payload:
                    if sha256(payload) != digest:
                        raise ValueError(f'Payload hash differs: {name}')

    archive_matches = archive_hash == manifest['sha256']
    if not archive_matches and not allow_repacked:
        raise ValueError('ZIP hash differs, but every payload matches. Use '
                         '--allow-repacked only when a repacked ZIP is expected.')
    return {'version': manifest.get('version'),
            'source_commit': manifest.get('source_commit'),
            'archive_sha256': archive_hash,
            'archive_matches': archive_matches,
            'payload_matches': True, 'file_count': len(expected)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path,
                        help='Trusted local manifest from the frozen build')
    parser.add_argument('--archive', required=True, type=Path,
                        help='Local or downloaded ZIP; its filename may differ')
    parser.add_argument('--allow-repacked', action='store_true',
                        help='Accept changed ZIP bytes only if all payloads match')
    args = parser.parse_args()
    try:
        result = verify_archive(args.manifest, args.archive, args.allow_repacked)
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError) as error:
        parser.exit(1, f'Verification failed: {error}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
