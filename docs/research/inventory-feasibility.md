# Improved Inventory: first implementation milestone

Historical research from the first prototype. For the released mod, use the
[current README](../../mods/improved-inventory/README.md) and
[release checklist](../../mods/improved-inventory/RELEASE_CHECKLIST.md).

24 September 2026. Stage 0 is partially complete. Experimental native scrolling and All / Consumables / rarity / Reset controls run in the existing Storage/Trash screen; this is not the first playable release. A temporary campaign copy verified combined filters, empty results, Reset, Escape dismissal, 37 transfers into Trash, immediate scrolling beyond 36 items, background retention, returning all items, and reopening. The main save stayed byte-identical and loader configuration was restored afterward. Broader filters, input coverage, and lifecycle hardening remain. See [native notes](native-inventory-notes.md) for current evidence and the exit-time exception also reproduced without our DLL.

## What exists

- `mods/improved-inventory/src/browser/`: readable scrolling cards, compact/comfortable density, expanded horizontal search/type/location bar, a “More filters” dialog for slot/rarity/set choices, removable applied-filter chips, stable sorting, and per-copy inspection. Apply commits dialog choices; Cancel/Escape preserves the applied filters. Filters combine with AND across categories and OR within selected rarities. Unknown set membership is excluded from “No set.” There are no transfer or save-writing controls.
- `tools/probe_inventory.py`: bounds-checked saved-inventory parser and installed-build inspection. SQLite is opened read-only and copied into memory for a consistent read; only inventory records are exported locally.
- `tools/build_preview.py`: joins the saved records to installed base item definitions and English names. Explicit relevant metadata overrides in active mods are flagged rather than guessing engine merge precedence.
- `mods/improved-inventory/src/native/inventory_probe.c`, `inventory_layout.h`, `inventory_controls.h`, and `inventory_filter.h`: a guarded Mewjector diagnostic DLL with passive observation and optional experimental scrolling/filtering. A fresh `layout-test` marker enables a six-column cap, wheel input, and native controls using generated SWF assets. Runtime item definitions supply consumable status; native drawer metadata supplies rarity. The default runner remains observational. A PE/byte-guarded startup callback defers initialization to the game thread. Observer/layout hooks require the full executable hash and matching entry bytes. Loading the DLL alone still installs its guarded startup callback; it is not a gameplay package.
- `tools/inspect_inventory_native.py`: read-only PE string-reference and disassembly report. `tools/run_inventory_probe.py`: backs up saves/settings/config, temporarily enables the passive probe, starts through Steam, and restores the configuration after exit if no external deployment changed it. `tools/inspect_probe_dump.py`: local crash-dump triage; its raw stack candidates are not a proper unwind.

The browser is a separate development harness. It does not replace the inventory screen, preserve game tooltips, show item icons, or synchronize with the running game.

## Local evidence

