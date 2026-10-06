# Better Breeding Plus

Pass mutations to kittens and protect occupied mutation slots during events
and combat.

## Breeding

- A mutated parent part takes priority over an unmutated parent part.
- When both parents have a mutation in the same part, either can be inherited.
  Special mutations and birth defects are not ranked by strength.
- Each base stat inherits the higher of the parents' two values.
- Random loss of part inheritance at birth is disabled.
- New birth defects and newly generated birth disorders are prevented.
  Existing parental defects and disorders can still be inherited.

The game's paired-part mirroring remains in place. This is not a guarantee
that every distinct mutation on an asymmetric parent appears on its kitten.

## Event And Combat Mutations

For an occupied part, a stat-only mutation replaces the current mutation only
when no stat decreases and at least one increases. Equal changes and trade-offs
keep the current mutation. A +2/-1 is not automatically better than a +1.

If either mutation has a special effect, an in-game comparison lets you keep
the current mutation or accept the incoming one. Keeping it consumes the roll;
it does not reroll. Unknown mutation definitions are protected.

Empty parts keep the game's normal behavior, with no comparison window.
Ordinary overnight house mutations are unchanged.

## Installation

Requires the Mewjector native loader, API 3 or later, and the supported Windows
Steam game build: 1.1.21239 / Steam build 25143593.

Install and enable the archive through your mod manager. Enable its asset
entry in the game's mod load order as well as the native DLL.

For manual installation, place BetterBreedingPlus.dll in the game's mods
directory and the BetterBreedingPlus folder beside it. Launch with
`-modpaths mods/BetterBreedingPlus` included among the enabled asset paths.
Preserve the paths for any other enabled assets.

Close the game before installing, updating, or removing the mod. Back up
your saves. Removing the mod does not undo mutations or kittens already saved.

The mod disables itself when the executable is unsupported, its required
hook locations have changed, or its matching UI assets are not enabled.
Avoid combining native mods that change the same breeding functions.

The comparison labels are English. Mutation descriptions use the game's
loaded text.

## Source

[Source and issue reports](https://github.com/anitosq/mewgenics-mods/tree/main/mods/better-breeding-plus).
