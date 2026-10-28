# Improved Inventory changelog

## 0.5.0-beta.4

- Slot types are listed as Weapon, Head, Neck, Face, then Trinket.

## 0.5.0-beta.3

- Item type can now show one equipment slot: Head, Face, Neck, Weapon or
  Trinket.

## 0.5.0-beta.2

- The Stats and Effects menus show how many items match each choice.
- A Storage size equal to an unusual capacity (for example 9x9) is now
  remembered after a restart.
- Pre-adventure equipment no longer shows more columns than your Storage size.
- The Effects menu's Done button highlights on hover.

## 0.5.0-beta.1

- New Stats filter: find items that raise or lower Strength, Dexterity,
  Constitution, Intelligence, Charisma, Speed or Luck.
- New Effects filter: find items that grant or inflict Bleed, Poison, Burn,
  Freeze, Stun, Bruise, Confusion, Slow, Fear, Weakness, Thorns, Brace,
  Shield, Holy Shield, Health Regen, Dodge Chance or Charge, including through
  the ability an item grants. Pick several to match any of them.
- Stat and effect names appear in the game's language.
- Includes the 0.4.0-beta.6 grid sizes and adventure Backpack support.

## 0.4.0-beta.6

- Fixed Improved Inventory no longer appearing at home in 0.4.0-beta.5.
- A fixed Trash size is a maximum: Trash grows with its items up to that size,
  then scrolls. An empty Trash stays at the game's 4x4. This replaces the
  beta.4/beta.5 attempt to show the full size from the start.

## 0.4.0-beta.5

- Fixed Trash sizes now apply: 0.4.0-beta.4 kept the game's 4x4 Trash.
  A larger Trash size now takes effect immediately.

## 0.4.0-beta.4

- Search, filters, set list and a scrolling grid in the adventure Backpack.
  The Backpack follows the Trash size (a fixed size or Auto); items equipped
  on your cats stay in their slots.
- The Storage button only offers sizes up to your Storage capacity.
- A fixed Trash size now shows the full grid even when Trash is nearly empty.
  A new Trash size applies the next time Trash opens.
- Still logs the read-only Backpack diagnostics from 0.4.0-beta.3.

## 0.4.0-beta.3

- Diagnostic beta: logs read-only details about the adventure Backpack screen
  to `mod_logs/chainloader.log`, to prepare inventory tools there. The Backpack
  itself is unchanged. Otherwise identical to 0.4.0-beta.2.

## 0.4.0-beta.2

- Choose each grid's size with the new Storage and Trash buttons beside the
  item counts: 5x5, 6x6, 7x7, 8x8 or 10x10, with scrolling beyond that.
  Trash can also be set to Auto, which grows with the items to discard, as in
  the unmodded game. The Storage size also applies to pre-adventure equipment.
  Both choices are remembered.
- Set names now appear without the "Set Bonus" wording in every game language,
  so the set filter and set list show the actual set name in French and others.
- Includes the 0.3.8-beta.1 performance changes.

## 0.3.8-beta.1

- Skip repeated animation updates for idle, hidden items while Storage or Trash is open.
- Reduce item-metadata lookup work when using search, filters and set lists.

## 0.3.7

- Fixed house-screen lag after closing Storage.
- Reduced repeated work while scrolling and using inventory search, filters and set lists.

## 0.3.6

- Fixed the mod loading without activating when Mewjector loads it after game startup.

## 0.3.5

- Reduced repeated work for hidden inventory controls.

## 0.3.4

- Reduced inventory update overhead, especially with large inventories.

## 0.3.3

- Fixed the "Not managed by Vortex" label in Load Order.

## 0.3.2

- One download now works with both Vortex and Mewtator.
- Inventory features are unchanged from 0.3.1.

## 0.3.1

- Added a Mewtator download that supports its configured mods directory,
  including folders outside the game installation.
- Startup logs now distinguish disabled assets from missing or mismatched files.

## 0.3.0 (2026-09-28)

- See separate Storage and Trash counts beside each set.
- Set counts include duplicate copies and stay visible while you search or filter.

## 0.2.0 (2026-09-25)

- Fixed a crash when leaving the house to assign equipment for an adventure.
- Added scrolling, search, and type, rarity and set filters to pre-adventure
  equipment selection. Equipped items stay in their normal slots.

## 0.1.0 (2026-09-25)

First public release.

- Scrolling Storage and Trash grids with readable item icons.
- Search by item name, description or set name.
- Filters for consumables, reusable weapons, worn and broken items, and rarity.
- A searchable set list with item counts and multiple selections.

Requires Mewjector API v3 and Mewgenics 1.1.21239 (Steam build 25143593)
on Windows x64. See INSTALL.md for setup.