| Check | Result |
| --- | --- |
| Installed executable | Steam build 25143593; SHA-256 `4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea` |
| UI reference signatures | All 44 reference function entries matched their expected 64 bytes in the installed executable; this establishes byte compatibility, not runtime correctness |
| UI asset names | `house.swf` contains `StorageMenu`, `TrashMenu`, filters and sorting symbols; `ui.swf` contains inventory grid and tooltip symbols |
| Saved inventory | 94 storage items, 0 backpack, 0 trash; 93 distinct item types in the selected save snapshot dated 2026-09-24 00:01:04 local time |
| Consumables | 3 records; adding the Rare filter yields 2 |
| Set search | “bionic” yields 7 records in this snapshot |
| Reader/metadata tests | 9 Python tests passed, including truncation at every byte boundary and unsupported versions |
| Filter tests | 4 Node tests passed, including combined filters, multi-set membership, unknown metadata, and duplicate identities |
| Large-grid check | A synthetic 1,000-item fixture scrolls to the last row without shrinking cards; no browser console errors observed. This is not an in-game performance measurement |
| Native guard | Compiles with warnings treated as errors; a separate non-game process accepts the researched executable and rejects unrelated/missing files |
| Native runtime | Bootstrap, renderer, and grid hooks loaded; opening Storage/Trash produced `InventoryGridBGBox` renderer events and grid mode 0, columns 10/4, matching the screen |
| Browser filter dialog | Consumables + Rare gives 2 results; Cancel and Escape discard edits; removing Rare returns 3; Clear returns 94; no console warnings/errors observed |
| Native filters | Item consumables 3; + Rare 2; Weapon consumables 3; Worn 7; Broken 0; Reusable weapons 16; Reset 92 native drawers. Open dropdowns block item tooltips, clicks and wheel input |
| Native scroll arrows | Clickable directional arrows follow animated panel positions; Storage top/middle/bottom states verified, Trash arrow appears immediately at 37 items and disappears after returning to 36 |
| Native Trash regression | 37 items transferred into the temporary copy's Trash; backgrounds retained and wheel scrolling immediately reaches row 1/1; returning item 37 clamps row to 0; all items returned |
| Latest session cleanup | Original loader restored; marker and temporary slot removed; all pre-existing save files byte-identical. Test copy retains 94 Storage records with item properties conserved except ordering metadata |

Saved inventory may lag the running household. Snapshot-local instance IDs combine container, record position, and record hash; they are not persistent live identities. Raw fields whose meaning is unverified remain uninterpreted. Worn/broken condition remains unknown in the saved-record browser parser; the native mod now reads the verified per-item runtime condition field.

Metadata inspection is deliberately limited: it checks explicit fields in active item files, but does not implement all replacement/deletion, inheritance, patch, or runtime semantics. Base localization is used without numeric set-marker prefixes. Runtime effective definitions must replace this approximation before a gameplay release, particularly before any cleanup action.

## How public source helped

