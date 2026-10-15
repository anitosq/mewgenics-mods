# Change Soul Well's summon passives

Close the game and open this file in a text editor:

```text
SoulWell/data/abilities/item_abilities.gon.merge
```

Find `tk_SoulWell`, then its `spawn` section. Edit the contents of this block:

```gon
additional_passives {
    SafeDoomed 1
    InjuryImmunity 1
}
```

Add the same valid passive entries you use for your modified Soul Jar. Keep
the opening and closing braces and use the game's exact passive identifiers
and value syntax. If your modification grants an ability through a passive,
copy that passive entry, including any arguments it requires. An ability
name alone is not necessarily a valid passive entry.

Save as UTF-8 and restart the game. The DLL accepts edits inside this block,
including nested values, comments, and Windows line endings. It does not
validate whether a passive exists or what the passive does. Changes apply
to newly created shades.

Keep `SafeDoomed` and `InjuryImmunity` unless you intend to change the normal
temporary-shade behavior. The effects of removing them are untested.

## Where to edit

- Manual install: edit the file under the game's `mods/SoulWell` folder.
- Mewtator: edit the file in the Soul Well folder imported by your active profile.
- Vortex: edit the file in the mod's staging folder, then deploy. Editing only
  the deployed copy can be overwritten on the next deployment.

Keep a copy of your edited block before updating or reinstalling the mod.

## What stays fixed

Soul Well uses `tk_SoulWell`; the original Soul Jar uses `tk_SoulJar`.
Editing one does not edit the other. This package does not change the original
Soul Jar definition.

The native code depends on the surrounding summon definition, particularly
`template spawn`, `lob false`, `catdata item_aux_catid`, the shade object and
targeting. Leave everything outside the `additional_passives` block unchanged.
The startup check also expects the supplied item and shop files. If those
core files change, the DLL stays inactive and logs a data mismatch.

All files remain plain text. The source repository includes the DLL builder
for changes that go beyond passive customization; see `DEVELOPMENT.md` there.
