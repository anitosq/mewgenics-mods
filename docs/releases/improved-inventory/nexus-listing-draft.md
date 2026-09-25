# Improved Inventory — Nexus listing draft

Author preparation only. Not published. Finish the release checklist and add
representative screenshots before submitting. Use the same tested archive
and version as GitHub; do not build a separate Nexus package.

## Summary

Readable, scrolling Storage and Trash grids with search, item-type, rarity
and set filters, integrated into Mewgenics' existing inventory screen.

## Description

Find what you need after an adventure without shrinking your inventory into
a wall of tiny icons. Improved Inventory adds readable scrolling grids and
filters directly to the existing Storage/Trash view.

- Scroll through large inventories with directional indicators.
- Search item names, descriptions and set names.
- Filter weapon consumables, item consumables, worn items, broken items and
  reusable weapons.
- Filter Common, Uncommon, Rare and Very Rare items using the game's symbols.
- Browse sets with item counts and multiple selections, or show items with
  any set or no set.
- Combine filters and clear them with Reset.
- Uses the game's font, rarity artwork and sound feedback.

This beta does not add bulk trash actions. Item transfers and End Day retain
their normal effects, including deletion of items left in Trash at End Day.

## Requirements

Windows x64, Mewgenics Steam build 25143593 (displayed version 1.1.21239).
Other executable builds are rejected by the compatibility check.

Requires [Mewjector](https://www.nexusmods.com/mewgenics/mods/218), API v3;
runtime v3.0 is the tested baseline. No other gameplay mod, Mewtator or
Mewgenics Mod Manager is required. The loader is a separate download.

## Install with Vortex

1. Close the game. Install Mewjector and the
   [Mewgenics Vortex extension](https://www.nexusmods.com/site/mods/1691).
2. Install the Improved Inventory archive, enable it and deploy.
3. Check ImprovedInventory in the game's Load Order page.
4. Launch through Vortex's Custom Launch tool and open Storage/Trash.

The standard Mewjector scan setting discovers the DLL automatically. No
extra numbered loader entry is needed. Keep the DLL and UI assets from the
same archive. If you tested the former Inventory QoL candidate, remove that
old installation first to avoid keeping two copies.

Close the game before disabling, updating or removing the mod. Disable or
remove it through Vortex, then deploy. Unchecking only its Load Order entry
also leaves the mod inactive at the next launch.

## Compatibility and beta limits

Tested through Vortex with only Mewjector and this mod active. Also tested
locally in a larger mod collection; numbered set markers and expanded set
tooltips come from separate mods and are not included here. Compatibility
with every inventory replacement or native hook mod is not guaranteed.

Controller/IME input, other game builds, manual installation and broader
resolution/UI-scale coverage remain unverified. Large-inventory logic has
automated coverage; measured in-game performance at very large counts is
still pending. See the repository's release checklist for detailed evidence.

For startup problems, check `mod_logs/chainloader.log` for ImprovedInventory.
Report the game build, mod version, relevant log lines and reproduction steps.
Remove personal paths from logs before posting; saves are not required for
an initial bug report.

## Source and permissions

[Source and issue tracker](https://github.com/anitosq/mewgenics-mods).
Original code is MIT licensed. Third-party notices remain applicable. The
mod references the game's fonts/artwork at runtime; those assets are not
redistributed in the archive.

## Author upload checklist

- Upload the frozen `ImprovedInventory-0.1.0-beta.2.zip` as a beta/test file;
  set both mod/file version fields consistently and enable Vortex downloads.
- Add Mewjector as a requirement and make original-code permissions match MIT.
- Add screenshots: full inventory, rarity menu, set selection and Trash
  scrolling. Use an isolated profile so screenshots do not imply that other
  mods' numbered markers/tooltips are included.
- Add the actual GitHub release link after it exists; record Nexus mod/file
  IDs and verify both downloads before marking the release synchronized.
