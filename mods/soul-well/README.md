# Soul Well

Summon one shade for each unique cat in your full Soul Jars with a single
trinket use. Soul Well scans home storage, the adventure backpack and jars
equipped by allied campaign cats on the battlefield. The jars stay where they
are. Duplicate jars summon their cat only once per cast.

This is **0.1.0-beta.1**, for Windows x64, Mewgenics **1.1.21239** (Steam build
25143593) and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218) API 3 or
newer. The packaged beta and its shop acquisition have not yet been tested
in-game. Back up your saves before testing.

[Download the GitHub prerelease](https://github.com/anitosq/mewgenics-mods/releases/tag/soul-well/v0.1.0-beta.1).
Choose `SoulWell-0.1.0-beta.1.zip`, not GitHub's source-code archive.

## Install

Close the game first. Install Mewjector separately; it is not included.

- **Vortex:** install the ZIP from file, enable and deploy it. Enable Soul
  Well in Mewgenics Load Order and launch through Vortex.
- **Mewtator:** import the ZIP, enable Soul Well and DLL Mod Support, then
  launch through Mewtator.
- **Manual:** extract the `SoulWell` folder into the game's `mods` folder.
  Move `SoulWell.dll` out of that folder into `mods`. Add the full path to
  `mods/SoulWell` to the game's `-modpaths` launch arguments, alongside your
  other data mods. Keep Mewjector enabled and scanning `mods`.

For a manual install, the two required locations are:

```text
Mewgenics/mods/SoulWell.dll
Mewgenics/mods/SoulWell/data/...
```

Keep only one copy of the DLL. The DLL and data must come from the same
release. A `modlist.txt` entry alone does not satisfy this beta's launch check.
If the executable or required data does not match, the DLL stays inactive and
records the reason in the Mewjector log.

## Get and use Soul Well

For this beta, Tracy's **newly generated consumable offers sell Soul Well for
$1**, including special days. Food and other item slots are unchanged. Stock
already saved in your campaign may need to refresh on a later day. Another
mod that changes Tracy's shop can override these offers; load Soul Well after it.

Equip Soul Well in a trinket slot. During battle, select its ability and a
free adjacent tile. It costs no mana, is a cantrip and can be used once per
fight. The first shade uses the selected tile; the game places subsequent
shades in nearby available spaces. It uses the existing Soul Jar icon.

Keep the full jars in storage, your backpack or the adventurers' equipment.
Soul Well scans them again on every cast. It does not store permanent soul
bindings or consume jars. Empty jars and missing cat IDs are skipped.
Jars equipped by cats left at home are not included.

## Edit the summon passives

The ZIP contains loose `.gon.merge` and `.gon.patch` files, with no `.gpak`.
Edit `additional_passives` inside
`data/abilities/item_abilities.gon.merge`, then restart the game. These edits
do not require rebuilding the DLL. See [CUSTOMIZE.md](CUSTOMIZE.md) for the
exact location and manager-specific notes.

Soul Well has its own ability, `tk_SoulWell`. Changes to the default Soul
Jar's ability do not automatically carry over.

## Limits and removal

A free adjacent target tile is required. Crowded battlefields may leave too
little room for every shade. Completely full boards, large collections and
Armor Specialist extra uses have not been verified. Ordinary repeated use is
blocked by the once-per-fight cost. Scanning stops rather than processing
more than 1,024 unique IDs.

Only the stated executable is supported. Other mods that replace item or
ability definitions can interfere with Soul Well. Keep the summon structure
outside `additional_passives` unchanged; this beta checks that structure at
startup. Other GON edits require rebuilding the matching DLL.

Before uninstalling, remove Soul Well items from the campaign while the mod
is still enabled, then save and close the game. Uninstall through your
manager and deploy afterward in Vortex. For manual installs, remove the DLL,
the `SoulWell` folder and its launch argument. Your Soul Jars are unchanged.

[Source and issue reports](https://github.com/anitosq/mewgenics-mods).
Code and documentation are MIT licensed; see [LICENSE](LICENSE) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
