# Improved Inventory installation

Version 0.3.0 requires Windows x64, Steam build 25143593 (game version
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

For 0.3.0, extract the ZIP into the `mods` directory beside `Mewgenics.exe`,
preserving both components shown above. This is inside your game's installation
folder, wherever Steam installed it; it does not have to be on C: or in Program
Files. Enable the full
`mods/ImprovedInventory` path in the game's
`-modpaths` arguments alongside your other enabled mod paths. Mewjector must
be enabled and scanning `mods`. Starting from Steam without the configured
asset launch arguments will leave this mod inactive.

### Mewtator and custom mod folders (0.3.0)

Use the game's `mods` folder for this release. Extracting the ZIP into a separate
Mewtator mods folder does not work with Mewtator's normal DLL discovery: our DLL
sits beside the ImprovedInventory folder, while Mewtator scans inside enabled
mod folders. Moving the DLL inside that folder also fails the mod's current
asset check. Support for that layout needs a mod update.

Keep the DLL beside the ImprovedInventory folder and enable its asset folder
in the launch options as described above. One player reports this manual setup
working with Mewtator; we have reproduced the custom-folder discovery problem
in an isolated check, but have not run a full Mewtator gameplay test.

## Updating and removing

Close the game before changing the mod. Update the whole archive through
Vortex, keep one version enabled, and deploy. The DLL and SWFs must come from
the same archive; mismatched files leave the mod inactive.

To uninstall, disable/remove Improved Inventory in Vortex and deploy. For a manual
installation remove only `mods/ImprovedInventory.dll` and `mods/ImprovedInventory/`, and
remove its asset launch argument.

## Troubleshooting

Look for ImprovedInventory and its version in `mod_logs/chainloader.log`.
"Improved Inventory enabled; all UI hooks installed" confirms native startup.
If it reports inactive assets, deploy the complete package and check that
ImprovedInventory is enabled in Vortex's Load Order page. Launch with Vortex's
default Mewgenics tool (called Custom Launch in Tools). The
[game-support extension](https://www.nexusmods.com/site/mods/1691) supplies the
mod launch arguments; it is already present in a working Vortex setup.
If it reports an unsupported executable or a hook
entry mismatch, use a compatible release and report the game build and
relevant log lines.

## Compatibility

Mewjector is the only mod dependency; runtime v3.0 was used for testing.
Controller and IME input and broader resolution/UI-scale coverage haven't been
tested. Manual installation has a player report, with the Mewtator limitation
described above; our in-game release checks used Vortex.
In-game performance with very large inventories
also needs testing. Other game builds are unsupported.

Item transfers are still manual, and End Day still deletes items left in Trash.

See the [release notes](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/releases/0.3.0.md)
for features and the [test checklist](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/RELEASE_CHECKLIST.md)
for detailed coverage.
