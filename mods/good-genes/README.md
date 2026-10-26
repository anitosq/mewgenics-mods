# Good Genes

Pass parental skills, mutations and higher base stats to kittens, and protect
occupied mutation slots during events and combat.

Version 0.3.2.
[Nexus Mods](https://www.nexusmods.com/mewgenics/mods/547) |
[Published releases](https://github.com/anitosq/mewgenics-mods/releases)

## Breeding

- Kittens inherit two distinct parental active skills when available, or one
  when only one unique skill is available, plus one eligible parental passive.
- Skills are chosen randomly from the parents' combined unique skills.
  Stimulation does not affect their inheritance chance or favor class skills.
  This does not copy every skill from both parents or increase skill-slot limits.
- Active skills lose their upgrades as in the base game. Skill Share itself
  is excluded from ordinary passive inheritance; its native upgraded-parent
  exception remains. With no eligible parental skill, the normal starting
  skill for that category is retained.
- A mutated parent part takes priority over an unmutated parent part.
- When both parents have a mutation in the same part, either can be inherited.
  Special mutations and birth defects are not ranked by strength.
- Each base stat inherits the higher of the parents' two values.
- Random loss of part inheritance at birth is disabled.
- New birth defects and newly generated birth disorders are prevented.
  Existing parental defects and disorders can still be inherited.

Left and right arms, legs, eyes, eyebrows and ears retain separate inherited
parts when either side has a mutation. When both inherited sides are ordinary,
the pair uses the game's normal mirroring, keeping their appearances matched.
This is decided separately for each pair. Unknown part definitions retain
their separate inherited appearances. If both parents have different mutations
on the same side, the kitten inherits one of them.
Matching paired mutations still count once, and the inherited head can hide
facial parts and their effects. No extra mutation slots are added.

## Event And Combat Mutations

For an occupied part, a stat-only mutation replaces the current mutation
automatically when no stat decreases and at least one increases. Equal changes
and pure downgrades keep the current mutation. A trade-off that increases some
stats and decreases others opens the selector. A +2/-1 is not automatically
better than a +1. Comparisons use combined mutation bonuses, counting matching
paired mutations once.

Head changes also account for mutations on facial parts they hide or restore.
Those affected parts appear in the same comparison; Replace Head accepts the
whole head change, not just the part currently shown.

If either mutation has a special effect, an in-game comparison lets you keep
the current mutation or accept the incoming one. Keeping it consumes the roll;
it does not reroll. Unknown mutation definitions are protected.

Each column shows inherited base stats plus active mutation bonuses before
and after the entire roll. Incoming values are green for increases, red for
decreases, and the normal color when unchanged. These previews exclude levels,
class bonuses, equipment, and temporary effects. Unreadable totals show
"Stats unavailable" rather than guessed values.

Descriptions appear in full without text paging. Arrows appear only for
different affected parts; matching paired mutations share one comparison.
If a description exceeds the panel's twelve-line limit, the mod keeps the
existing mutation rather than offering an incomplete comparison.

Empty parts keep the game's normal behavior, unless a head change also affects
an existing facial mutation.
Ordinary overnight house mutations are unchanged.

## Installation

Requires the Mewjector native loader, API 3 or later, and the supported Windows
Steam game build: 1.1.21239 / Steam build 25143593.

The download includes the DLL and the UI assets. There is no separate asset
download or mod named "Assets". The assets are `good_genes.swf` and
`swflist.gon.append` inside the archive's `GoodGenes/swfs/` folder.

### Vortex

1. Install the complete Good Genes ZIP, enable it under Mods, and deploy.
2. Open Load Order and enable the entry containing `GoodGenes`. This is the
   bundled asset folder; it may display the full mod title followed by
   `(GoodGenes)`.
3. Launch Mewgenics through Vortex so it supplies the enabled asset paths.

If that entry is missing, check that deployment created
`mods/GoodGenes/description.json` and `mods/GoodGenes/swfs/` in the game folder.
If those files are missing, reinstall the complete ZIP and deploy again.
Installing only `GoodGenes.dll` is insufficient.

### Mewtator

Import the complete ZIP and enable Good Genes, including its assets if the
manager shows separate native-code and asset controls. Keep the bundled
`GoodGenes` folder intact and launch through Mewtator.

### Manual installation

For manual installation, place GoodGenes.dll in the game's mods
directory and the GoodGenes folder beside it, retaining `description.json`,
`swfs/good_genes.swf`, and `swfs/swflist.gon.append` inside that folder.
Move the DLL out of the extracted GoodGenes folder for this layout; keep only
one installed copy of the DLL. Launch with
`-modpaths mods/GoodGenes` included among the enabled asset paths.
Preserve the paths for any other enabled assets.
For a Steam launch, add this argument in Mewgenics > Properties > General >
Launch Options. An entry in `modlist.txt` alone does not supply this argument.

Close the game before installing, updating, or removing the mod. Back up
your saves. Removing the mod does not undo mutations or kittens already saved.

The mod disables itself when the executable is unsupported, its required
hook locations have changed, or its matching UI assets are not enabled.
Avoid combining native mods that change the same breeding functions.

The comparison labels are English. Mutation descriptions use the game's
loaded text.

## Checking activation

In `mod_logs/chainloader.log`, look for `[GoodGenes] 0.3.2 enabled`.
Each birth also logs the higher parental base stats, selected parental skills
and inherited part IDs. Paired IDs are listed left/right before the game's
head visibility update; they do not count active mutation effects. An unknown
parental definition logs a fallback to native inheritance. A DLL-loading line
alone does not confirm activation.

## Source

[Source and issue reports](https://github.com/anitosq/mewgenics-mods/tree/main/mods/good-genes).
