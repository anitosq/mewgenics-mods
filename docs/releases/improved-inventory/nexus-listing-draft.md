# Improved Inventory Nexus listing copy

Published as [Improved Inventory](https://www.nexusmods.com/mewgenics/mods/526),
version 0.1.0. Release IDs and checksums are in the
[publication record](0.1.0-publication.md); screenshots and artwork are in the
[media guide](listing-media.md).

## Summary

Keep item icons readable with scrolling Storage and Trash grids. Search by
name, description or set, and filter by type, condition and rarity.

## Description

Improved Inventory keeps your item icons readable as your inventory grows.
It adds scrolling, search and filters to the existing Storage/Trash screen.

- Scroll through both grids, with arrows showing when there are more items.
- Search item names, descriptions and set names.
- Filter weapon consumables, item consumables, reusable weapons, worn items and broken items.
- Pick Common, Uncommon, Rare or Very Rare using the game's rarity symbols.
- Search the set list, see item counts and select multiple sets. You can also show any set or no set.
- Combine filters and clear them with Reset.

The controls use the game's font and sounds. Open dropdowns block clicks
and item tooltips underneath them.

## Requirements

Windows x64, Mewgenics 1.1.21239 (Steam build 25143593), and
[Mewjector](https://www.nexusmods.com/mewgenics/mods/218) API v3 (tested with runtime v3.0).
Install Mewjector separately; it's the only mod dependency.
The mod stays inactive on other game builds.

## Install with Vortex

1. Close the game. Install Mewjector and the
   [Mewgenics Vortex extension](https://www.nexusmods.com/site/mods/1691).
2. Download Improved Inventory with Vortex, then install, enable and deploy it.
3. Check ImprovedInventory in the game's Load Order page.
4. Start the game with Vortex's Custom Launch and open Storage/Trash.

If you used the earlier Inventory QoL test build, remove it first.
For updates, close the game and replace the whole mod through Vortex.
To uninstall, remove it and deploy.

## Notes

- Items left in Trash are still deleted when you end the day. Bulk trash actions aren't included.
- Numbered set markers and expanded set tooltips shown in the screenshots come from other mods.
- Controller and IME input, manual installation, and wider UI-scale coverage haven't been tested. In-game performance with very large inventories also needs testing.

For setup problems, look for ImprovedInventory in `mod_logs/chainloader.log`.
Include your game build, mod version and steps to reproduce the problem when
reporting a bug. Remove personal paths before sharing log lines.

## Links

[GitHub release](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.1.0) |
[Source and issue tracker](https://github.com/anitosq/mewgenics-mods)

## File description

Display name: Improved Inventory. File version: 0.1.0.

Scrolling Storage and Trash grids with search and filters. Requires Mewjector API v3 and Mewgenics 1.1.21239 (Steam build 25143593).

## Version 0.1.0 changelog

- First public release, using the tested beta.2 implementation.
- Scrolling Storage and Trash grids with readable icons and scroll arrows.
- Search by item name, description or set name.
- Item-type and rarity filters, plus a searchable set list with item counts and multiple selections.
- The game's font, rarity symbols and sounds. Open dropdowns block clicks and item tooltips underneath them.

## Credits

Mod by anitosq. Thanks to maishullothli for Mewjector and
michael-trinity/mewgenics-savegame-editor for inventory-format research.
Mewgenics provides the font, rarity symbols and sounds. See
THIRD_PARTY_NOTICES.md in the download for licenses.

## Permissions

Use the MIT license text, followed by:

This license covers the mod's code and documentation. Game assets and listing
artwork retain their original ownership; see THIRD_PARTY_NOTICES.md for other licenses.

Source and license: https://github.com/anitosq/mewgenics-mods

## Author upload checklist

- Upload the frozen `ImprovedInventory-0.1.0.zip` to both sites.
- Match the mod/file versions and enable Vortex downloads.
- Add Mewjector as a requirement and use MIT permissions.
- Upload the cover, header and screenshots from [the media guide](listing-media.md).
- Link GitHub and Nexus, record file IDs, and verify both downloads.
