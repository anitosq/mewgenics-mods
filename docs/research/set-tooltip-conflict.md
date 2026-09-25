# Set tooltip conflict — 24 September 2026

Diagnosed during test session 212543, PID 40028. Read-only inspection of the
running game's item definitions is saved in
`work/inventory-native/set-tooltip-conflict.json`.

The active order places VisibleSetItemMarkers and SetBonusTooltips before
NoCursesOnItems, followed by NoBreakingItems. NoCursesOnItems supplies full
replacement `.gon` files for eight item categories, including
`beanies_quest_items.gon` and `head_items.gon`, but not `weapons.gon`.
Those replacements erase the earlier marker/name and tooltip merges.

| Runtime item | Set | Set tooltip | Marker in name |
| --- | --- | --- | --- |
| PrincessHat_Fixed (Used Princess Hat) | Used | Missing | Missing |
| HeavyMace | Fighter | FighterSet: 1 | 034 |
| Wig | Used | Missing | Missing |
| MedievalHelmet | Fighter | Missing | Missing |

All four retain `degrade_after_adventure: false`, consistent with the later
NoBreakingItems merge. SetBonusTooltips explicitly includes
`PrincessHat_Fixed {tooltips{UsedSet 1}}`; the tooltip is not absent from that
mod's source. Our inventory DLL reads item metadata and does not rewrite
these item definitions; its generated assets contain only inventory UI SWFs.

Recommended repair: place NoCursesOnItems before the item-data merge mods
in Vortex's data-mod load order, especially VisibleSetItemMarkers,
SetBonusTooltips and NoBreakingItems. Review Better Loot's position against
the same replacement files as well. This concerns `mods/modlist.txt` and
the generated `-modpaths` order, not DLL ordering in `chainloader.ini`.
No load-order changes were made during this diagnosis. Restart and verify
the effective definitions and hover display after applying the repair.

Follow-up: the user moved NoCursesOnItems before the three item merge mods.
Both modlist.txt and Vortex's launch.bat reflect the correction. The broader
audit in `mod-compatibility-audit.md` found no shared Better Loot paths;
moving Better Loot is unnecessary. Runtime hover verification of the revised
order is still pending.
