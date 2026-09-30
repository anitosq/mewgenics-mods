# Guaranteed Boss Loot

Makes these three boss-loot choices always succeed without killing the cat
who searches:

- **Dead King:** looting the Throbbing King's body succeeds.
- **The Rift:** reaching into the rift succeeds.
- **Dead God:** examining the Creator's remains succeeds.

Rewards, boss fights and other events are unchanged.

## Install

Available on [Nexus Mods](https://www.nexusmods.com/mewgenics/mods/536).

Use the `GuaranteedBossLoot` ZIP attached to the
[GitHub release](https://github.com/anitosq/mewgenics-mods/releases/tag/guaranteed-boss-loot/v0.1.1),
not GitHub's automatic source-code archive.

**Vortex:** install the archive, enable it, deploy, then enable Guaranteed
Boss Loot in the Mewgenics Load Order view. Launch through Vortex.

**Mewtator:** import the same ZIP, enable Guaranteed Boss Loot in your profile,
and launch through Mewtator.

**Manual:** extract the `GuaranteedBossLoot` folder into the game's `mods`
folder and add `GuaranteedBossLoot` to `mods/modlist.txt`.

This is a data-only mod. It does not require Mewjector or a DLL loader.

## Compatibility

Load after other mods that change these boss events, including FewerBadEvents
and Event Descriptions. Later overrides can undo the guarantee.
Event-description mods may still display their original warning text.

Disable or remove the mod to restore normal behavior for future events.

## Credits and source

Original mod source is under the MIT License. Mewgenics and its artwork
belong to Edmund McMillen, Tyler Glaiel and their respective rights holders.
Promotional images are separate from the downloadable mod.

[Source and issue reports](https://github.com/anitosq/mewgenics-mods)

## Development

From the repository root, run `python tools/check_boss_loot.py --game PATH`.
Add `--build` on a clean committed tree to create a frozen archive and manifest
under `outputs/releases/guaranteed-boss-loot/`. The check reads game files;
it does not deploy mods, launch the game or change saves.
