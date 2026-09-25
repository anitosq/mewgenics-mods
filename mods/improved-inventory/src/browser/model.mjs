// Pure view operations. No inventory mutation or persistence.
export function filterItems(items, filters = {}) {
  const query = (filters.query || '').trim().toLocaleLowerCase();
  return items.filter(item => {
    if (filters.container && filters.container !== 'all' && item.container !== filters.container) return false;
    if (filters.category && filters.category !== 'all' && item.category !== filters.category) return false;
    if (filters.slot && filters.slot !== 'all' && item.slot !== filters.slot) return false;
    if (filters.rarities?.length && !filters.rarities.includes(item.rarity)) return false;
    if (filters.set === 'any' && (!item.setsKnown || !item.setIds.length)) return false;
    if (filters.set === 'none' && (!item.setsKnown || item.setIds.length)) return false;
    if (filters.set && !['all', 'any', 'none'].includes(filters.set) && !item.setIds.includes(filters.set)) return false;
    return !query || [item.name, item.itemId, ...item.sets.map(set => set.name)].join(' ').toLocaleLowerCase().includes(query);
  });
}

const rarityRank = new Map(['common', 'uncommon', 'rare', 'very_rare', 'veryrare', 'sidequest', 'quest', 'unknown'].map((r, i) => [r, i]));
export function sortItems(items, sort = 'name') {
  return [...items].sort((a, b) => {
    let comparison = 0;
    if (sort === 'rarity') comparison = (rarityRank.get(a.rarity) ?? 99) - (rarityRank.get(b.rarity) ?? 99);
    if (sort === 'slot') comparison = a.slot.localeCompare(b.slot);
    if (sort === 'set') comparison = (a.sets[0]?.name ?? '\uffff').localeCompare(b.sets[0]?.name ?? '\uffff');
    if (sort !== 'original') comparison ||= a.name.localeCompare(b.name);
    return comparison || a.container.localeCompare(b.container) || a.ordinal - b.ordinal;
  });
}

export function availableSets(items) {
  const sets = new Map();
  for (const item of items) for (const set of item.sets) sets.set(set.id, set);
  return [...sets.values()].sort((a, b) => a.name.localeCompare(b.name));
}
