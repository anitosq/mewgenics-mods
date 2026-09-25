# Inventory QoL release readiness

Status: preparation; no installable archive or public release. Updated
25 September 2026. This checklist distinguishes existing development evidence
from checks that still need to be performed on a packaged build.

## Completed foundations

- [x] Native Storage/Trash scrolling, filters, search and set browsing implemented.
- [x] Native font, rarity art references and sound feedback verified locally.
- [x] Fixture tests cover filtering, search and set-cache invalidation.
- [x] Supported-executable guard tested against matching and unrelated files.
- [x] Original code/documentation MIT licensed; third-party notices retained.
- [x] Shared release policy and automated fixture-check workflow added.

## Release startup and packaging: next implementation milestone

- [ ] Add a release build mode and stable `InventoryQoL.dll` name/version.
      Activate normally without a fresh session marker, environment switch,
      absolute development path or development runner. Retain diagnostic mode.
- [ ] Keep the exact supported-image guard and useful version/compatibility
      messages. Validate every required hook entry before enabling changes;
      handle partial hook installation and missing UI assets safely. Current
      initialization can install some hooks before later checks fail.
- [ ] Register the original UI SWF and `swflist.gon.append` through the normal
      deployed mod folder and Vortex launch arguments.
- [ ] Implement the per-mod version source, release build and allowlisted
      packager. Include instructions, license/notices, checksums and a public
      provenance summary; exclude all diagnostic/session/private files.
- [ ] Validate the archive against the installed Mewgenics Vortex extension
      0.4.0, then verify a real Vortex installation. A proposed simple layout is
      one stable `InventoryQoL/` folder with `description.json`, the DLL and
      `swfs/`; its installer behavior is source-supported, not runtime-tested.
- [ ] Settle the DLL registration method. A nested DLL is not discovered by
      Mewjector v3's nonrecursive `ScanPath=mods`. Current Vortex handles asset
      launch paths but does not generate a DLL manifest or edit `chainloader.ini`.
      An explicit `ModN=InventoryQoL\InventoryQoL.dll` entry is a viable
      fallback: keep `ScanPath`, preserve other entries and use consecutive
      numbers. Document the one-time setup/removal if retained. Do not ship a
      replacement loader config, bundle a second loader or claim one-click
      installation until it is demonstrated.

## Packaged-build test matrix

- [ ] Clean setup: supported game + Mewjector v3.0 + only Inventory QoL,
      launched through Vortex's Custom Launch, with both DLL and assets loaded.
- [ ] Existing mod collection: set markers/tooltips and item overrides coexist.
- [ ] Fresh install, upgrade, reinstall, disable, re-enable, purge/redeploy,
      remove and launch without the mod; no duplicate DLL or stale activation.
- [ ] Vortex load-order unchecking separately from disabling the deployed mod:
      assets may be omitted while a DLL remains loadable. Fail safely.
- [ ] Search, type, rarity and set combinations, empty results, reset and
      dropdown input blocking on both panels; scroll and arrows at boundaries.
- [ ] Populate Trash beyond 36 items, return items, reopen and save/reload using
      a temporary campaign copy. Preserve pre-existing saves and loader setup.
- [ ] Inventory UI creation/teardown and game exit; distinguish baseline exit
      exceptions from regressions introduced by this packaged build.
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
failure recovery. Source inspection for the packaging notes used the locally
installed extension's `testMod`/`installMod` and `testMewjectorMod` functions,
installer registration order, plus the loader research. Do not redistribute
the extension implementation. [Extension author page](https://www.nexusmods.com/site/mods/1691).
