import {filterItems, sortItems, availableSets} from './model.mjs';
const $ = id => document.getElementById(id);
let data;
let advanced = {slot:'all', set:'all', rarities:[]};
const pretty = value => value.replaceAll('_', ' ').replace(/\b\w/g, c => c.toUpperCase());
function node(tag, text, className) {
  const element = document.createElement(tag);
  element.textContent = text;
  if (className) element.className = className;
  return element;
}
function option(value, text) {
  const result = node('option', text); result.value = value; return result;
}
function inspect(item) {
  const content = $('details-content'); content.replaceChildren(node('h2', item.name));
  const fields = {ID:item.itemId, Location:pretty(item.container), Type:pretty(item.category), Slot:pretty(item.slot),
    Rarity:pretty(item.rarity), Sets:item.sets.map(s=>s.name).join(', ') || (item.setsKnown ? 'No set' : 'Unknown'),
    Uses:item.charges < 0 ? 'Not specified / unlimited' : String(item.charges), Condition:'Not yet decoded',
    Metadata:item.metadataStatus};
  const dl = document.createElement('dl');
  for (const [key, value] of Object.entries(fields)) dl.append(node('dt',key),node('dd',value));
  content.append(dl, node('p','This copy is identified only within this saved snapshot.', 'note'));
  $('details').showModal();
}
function render(resetScroll = true) {
  const filters = {query:$('search').value, container:$('container').value, category:$('category').value,
    ...advanced};
  const items = sortItems(filterItems(data.items, filters), $('sort').value);
  $('result-count').textContent = `${items.length} matching / ${data.items.length} items`;
  $('scope-summary').textContent = filters.container === 'all' ? 'All saved containers' : `${pretty(filters.container)} · ${data.containerCounts[filters.container]} saved items`;
  const fragment = document.createDocumentFragment();
  for (const item of items) {
    const card = node('button', '', 'item'); card.type='button'; card.dataset.rarity=item.rarity; card.dataset.instanceId=item.instanceId;
    card.append(node('span', item.category === 'consumable' ? 'Consumable' : pretty(item.slot), 'item-type'), node('span',item.name,'item-name'));
    card.append(node('span',`${pretty(item.rarity)}${item.charges >= 0 ? ` · ${item.charges} uses` : ''}`, 'item-meta'));
    const tags = node('span','','tags');
    for (const set of item.sets) tags.append(node('span',set.name,'tag'));
    if (!item.setsKnown) tags.append(node('span','Unverified metadata','tag'));
    if (item.setsKnown && !item.sets.length) tags.append(node('span','No set','tag'));
    card.append(tags,node('span',pretty(item.container),'location'));
    card.addEventListener('click',()=>inspect(item)); fragment.append(card);
  }
  const position=$('items').scrollTop;
  $('items').replaceChildren(fragment); $('items').scrollTop=resetScroll?0:position;
  $('empty').hidden=items.length!==0;
  renderFilterChips();
}

function renderFilterChips() {
  const chips = [];
  const add = (text, remove) => {
    const chip = node('button', `${text} ×`, 'filter-chip');
    chip.type='button'; chip.setAttribute('aria-label',`Remove filter: ${text}`);
    chip.addEventListener('click',()=>{remove();render();}); chips.push(chip);
  };
  if ($('search').value.trim()) add(`Search: ${$('search').value.trim()}`,()=>{$('search').value='';});
  if ($('category').value!=='all') add(pretty($('category').value),()=>{$('category').value='all';});
  if (advanced.slot!=='all') add(`Slot: ${pretty(advanced.slot)}`,()=>{advanced.slot='all';});
  if (advanced.set!=='all') {
    const setName = [...$('set').options].find(option=>option.value===advanced.set)?.textContent || advanced.set;
    add(setName,()=>{advanced.set='all';});
  }
  for (const rarity of advanced.rarities) add(pretty(rarity),()=>{advanced.rarities=advanced.rarities.filter(r=>r!==rarity);});
  $('active-filters').replaceChildren(...chips);
  $('active-filters').hidden=chips.length===0;
  const count=Number(advanced.slot!=='all')+Number(advanced.set!=='all')+advanced.rarities.length;
  $('more-filters').textContent=count?`More filters (${count})`:'More filters';
}

function openFilters() {
  $('slot').value=advanced.slot; $('set').value=advanced.set;
  for (const input of $('rarities').querySelectorAll('input')) input.checked=advanced.rarities.includes(input.value);
  $('filter-dialog').showModal();
}

function clearFilters() {
  $('search').value='';
  for (const id of ['category','container']) $(id).value='all';
  advanced={slot:'all',set:'all',rarities:[]};
  render();
}
try {
  const response=await fetch('./inventory.json');
  if(!response.ok) throw new Error('Inventory snapshot unavailable');
  data=await response.json();
  if(!Array.isArray(data.items)) throw new Error('Invalid snapshot');
  for (const slot of [...new Set(data.items.map(i=>i.slot))].sort()) $('slot').append(option(slot,pretty(slot)));
  for (const rarity of [...new Set(data.items.map(i=>i.rarity))].sort()) {
    const label=node('label',''); const input=document.createElement('input');input.type='checkbox';input.value=rarity;
    label.append(input,document.createTextNode(pretty(rarity)));$('rarities').append(label);
  }
  for(const set of availableSets(data.items)) $('set').append(option(set.id,set.name));
  $('snapshot').textContent=`Saved ${data.savedAt.replace('T',' ')} · ${data.source}`;
  for(const id of ['container','category']) $(id).addEventListener('change',()=>render());
  $('more-filters').addEventListener('click',openFilters);
  $('cancel-filters').addEventListener('click',()=>$('filter-dialog').close());
  $('filter-form').addEventListener('submit',event=>{
    event.preventDefault();
    advanced={slot:$('slot').value,set:$('set').value,
      rarities:[...$('rarities').querySelectorAll('input:checked')].map(input=>input.value)};
    $('filter-dialog').close();render();
  });
  $('search').addEventListener('input',()=>render());
  $('sort').addEventListener('change',()=>render(false));
  $('density').addEventListener('change',()=>{$('items').dataset.density=$('density').value;});
  $('clear').addEventListener('click',clearFilters);$('empty-clear').addEventListener('click',clearFilters);
  $('close-details').addEventListener('click',()=>$('details').close());
  render();
} catch(error) {
  $('items').replaceChildren(node('p',`Could not load preview: ${error.message}`, 'error'));
  $('result-count').textContent='Preview unavailable';
}
