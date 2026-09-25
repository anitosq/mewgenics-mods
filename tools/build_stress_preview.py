"""Make clearly labeled synthetic copies for UI scale testing, not gameplay."""
import json
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[1]
preview = root / 'outputs/inventory-preview'
data = json.loads((preview / 'inventory.json').read_text(encoding='utf-8'))
originals = data['items']
data['items'] = [{**originals[i % len(originals)], 'instanceId': f'synthetic:{i}',
                  'ordinal': i, 'container': 'storage'} for i in range(1000)]
data.update(source='SYNTHETIC 1000-ITEM TEST', mode='synthetic load test',
            containerCounts={'storage':1000,'trash':0,'backpack':0})
target = preview / 'stress'
target.mkdir(exist_ok=True)
for path in (root / 'mods/improved-inventory/src/browser').iterdir():
    shutil.copyfile(path, target / path.name)
(target / 'inventory.json').write_text(json.dumps(data, ensure_ascii=False), encoding='utf-8')
print(target)
