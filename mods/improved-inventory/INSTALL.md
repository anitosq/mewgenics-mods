# Improved Inventory installation

Version 0.3.1 requires Windows x64, Steam build 25143593 (game version
1.1.21239), and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218)
API v3 (tested with runtime v3.0). The mod stays inactive on other game builds.

## Vortex

1. Close Mewgenics. Install and enable Mewjector if you don't already use it.
2. Use Mod Manager Download on Nexus, or install `ImprovedInventory-<version>.zip`
   with Vortex's Install From File. Enable and deploy it.
3. Launch Mewgenics through Vortex.

If Vortex already manages your Mewgenics mods, you don't need to download a
separate extension for Improved Inventory. For a new setup, add Mewgenics in
Vortex's Games page and follow its setup prompts.

The archive deploys two components together:

```text
Mewgenics/mods/ImprovedInventory.dll
Mewgenics/mods/ImprovedInventory/description.json
Mewgenics/mods/ImprovedInventory/swfs/improved_inventory.swf
Mewgenics/mods/ImprovedInventory/swfs/swflist.gon.append
```

Mewjector's default `ScanPath=mods` finds the DLL automatically. Vortex enables
the asset folder through `-modpaths`. If you use a custom scan path, include
`mods/ImprovedInventory.dll`. Keep only one copy of the mod installed.

Unchecking the asset folder in Load Order leaves the installed DLL inactive
at the next launch. Use Disable and Deploy to remove both components from
the game. Changes take effect after restarting the game.

## Manual installation

Extract `ImprovedInventory-0.3.1.zip` into the `mods` directory beside `Mewgenics.exe`,
preserving both components shown above. This is inside your game's installation
folder, wherever Steam installed it; it does not have to be on C: or in Program
Files. Enable the full
`mods/ImprovedInventory` path in the game's
`-modpaths` arguments alongside your other enabled mod paths. Mewjector must
be enabled and scanning `mods`. Starting from Steam without the configured
asset launch arguments will leave this mod inactive.

## Mewtator

1. Close Mewgenics and download `ImprovedInventory-0.3.1-Mewtator.zip`.
2. Extract its `ImprovedInventory` folder into your configured Mewtator mods
   directory. The DLL, `description.json` and `swfs` must be inside that folder.
3. Enable Improved Inventory and DLL Mod Support in Mewtator. Mewjector must
   be installed in the game directory.
4. Launch the game through Mewtator.

Your Mewtator mods directory can be outside the game directory. Its package has
this layout:

```text
ImprovedInventory/ImprovedInventory.dll
ImprovedInventory/description.json
ImprovedInventory/swfs/improved_inventory.swf
ImprovedInventory/swfs/swflist.gon.append
```

Use only one download. If switching from the Vortex or manual package, remove
that installation first, including the old `mods/ImprovedInventory.dll`. Vortex
users should remove it through Vortex and deploy before switching managers.

## Updating and removing

Close the game before changing the mod. Update the whole archive through
Vortex, keep one version enabled, and deploy. For Mewtator, replace the whole
ImprovedInventory folder with the new Mewtator download. The DLL and SWFs must come from
the same archive; mismatched files leave the mod inactive.

To uninstall, disable/remove Improved Inventory in Vortex and deploy. For a manual
installation remove only `mods/ImprovedInventory.dll` and `mods/ImprovedInventory/`, and
remove its asset launch argument.
For Mewtator, disable the mod and remove its ImprovedInventory folder.

## Troubleshooting

Look for ImprovedInventory and its version in `mod_logs/chainloader.log`.
"Improved Inventory enabled; all UI hooks installed" confirms native startup.
If it reports that the asset folder is not enabled, check Improved Inventory
in your manager's mod list (Vortex's Load Order page). If it reports missing or
mismatched UI assets, reinstall the complete download for your manager.
An ambiguous-folder message means both folder layouts are enabled; remove the
old installation and keep one asset path.

For Vortex, launch with its
default Mewgenics tool (called Custom Launch in Tools). The
[game-support extension](https://www.nexusmods.com/site/mods/1691) supplies the
mod launch arguments; it is already present in a working Vortex setup.
If it reports an unsupported executable or a hook
entry mismatch, use a compatible release and report the game build and
relevant log lines.

## Compatibility

Mewjector is the only mod dependency; runtime v3.0 was used for testing.
Controller and IME input and broader resolution/UI-scale coverage haven't been
tested. See the test checklist for manager and gameplay test coverage.
In-game performance with very large inventories
also needs testing. Other game builds are unsupported.

Item transfers are still manual, and End Day still deletes items left in Trash.

See the [release notes](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/releases/0.3.1.md)
for features and the [test checklist](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/RELEASE_CHECKLIST.md)
for detailed coverage.
