"""Compare serialized inventory containers without modifying either save."""
import argparse
import json
from pathlib import Path
from probe_inventory import read_save


def compare(before, after):
    old_items, old_blobs = read_save(before)
    new_items, new_blobs = read_save(after)
    return {'beforeCount': len(old_items), 'afterCount': len(new_items),
            'containersUnchanged': old_blobs == new_blobs,
            'before': old_blobs, 'after': new_blobs}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    args = parser.parse_args()
    result = compare(args.before, args.after)
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['containersUnchanged'] else 1)
