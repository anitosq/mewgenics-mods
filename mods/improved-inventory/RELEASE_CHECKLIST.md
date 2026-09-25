# Improved Inventory release readiness

Status: beta.2 user-tested with only Mewjector and Improved Inventory active;
startup and test actions verified in logs. No public release. Updated
25 September 2026. This checklist distinguishes existing development evidence
from checks that still need to be performed on a packaged build.

## Completed foundations

- [x] Native Storage/Trash scrolling, filters, search and set browsing implemented.
- [x] Native font, rarity art references and sound feedback verified locally.
- [x] Fixture tests cover filtering, search and set-cache invalidation.
- [x] Supported-executable guard tested against matching and unrelated files.
- [x] Original code/documentation MIT licensed; third-party notices retained.
- [x] Shared release policy and automated fixture-check workflow added.

## Release startup and packaging

- [x] Add a release build mode and stable `ImprovedInventory.dll` name/version.
      Activate normally without a fresh session marker, environment switch,
      absolute development path or development runner. Retain diagnostic mode.
- [x] Retain the exact image guard and version messages. All hook entries are
      checked first; UI activation is committed only after all hooks install.
      Fault tests cover every partial-install position. The release startup
      gate requires the exact enabled asset path and matching SWF/append hashes.
- [x] Implement per-mod VERSION, reviewed compatibility metadata and allowlisted
      packaging from a clean commit, with ZIP/payload checksums and provenance.
- [x] Verify normal UI asset loading and actual Vortex deployment of the archive.
      The DLL lives at `mods/ImprovedInventory.dll`; assets live in `mods/ImprovedInventory/`.
      The installed extension handles this mixed payload as one mod installation.
      Deployment hashes match the candidate; Vortex excludes CHANGELOG.md.
- [x] Use Mewjector's normal `ScanPath=mods` discovery for the top-level DLL.
      No numbered loader entry or configuration replacement is packaged.
      The native startup gate stays inactive when the asset folder is unchecked
      from Vortex Load Order in guard fixtures. Real deployment passed;
      disable/re-enable/remove/reinstall and separate asset load-order
      unchecking passed user testing; the inactive startup log was verified.

## Packaged-build test matrix

- [x] Isolated mod setup: supported game + Mewjector v3.0 + only Improved Inventory,
      launched through Vortex's Custom Launch, with both DLL and assets loaded.
      Verified beta.2 user session on 25 September. The disabled BiggerWallet
      priority entry failed to load, leaving only our DLL. This was an isolated
      profile on the existing installation, not a fresh game/loader install.
- [ ] Existing mod collection: set markers/tooltips and item overrides coexist.
      Initial smoke test passed with the collection enabled: markers and the
      Transmitter Set tooltip rendered. Broader override coverage remains.
- [x] Disable/deploy/launch, re-enable/deploy/launch, remove/deploy/launch,
      and reinstall/deploy/launch: user confirmed all four steps worked.
      Final reinstall log and payload hashes independently verified; only
      one mod DLL is deployed. Earlier step logs were not separately retained.
- [ ] Same-name version upgrade and explicit purge/redeploy. First installation
      and the rename migration were tested separately; neither proves these paths.
- [x] Vortex load-order unchecking separately from disabling the deployed mod:
      assets may be omitted while a DLL remains loadable. Fail safely.
      User confirmed expected behavior. Latest log independently verifies
      inactive startup with assets unchecked, followed by loader detachment.
- [ ] Search, type, rarity and set combinations, empty results, reset and
      dropdown input blocking on both panels; scroll and arrows at boundaries.
- [ ] Populate Trash beyond 36 items, return items, reopen and save/reload using
      a temporary campaign copy. Preserve pre-existing saves and loader setup.
      Beta.2 logs verify 37 in Trash, scrolling on both panels, return to 92/0,
      and total count 92 throughout. Save/reload and backup status were not
      independently verified for that user-run session.
- [ ] Inventory UI creation/teardown and game exit; distinguish baseline exit
      exceptions from regressions introduced by this packaged build.
      No exit exception or matching crash report in the isolated beta.2 run.
      Earlier automation-assisted runs reported UIAutomationCore-heavy raw
      stacks, including without our DLL; causation remains unproven.
- [ ] Supported window sizes/UI scales and input behavior; explicitly document
      any controller, keyboard-layout, IME or Unicode editing limitations.
- [ ] Missing assets, missing/older loader, unsupported executable, conflicting
      hooks and duplicate installation produce a safe, diagnosable outcome.
- [ ] Large-inventory responsiveness and stable counts after transfers/rebinds.

## Publication

- [ ] Finalize the supported Steam build ID/hash and tested dependency versions.
- [ ] Write and test player install/update/uninstall instructions.
- [ ] Capture representative in-game screenshots and finalize the Nexus listing.
- [ ] Tag tested commit; attach frozen ZIP/checksum/notes to a GitHub draft.
- [ ] Upload the same archive to Nexus; set version, requirements and MIT permissions.
- [ ] Verify both downloads and Vortex mod-manager installation; record Nexus IDs.
- [ ] Complete the release record and cross-link both destinations.

See the [release workflow](../../docs/releases/README.md) for publishing and
failure recovery and the [candidate test record](../../docs/releases/improved-inventory/0.1.0-beta.2-candidate.md)
for observed results and remaining checks. Source inspection for the packaging notes used the locally
installed extension's `testMod`/`installMod` and `testMewjectorMod` functions,
installer registration order, plus the loader research. Do not redistribute
the extension implementation. [Extension author page](https://www.nexusmods.com/site/mods/1691).
