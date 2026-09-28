# Improved Inventory installation

Version 0.3.2 requires Windows x64, Mewgenics 1.1.21239 (Steam build
25143593), and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218)
API v3. The same ZIP works with Vortex and Mewtator.

## Vortex

1. Close the game. Install and enable Mewjector if you don't already use it.
2. Use Mod Manager Download on Nexus, or install the ZIP with Install From File.
3. Enable and deploy Improved Inventory, then launch Mewgenics through Vortex.

**Updating from 0.3.1 or earlier:** remove the old version in Vortex and deploy
before installing 0.3.2. This clears the old installation before Vortex places
the new package.

If Vortex already manages your Mewgenics mods, you don't need a separate
extension download. For a new setup, add Mewgenics on Vortex's Games page and
follow its setup prompts.

## Mewtator

1. Close the game and import the ZIP into Mewtator. You can also extract just
   its `ImprovedInventory` folder into your configured mods directory.
2. Enable Improved Inventory and DLL Mod Support. Mewjector must be installed
   in the game directory.
3. Launch the game through Mewtator.

Your mods directory can be outside the game folder. When updating, replace
the existing ImprovedInventory folder. The `fomod` folder is only for Vortex;
Mewtator's ZIP importer skips it automatically.

If switching managers, remove the previous installation through that manager
first. Vortex users should deploy after removing it.

## Manual installation without a manager

Extract the `ImprovedInventory` folder into `Mewgenics/mods`, then move
`ImprovedInventory.dll` out of that folder into `Mewgenics/mods`:

```text
Mewgenics/mods/ImprovedInventory.dll
Mewgenics/mods/ImprovedInventory/description.json
Mewgenics/mods/ImprovedInventory/swfs/improved_inventory.swf
Mewgenics/mods/ImprovedInventory/swfs/swflist.gon.append
```

Keep Mewjector enabled and scanning `mods`. Include the full path to
`mods/ImprovedInventory` in the game's `-modpaths` launch arguments alongside
your other enabled mods. Do not copy the `fomod` folder into the game.

## Removing the mod

Close the game first. In Vortex, remove Improved Inventory and deploy. In
Mewtator, disable it and remove its ImprovedInventory folder. For a manual
installation, remove `mods/ImprovedInventory.dll`, `mods/ImprovedInventory/`
and its asset launch argument.

## Troubleshooting

Look for ImprovedInventory and its version in `mod_logs/chainloader.log`.
"Improved Inventory enabled; all UI hooks installed" confirms native startup.

If the asset folder is not enabled, check the mod in your manager's mod list
(Vortex's Load Order page). Launch through your manager so the game receives
the asset paths. For missing or mismatched assets, reinstall the complete ZIP.
Keep only one copy installed; the DLL and SWFs must come from the same archive.

The mod stays inactive on unsupported game builds. If the log reports an
unsupported executable or hook entry mismatch, report the game build and the
relevant log lines.

## Compatibility

Mewjector is the only mod dependency; runtime v3.0 was used for testing.
Controller, IME input and broader resolution/UI-scale coverage haven't been
tested. Item transfers are still manual, and End Day deletes items left in Trash.

See the [test checklist](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/RELEASE_CHECKLIST.md)
for detailed coverage.
