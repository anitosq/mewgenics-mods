// Inspect the user's installed extension in memory; do not vendor its code.
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import assert from 'node:assert/strict';

const [extension, manifestPath] = process.argv.slice(2);
if (!extension || !manifestPath) throw new Error('Usage: node tools/check_vortex_package.mjs <extension index.js> <package.manifest.json>');
const source = fs.readFileSync(extension, 'utf8');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
const files = Object.keys(manifest.files).map(name => name.replaceAll('/', '\\'));
const sandbox = vm.createContext({
  path: path.win32, MOD_ID: 'mewgenics-mod', MOD_FILES: ['description.json'],
  MOD_FOLDERS: ['data','audio','levels','shaders','swfs','textures'],
  LO_ATTRIBUTE: 'modName', debug: false, spec: {game: {id: 'mewgenics'}},
});
for (const name of ['testMod', 'installMod']) {
  const start = source.indexOf(`function ${name}(`);
  const next = source.indexOf('\nfunction ', start + 1);
  assert(start >= 0 && next > start, 'Installed extension implementation changed');
  vm.runInContext(source.slice(start, next), sandbox, {timeout: 1000});
}
const priority = name => Number(source.match(new RegExp(`registerInstaller\\(${name}, (\\d+)`))?.[1]);
assert(priority('MOD_ID') < priority('MEWJECTOR_MOD_ID'), 'Asset installer no longer precedes DLL-only installer');
assert((await sandbox.testMod(files, 'mewgenics')).supported);
const result = await sandbox.installMod(files, 'InventoryQoL-0.1.0-beta.1');
const copies = result.instructions.filter(x => x.type === 'copy');
assert.equal(copies.length, files.length);
for (const file of files) {
  assert(copies.some(x => x.source === file && x.destination === file), `Wrong destination: ${file}`);
}
assert(result.instructions.some(x => x.type === 'attribute' && x.key === 'modName' && x.value === 'InventoryQoL'));
assert(result.instructions.some(x => x.type === 'setmodtype' && x.value === 'mewgenics-mod'));
console.log('Installed Vortex extension preserves all payload paths under mods/ and registers InventoryQoL as the asset load-order entry.');
