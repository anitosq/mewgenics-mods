# Improved Inventory: Mewtator folder compatibility

Investigated 28 September 2026 after a player reported that 0.3.0 worked in
the game's mods folder but not a separately configured Mewtator mods folder.
No player log was supplied, so their exact failing startup stage is unknown.

## Confirmed cause in our package

There is no hard-coded Steam installation directory. The startup check in
`inventory_startup.h` derives an ImprovedInventory asset folder next to the
loaded DLL, requires that exact folder in `-modpaths`, and verifies its SWF and
append hashes. An external directory passes those checks with the shipped layout.

However, the package puts ImprovedInventory.dll beside ImprovedInventory/, not
inside it. Mewtator's DLL service recursively scans each enabled mod folder, so
it does not discover our sibling DLL. Moving that DLL into the asset folder
makes discovery work but causes our gate to look one folder too deep.

Checked current upstream HEAD against the local source checkout:
`dd48fd8c04c9257af346363bd8fc7384308fd587`.
[Mewtator DLL discovery](https://github.com/dancomstock/mewtator/blob/dd48fd8c04c9257af346363bd8fc7384308fd587/app/core/services/dll_injection_service.py#L83).
Mewtator creates a manifest for Mewjector when DLL Mod Support is enabled;
its Windows launch strategy supplies enabled folders through `-modpaths`.

An isolated reproduction used the frozen public 0.3.0 ZIP, its exported startup
validation function, and the upstream discovery service in a temporary directory:

- External sibling layout: asset gate accepts; Mewtator finds zero DLLs.
- DLL inside ImprovedInventory/: Mewtator finds it; asset gate rejects.

Evidence script: `work/check_mewtator_layout.py`. No game launch, save, manager
configuration or installed mod was changed. These checks establish the layout
mismatch, not full Mewtator gameplay compatibility.

## Proposed patch milestone

Keep the Vortex/default-scan archive layout and add a Mewtator archive with
ImprovedInventory/ containing the DLL, description.json and swfs/ together.
Build the DLL/assets once and package those identical payloads in both layouts.
Accept the DLL's containing asset folder as well as the old sibling layout,
while still requiring the selected folder in `-modpaths` and matching asset
hashes. Keep executable/hook guards intact. Report missing enabled paths
separately from missing or mismatched assets.

A single nested package would regress default Mewjector discovery:
its ScanPath scan is nonrecursive. Confirmed at current upstream
`ccdd6813cef0f51342eb74c0cecb47654f7dbeef`,
[ScanAndLoadDir](https://github.com/githubuser508/mewjector/blob/ccdd6813cef0f51342eb74c0cecb47654f7dbeef/version.c#L930).
Prefer two clearly named installation variants over asking every Vortex user
to change chainloader.ini. A universal archive needs a verified manager/loader
mechanism first; do not assume recursive scanning.

Before releasing a patch:

1. Add regression cases for both layouts, external paths with spaces, disabled
   paths, missing assets and mismatched pairs. Exercise real Mewtator discovery
   and manifest generation in a temporary directory.
2. Verify the existing-layout archive with the installed Vortex extension and
   default Mewjector scan. Test upgrading from 0.3.0 and switching variants;
   confirm no old DLL remains to load twice. Do not install both variants.
3. Test Mewtator with an external mods directory and DLL Mod Support enabled,
   plus Vortex's normal layout. Use a copied campaign for Storage/Trash and
   equipment-selection checks, including disable/re-enable.
4. Publish a new patch version, mirroring each tested archive unchanged to both
   hosts. Keep the Vortex archive Main/primary and clearly label the Mewtator
   variant. Leave published 0.3.0 artifacts unchanged. Then replace the
   temporary listing note.

The 0.3.1 patch implements both layouts and two archives; validation is tracked
in the [candidate record](../releases/improved-inventory/0.3.1-candidate.md).
Version 0.3.1 is now published with both layouts. A subsequent
[single-archive FOMOD experiment](improved-inventory-universal-package.md)
passed file-placement checks but exposed Vortex upgrade/removal problems;
it has not replaced the supported downloads.
