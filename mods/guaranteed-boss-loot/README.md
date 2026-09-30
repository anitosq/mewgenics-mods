# Guaranteed Boss Loot

Losing a cat to the loot roll after winning a boss fight is a rough way to
end an act. Guaranteed Boss Loot makes the three act-ending loot choices
succeed, so the cat who searches survives and you get the normal reward.

## What changes

- **Dead King:** looting the Throbbing King's body succeeds.
- **The Rift:** reaching into the rift succeeds.
- **Dead God:** examining the Creator's remains succeeds.

The original reward pools and successful outcomes stay the same. This does
not change boss fights, other events, item rarity or the number of rewards.
You still choose whether to loot and which cat takes the action.

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

Disable **Never Fail Boss Looting** if it is installed; you only need one.
Load Guaranteed Boss Loot after **FewerBadEvents**, **Event Descriptions**,
and any other mod that changes these boss events. A later event override
can undo the guarantee. Event-description mods may still display their
original warning text.

The patch changes only the success chance and failure outcome of these
three choices. It does not replace the full event file or alter saves.
Disabling or removing it restores normal behavior for future events;
it does not undo rewards or deaths that already happened.

## Credits and source

Inspired by BaronMcChicken's
[Never Fail Boss Looting](https://www.nexusmods.com/mewgenics/mods/80).
This is an independent implementation by anitosq.

Original mod source is under the MIT License. Mewgenics and its artwork
belong to Edmund McMillen, Tyler Glaiel and their respective rights holders.
Promotional images are separate from the downloadable mod.

[Source and issue reports](https://github.com/anitosq/mewgenics-mods)

## Development

From the repository root, run `python tools/check_boss_loot.py --game PATH`.
Add `--build` on a clean committed tree to create a frozen archive and manifest
under `outputs/releases/guaranteed-boss-loot/`. The check reads game files;
it does not deploy mods, launch the game or change saves.