- [The Spreadsheet Edmund Hates](https://github.com/Ivanca/the-spreadsheet-edmund-hates), inspected at `81ca05535575810cd35e2827c50130ecadaed9fd`: demonstrates custom UI using the game's renderer and supplies address/signature research. No license file was present; no implementation or assets were copied into our product and no supplied binaries or install scripts were executed.
- [Mewgenics Savegame Editor](https://github.com/michael-trinity/mewgenics-savegame-editor), inspected at `1217fc539314de3414ad09ef4e8d45ecd5f986c1`: MIT-licensed inventory serialization reference. The notice is retained in `THIRD_PARTY_NOTICES.md`; our tool has no save writer.
- [Mewjector](https://github.com/githubuser508/mewjector), inspected at `ccdd6813cef0f51342eb74c0cecb47654f7dbeef`: source verifies the installed v3 logging and hook ABI. Our probe uses that loader rather than introducing a second proxy DLL.

These references materially reduce blind reverse engineering, but their addresses, layouts, and behavior still need verification against this game build and mod collection.

## Native screen findings

For this exact executable, StorageMenu constructor RVA `215490` and TrashMenu constructor `215650` both call shared initialization `204030`. Shared grid layout `211700` resolves `ref_grid` and creates `InventoryGridBGBox`. Its loop at `2119C8`–`211A11` increases the number of columns until square-grid capacity fits, capped at 20. This supports the reported shrinking behavior. Shared sorting is at `2126D0`.

The grid observer read mode at instance offset `48` and storage/trash columns at `50`/`54`. Further layout work established `64` as the shared item-drawer count (92) and `74` as the Storage background count (100). Empty Trash has 16 backgrounds. The save has 94 serialized records; the difference from 92 native drawers remains unresolved. The experiment operates on existing drawers and their native identities, without manufacturing missing entries.

The first worker-thread bootstrap aborted in the game's thread-local initialization before entering our initialization function. Mewjector's source also describes this early-thread/CRT issue. The successful version uses the guarded game startup function at RVA `9B9970` and creates no worker thread. Full observer validation happens there, outside the loader callback. Steam handoff means the initial launcher process can exit before the actual game starts; a pending Steam launch-arguments prompt also needed completing during diagnosis. An initial process exit alone is not evidence that the game failed.

Successful session evidence is in `work/backups/inventory-observation-20260924-152258/`, including saves, original configuration, save comparison, and chainloader log. The normal save database changed on exit; the parsed inventory stayed at 94 records and every inventory blob hash matched the backup. No items were moved, used, or deleted, and no day was advanced. This was a backed-up real campaign observation, not an isolated save environment. Save-isolation switches are not yet verified.

User-provided test convenience: holding the left mouse button skips cutscenes, including the startup cinematic. Use a supported held-button UI action when available.

## Reproduce locally

Run from the project root with Python 3.10+ and Node. Select the intended save explicitly; the tools do not modify it.

```powershell
$gamePath = 'C:\Program Files (x86)\Steam\steamapps\common\Mewgenics'
$savePath = Join-Path $env:APPDATA 'Glaiel Games\Mewgenics\<Steam ID>\saves\steamcampaign01.sav'
python tools/probe_inventory.py --game $gamePath --save $savePath --output work/inventory-probe
python tools/build_preview.py --game $gamePath --save $savePath --output outputs/inventory-preview
python -m unittest discover -s mods/improved-inventory/tests -p 'test_*.py'
node --test mods/improved-inventory/tests/model.test.mjs
python tools/build_stress_preview.py
python -m http.server 8791 --bind 127.0.0.1 --directory outputs/inventory-preview
```

Open `http://127.0.0.1:8791/`; `/stress/` is explicitly labeled synthetic. The server is loopback-only and should serve only the generated preview folder. Rebuilding refreshes the snapshot; it is never live synchronization.

The probe comparison requires the reference checkout at `work/modding-sources/catstable/expectedBinaryData.cs`. To compile the diagnostic DLL, `python tools/bootstrap_zig.py` obtains official Zig 0.15.2 into `work/toolchains/` and verifies its published SHA-256. Then run:

```powershell
python tools/build_native_probe.py --game $gamePath
python tools/test_native_guard.py $gamePath
```

Output: `work/native-build/ImprovedInventoryProbe.dll`. These commands do not deploy it, edit `chainloader.ini`, stop the game, or launch it. Do not package the diagnostic DLL as the inventory mod.

With the game closed, a deliberate passive session can be run using `python tools/run_inventory_probe.py --game $gamePath --profile '<profile folder containing saves and settings.txt>'`. Build the generated assets with `python tools/build_filter_assets.py` before using `--layout-test`. For transfer tests add both `--layout-test --test-slot`: the runner copies campaign 1 into absent slot 3, refuses to overwrite an existing slot, and archives/removes its copy after exit. Select the **rightmost Home**. This isolates inventory, not profile settings or Steam. Keep Vortex deployment idle during the session; exit through the game menu and let the runner restore the config. Backups and a before/after save comparison remain in `work/backups/`. Native sorting and transfers can change saved sequence metadata.

## Next implementation gate

1. Passive observation and temporary campaign-copy transfer testing succeeded. Preserve the recovered main campaign and use the copy workflow for further mutations.
2. Effective runtime names, descriptions and set memberships now drive native search and set filters. Per-copy remaining uses and a populated Broken result still need validation.
3. Result counts, global search and a searchable multi-select set menu are implemented. Directional scroll arrows, expanded filters, modal dropdowns, populated Trash, click transfers and reopening have initial runtime evidence, including a round-trip transfer under combined search/set filters. Broader transfer combinations, precise input bounds, controller support and teardown still need validation. English text entry is the initial target; IME composition and full Unicode editing remain release gates.
4. Integrate the tested browsing behavior, check compatibility with the installed mods, and package the first playable version for Vortex.

An `Inventory::insert_item` string reference identifies a candidate low-level insertion routine at RVA `2E13F0`; this is not a validated transfer operation. Batch trash movement stays a later milestone and requires additional identity, capacity, and save/reload validation.
