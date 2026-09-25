# Improved Inventory installation

Version 0.1.0 requires Windows x64, Steam build 25143593 (game version
1.1.21239), and [Mewjector](https://www.nexusmods.com/mewgenics/mods/218)
API v3 (tested with runtime v3.0). The mod stays inactive on other game builds.

## Vortex

1. Close Mewgenics. Install Mewjector and the
   [Mewgenics Vortex extension](https://www.nexusmods.com/site/mods/1691).
2. Use Mod Manager Download on Nexus, or install `ImprovedInventory-<version>.zip`
   with Vortex's Install From File. Enable and deploy it.
3. Check ImprovedInventory in the game's Vortex Load Order page.
4. Start the game with Vortex's Custom Launch tool and open Storage/Trash.

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

This method hasn't been tested. Extract the ZIP into the game's `mods`
directory, preserving both components shown above. Enable the full
`mods/ImprovedInventory` path in the game's
`-modpaths` arguments alongside your other enabled mod paths. Mewjector must
be enabled and scanning `mods`. Starting from Steam without the configured
asset launch arguments will leave this mod inactive.

## Updating and removing

### Moving from the former Inventory QoL candidate

Close the game, remove the former Inventory QoL installation in Vortex and
deploy before installing Improved Inventory. The DLL and asset folder have
new names, so installing both archives creates two copies of the mod.
For the old manual installation, remove only `mods/InventoryQoL.dll` and
`mods/InventoryQoL/`, and its old asset launch argument, before following the
installation instructions above.

Close the game before changing the mod. Update the whole archive through
Vortex, keep one version enabled, and deploy. The DLL and SWFs must come from
the same archive; mismatched files leave the mod inactive.

To uninstall, disable/remove Improved Inventory in Vortex and deploy. For a manual
installation remove only `mods/ImprovedInventory.dll` and `mods/ImprovedInventory/`, and
remove its asset launch argument.

## Troubleshooting

Look for ImprovedInventory and its version in `mod_logs/chainloader.log`.
"Improved Inventory enabled; all UI hooks installed" confirms native startup.
If it reports inactive assets, deploy the complete package, check Load Order
and use Custom Launch. If it reports an unsupported executable or a hook
entry mismatch, use a compatible release and report the game build and
relevant log lines.

Mewjector is the only mod dependency. See the
[release notes](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.1.0)
for current limits and the
[test checklist](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/RELEASE_CHECKLIST.md)
for coverage.
Items left in Trash are still deleted when you end the day.
