import {test} from 'node:test';
import assert from 'node:assert/strict';
import {filterItems, sortItems, availableSets} from '../src/browser/model.mjs';
const make=(n, patch={})=>({instanceId:`storage:${n}`, ordinal:n, name:`Item ${n}`, itemId:`Item${n}`,
  container:'storage', category:'equipment', slot:'head', rarity:'common', setsKnown:true, setIds:[], sets:[], ...patch});

test('OR within rarity, AND across facets; multi-set items appear once',()=>{
  const items=[make(0,{category:'consumable',setIds:['Bone','Meat'],sets:[{id:'Bone',name:'Bone'},{id:'Meat',name:'Meat'}]}),
    make(1,{category:'consumable',rarity:'uncommon'}),make(2,{rarity:'uncommon'}),make(3,{category:'consumable',rarity:'rare'})];
  assert.equal(filterItems(items,{category:'consumable',rarities:['common','uncommon']}).length,2);
  assert.equal(filterItems(items,{set:'Meat'}).length,1);
  assert.equal(filterItems(items,{query:'bone'}).length,1);
  assert.equal(availableSets(items).length,2);
});
test('unknown metadata never counts as a non-set item',()=>{
  assert.equal(filterItems([make(0),make(1,{setsKnown:false})],{set:'none'}).length,1);
});
test('duplicates retain physical snapshot identity; sorting never mutates input',()=>{
  const items=[make(0,{name:'Z'}),make(1,{name:'A'}),make(2,{name:'A'})];
  const before=JSON.stringify(items);
  const sorted=sortItems(items);
  assert.deepEqual(sorted.map(i=>i.ordinal),[1,2,0]);
  assert.equal(new Set(sorted.map(i=>i.instanceId)).size,3);
  assert.equal(JSON.stringify(items),before);
});
test('1000 items remain present and composed filters are exact',()=>{
  const items=Array.from({length:1000},(_,i)=>make(i,{category:i%2?'equipment':'consumable',rarity:i%5?'common':'rare'}));
  assert.equal(sortItems(items).length,1000);
  assert.equal(filterItems(items,{category:'consumable',rarities:['rare']}).length,100);
  assert.equal(filterItems(items,{container:'trash'}).length,0);
});
