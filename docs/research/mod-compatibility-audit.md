# Installed mod compatibility audit

24 September 2026. Read-only audit of the 13 enabled mods, current
`mods/modlist.txt`, generated `launch.bat`, installed `resources.gpak`, our
inventory source/assets and the completed test session 212543. No deployment,
load-order or save edits were made. The revised order has not yet been
verified in a new game session.

## Result

No further load-order change is indicated by the inspected files. The user's
new order fixes the identified replacement/merge precedence problem:

`NoCursesOnItems → VisibleSetItemMarkers → SetBonusTooltips → NoBreakingItems`

Both Vortex-generated launch arguments and modlist agree. The old test
runner completed cleanup, restored the loader configuration, and restored
the original slot 3. Only the copied test campaign changed in that session.

## Overlap checks

| Mods | Evidence and conclusion |
| --- | --- |
| No Curses / markers / set tooltips / No Breaking | Eight whole-file replacements now precede the relevant merges. Across all item merge files, no two mods write the same leaf field: marker mods change names, tooltip mods add tooltip entries, and No Breaking changes degradation. Preserve this order. |
| Fewer Bad Events / Event Descriptions | Share 23 event files. The description merges contain 500 label edits only. Comparing them with the 187 overwrite operations in the overlapping event patches found no reintroduced removed branch. Current order allows description labels to follow gameplay changes. Their text can nevertheless describe outcomes that Fewer Bad Events removes. |
| Event Descriptions / Detailed Ability Descriptions | Both append combined.csv, but their 480 and 2,756 data keys have no overlap. Header rows were excluded from this key comparison. |
| Better Loot | Changes combat_reward_table.gon and item_pools/general_pools.gon. Neither path overlaps No Curses or any other active mod. No need to move it relative to No Curses. |
| Bigger Storage Early / Reroll / Better Furniture / StevenNeutered | Their target files are not shared by another active data mod. Whole-file compatibility with the current game is a separate concern below. |
| Inventory QoL / set markers | Inventory QoL supplies inventory_qol.swf and appends swflist.gon; markers replace catparts.swf. No common replaced SWF. Inventory QoL reads effective item metadata and changes inventory presentation, not the item-definition files. |
| Inventory QoL / BiggerWallet | Previous session logs show BiggerWallet hooks 114E38; none of Inventory QoL's nine hook sites match it. No evidence of an order collision. Keep the existing loader setup. |

The inventory runner reads the current enabled modlist on each launch and
adds its own assets last. It does not use a frozen list of the old order.
The six-column inventory is a display layout, not a storage-capacity edit.
These checks establish no detected overlap conflict, not exhaustive runtime
compatibility for every feature.

## Problems that reordering cannot fix

### No Curses on Items 0.1

The installed mod replaces eight complete item files. Relative to this
installed game's originals, it changes more than `cursed` flags:

- EmptySack, LuckyCoin and ScrapBag have `rarity uncommmon` instead of
  `uncommon`. The invalid token can affect rarity display/filter metadata;
  its runtime fallback has not been checked in this audit.
- Malaria adds six permanent -1 stat changes at battle end that the current
  base definition does not have.
- NaegleriaFowleri swaps the current Uncontrollable behavior for a
  battle-start SafeDie behavior at its passive threshold.
- Veiny item effects, FishNecklace passives, and HexagramSigil's immunity
  exclusions differ from the current base game as well.
- SlimyHat loses its base Immobile keyword tooltip.

These differences are consistent with an older full-file snapshot, although
the author's intent and exact source version are unverified. None of the
later active merge mods repairs these unrelated fields. Loading No Curses
earlier preserves the marker/tooltip merges but still replaces base gameplay.

Recommended repair: use a current compatible release or replace the eight
full files with narrowly scoped patches that only implement curse removal.
Do not patch only the rarity typos and assume the other differences are gone.

### StevenNeutered 1.0.0

This mod advertises disabling Steven's Deja Vu. It replaces all of
`data/passives/disorders.gon`, also changing Hypersomnia, SchrodingerDisorder
and Sociopathy relative to the current base. For example, Sociopathy becomes
+10 Charisma without the current +5 Intelligence, instead of +5/+5.

Recommended repair: restrict its changes to the intended DejaVu entries.
Reordering cannot recover unrelated current-game disorder definitions.

### Event description wording

Example: the TrashBin Examine label still advertises Poison/Parasite while
Fewer Bad Events replaces its examine branch with positive outcomes only.
Neither order updates the prose to reflect those changed outcomes. A small
compatibility text patch is appropriate if those warnings are confusing.

### Better Furniture

Its full replacement differs from the current base in 872 values, all named
Comfort, Appeal, Stimulation, Evolution or Health; no top-level furniture
definitions are missing or added. These are consistent with its furniture
stat purpose. No order issue or unrelated-field difference was found here.

## Audit evidence

- `work/inventory-native/mod-file-overlaps.json`: 100 active payload/support
  files grouped by target; paths and replacement/merge/append/patch types.
- `work/inventory-native/mod-order-audit.json`: item leaf collisions, event
  overwrite comparisons and CSV-key intersections. No GON parse failures.
- `work/inventory-native/full-replacement-audit.json`: semantic differences
  from the installed base for every full replacement GON.
- `work/backups/inventory-observation-20260924-212543/chainloader.log`:
  previously observed DLL hook sites.

No compatibility patches were installed during this audit. Priority for
follow-up is the unrelated No Curses changes, then StevenNeutered, then the
event-description wording.
