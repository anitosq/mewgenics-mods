# Soul Well

Summon one shade for each unique cat in your full Soul Jars with a single
trinket use. Soul Well scans home storage, the adventure backpack and jars
equipped by allied campaign cats on the battlefield. The jars stay where they
are. Duplicate jars summon their cat only once per cast.

This is **0.1.0-beta.2**, for Windows x64, Mewgenics **1.1.21239** (Steam build
25143593) and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218) API 3 or
newer. This beta's range customization and startup warning have not yet been
tested in-game. Back up your saves before testing.

Install `SoulWell-0.1.0-beta.2.zip` supplied with this candidate. This version
has not been published. Preserve your edited passive block before updating,
then copy it into the new ability file. Replace both the DLL and data files.

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
If the loaded DLL cannot activate, a Windows error dialog explains the cause
and the Mewjector log records it. Close the game and fix the reported issue
before using Soul Well. See the troubleshooting section below.

## Get and use Soul Well

For this beta, Tracy's **newly generated consumable offers sell Soul Well for
$1**, including special days. Food and other item slots are unchanged. Stock
already saved in your campaign may need to refresh on a later day. Another
mod that changes Tracy's shop can override these offers; load Soul Well after it.

Equip Soul Well in a trinket slot. During battle, select its ability and a
free tile within range (adjacent by default). It costs no mana, is a cantrip and can be used once per
fight. The first shade uses the selected tile; the game places subsequent
shades in nearby available spaces. It uses the existing Soul Jar icon.

Keep the full jars in storage, your backpack or the adventurers' equipment.
Soul Well scans them again on every cast. It does not store permanent soul
bindings or consume jars. Empty jars and missing cat IDs are skipped.
Jars equipped by cats left at home are not included.

## Edit the range and summon passives

The ZIP contains loose `.gon.merge` and `.gon.patch` files, with no `.gpak`.
Edit `target.max_range` or the contents of `spawn.additional_passives` inside
`data/abilities/item_abilities.gon.merge`, then restart the game. These edits
do not require rebuilding the DLL. See [CUSTOMIZE.md](CUSTOMIZE.md) for the
exact location and manager-specific notes. For example, `max_range 10` keeps
the DLL active. Comments, indentation and Windows line endings are accepted.

Soul Well has its own ability, `tk_SoulWell`. Changes to the default Soul
Jar's ability do not automatically carry over.

## Limits and removal

A free target tile within the configured range is required. Crowded battlefields may leave too
little room for every shade. Completely full boards, large collections and
Armor Specialist extra uses have not been verified. Ordinary repeated use is
blocked by the once-per-fight cost. Scanning stops rather than processing
more than 1,024 unique IDs.

Only the stated executable is supported. Other mods that replace item or
ability definitions can interfere with Soul Well. Keep the summon structure
outside `additional_passives` and `max_range` unchanged; this beta checks that
structure at startup. Other gameplay-setting edits require rebuilding the
matching DLL. Formatting changes are accepted in all three supplied GON files.

## If Soul Well is inactive

The startup dialog names a missing or incompatible data file, disabled data
path, unsupported game build, loader requirement or hook failure. The same
reason appears as `[SoulWell] INACTIVE` in `mod_logs/chainloader.log` under the
game folder. On the supported executable with its hooks installed, rejected
data also blocks Soul Well casts so they do not produce generic shades.

A DLL that was never loaded cannot show a dialog. Check for a `[SoulWell]`
line saying the version loaded before testing. If there is none, check the
DLL location, Mewjector installation, manager settings and launch method.
Unsupported executables or failed hooks cannot safely block the item's native
behavior, so do not use it after an activation error.

Successful casts log inventory scan counts and each requested jar cat ID.
These are clone requests, not proof that every shade found a free tile. Include
the log after one cast when reporting a problem.

Before uninstalling, remove Soul Well items from the campaign while the mod
is still enabled, then save and close the game. Uninstall through your
manager and deploy afterward in Vortex. For manual installs, remove the DLL,
the `SoulWell` folder and its launch argument. Your Soul Jars are unchanged.

[Source and issue reports](https://github.com/anitosq/mewgenics-mods).
Code and documentation are MIT licensed; see [LICENSE](LICENSE) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
