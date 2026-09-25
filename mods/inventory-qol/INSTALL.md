# Inventory QoL installation

This is a beta candidate. Supported platform: Windows x64, Steam build
25143593. Other executable builds are rejected by the compatibility check.
The required loader is [Mewjector](https://www.nexusmods.com/mewgenics/mods/218),
API v3 or newer; v3.0 is the tested baseline.

## Vortex

1. Close Mewgenics. Install Mewjector and the
   [Mewgenics Vortex extension](https://www.nexusmods.com/site/mods/1691).
2. Install the `InventoryQoL-<version>.zip` archive using **Install From File**
   (or Mod Manager Download once available on Nexus). Enable and deploy it.
3. Ensure **InventoryQoL** is checked in the game's Vortex Load Order page.
4. Launch with Vortex's **Custom Launch** tool and open Storage/Trash.

The archive deploys two components together:

```text
Mewgenics/mods/InventoryQoL.dll
Mewgenics/mods/InventoryQoL/description.json
Mewgenics/mods/InventoryQoL/swfs/inventory_qol.swf
Mewgenics/mods/InventoryQoL/swfs/swflist.gon.append
```

Mewjector's normal `ScanPath=mods` discovers the DLL; Vortex enables the asset
folder through `-modpaths`. No extra numbered loader entry is needed with
that standard scan setting. If your scan path is customized, configure the
loader to find `mods/InventoryQoL.dll` without replacing your existing settings.
Do not add a second copy in another folder or install the old diagnostic DLL.

Unchecking the asset folder in Load Order leaves the installed DLL inactive
at the next launch. Use Disable and Deploy to remove both components from
the game. Changes take effect after restarting the game.

## Manual installation

Extract the ZIP into the game's `mods` directory, preserving both components
shown above. Enable the full `mods/InventoryQoL` path in the game's
`-modpaths` arguments alongside your other enabled mod paths. Mewjector must
be enabled and scanning `mods`. Starting from Steam without the configured
asset launch arguments will leave this mod inactive.

## Updating and removing

Close the game before changing the mod. Update the whole archive through
Vortex, keep one version enabled, and deploy. Do not mix an older DLL with
newer SWFs; the startup gate deliberately rejects mismatched assets.

To uninstall, disable/remove Inventory QoL in Vortex and deploy. For a manual
installation remove only `mods/InventoryQoL.dll` and `mods/InventoryQoL/`, and
remove its asset launch argument. Leave Mewjector and other mods intact.
The mod has no direct save writer; ordinary in-game item transfers and End Day
retain their normal game effects.

## Troubleshooting and scope

Inspect `mod_logs/chainloader.log` for **InventoryQoL** and the version number.
"Inventory QoL enabled; all UI hooks installed" confirms native startup.
If it reports inactive assets, deploy the complete package, check Load Order
and use Custom Launch. If it reports an unsupported executable or a hook
entry mismatch, the mod remains inactive: use a compatible release and report
the game build and relevant log lines. Do not bypass the compatibility guard.

Other gameplay mods, Mewtator and Mewgenics Mod Manager are not dependencies.
No bulk trash operation is included. Controller/IME support, other game builds
and the full installation lifecycle remain unverified for this candidate.
