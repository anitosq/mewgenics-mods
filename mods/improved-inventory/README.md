# Improved Inventory

A Mewgenics mod for sorting through a crowded inventory. Storage and Trash
scroll without shrinking your item icons. Search and filters work at home
and while choosing your cats' equipment before an adventure.

- Fixed-size, scrolling item grids with directional scroll indicators.
- Search across item names, descriptions and set names.
- Item-type and rarity filters, including worn and broken items.
- Searchable set list with multiple selections and separate Storage/Trash counts.

## Download and install

Version 0.3.7: [GitHub download](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.3.7)
and [Nexus Mods / Vortex](https://www.nexusmods.com/mewgenics/mods/526).
Requires Windows x64, Mewgenics Steam build 25143593 (1.1.21239), and
Mewjector API v3 (tested with the v3.4 GitHub release). Mewjector is the only mod
dependency. The same ZIP supports Vortex and Mewtator. Follow the
[installation guide](INSTALL.md), including the update step for older Vortex installs.
The mod stays inactive on unsupported game builds.

See the [release notes](releases/0.3.7.md) for this update and the
[changelog](CHANGELOG.md) for earlier versions.

## Release records

The [candidate record](../../docs/releases/improved-inventory/0.3.7-beta.4-candidate.md)
contains the package test results and runtime test scope. The
[release checklist](RELEASE_CHECKLIST.md) tracks remaining tests, and the
[publication record](../../docs/releases/improved-inventory/0.3.7-publication.md)
records download verification. Use the [publishing workflow](../../docs/releases/README.md)
for updates to Nexus Mods and GitHub.

## Development

The [0.4.0-beta.6 candidate](../../docs/releases/improved-inventory/0.4.0-beta.6-candidate.md)
fixes the beta.5 regression that kept the mod from appearing. The
[0.4.0-beta.4 candidate](../../docs/releases/improved-inventory/0.4.0-beta.4-candidate.md)
adds the adventure Backpack, capacity-limited Storage sizes and fixed Trash
grids. The [0.4.0-beta.3 candidate](../../docs/releases/improved-inventory/0.4.0-beta.3-candidate.md)
added read-only Backpack-screen probes, which betas still include. The
[0.4.0-beta.2 candidate](../../docs/releases/improved-inventory/0.4.0-beta.2-candidate.md)
adds per-container grid sizes and localized set names on top of the untested
0.3.8-beta.1 performance work. It awaits owner testing; 0.3.7 remains the
public release.

The Storage and Trash buttons beside the item counts each cycle through 5x5,
6x6 (default), 7x7, 8x8 and 10x10; Trash also offers Auto. Storage's game grid
matches its capacity, so its button offers the sizes below the capacity and
the capacity itself. A smaller size scrolls. A fixed Trash size is a maximum:
the game's Trash grid grows with its items (from 4x4) up to it, then scrolls.
Trash Auto never caps, as in the unmodded game. The choices are saved in
`%LOCALAPPDATA%\ImprovedInventory\settings.ini`, outside the mod folders.

Set counts include duplicate copies and use all items in each container,
independent of search and filters. Equipment selection counts unassigned items
under "Available".


Equipment selection uses the same column cap with five rows for every six
columns (6x5 by default), with scrolling for the rest. The adventure Backpack
is the same native screen in another mode. It gets the same toolbar and grid
and follows the Trash size, since adventures have no capacity limit.

Run commands from the repository root. Python 3.10+, Node.js and Windows x64
are used for development. The pinned native compiler is Zig 0.15.2; download
it into the ignored `work/` directory with:

```powershell
python tools/bootstrap_zig.py
```

Run the fixture checks without opening the game or accessing a save:

```powershell
python tools/test_improved_inventory.py
```

This runs the Python reader tests, browser model tests and native filter,
search and set-menu cache tests. Native assertions are explicitly enabled.

The native build additionally requires a locally installed supported game and
the local compatibility report. Follow the
[research setup and build instructions](../../docs/research/inventory-feasibility.md#reproduce-locally).
Generated binaries stay in `work/native-build/`.

With a clean committed working tree and the local compatibility report in
place, build a candidate without deploying it:

```powershell
python tools/package_improved_inventory.py --game 'C:\Program Files (x86)\Steam\steamapps\common\Mewgenics'
```

The packager runs fixtures, native font/art checks, release compilation and
startup guard checks. It builds the DLL/assets once and writes one ZIP
with a checksum and payload manifest under
`outputs/releases/improved-inventory/<version>/` and refuses to overwrite an
existing candidate directory. Use `--output outputs/candidates/<new-name>`
for another local test build. See [installation instructions](INSTALL.md).

`src/native/` is the in-game implementation. `src/browser/` is an earlier,
read-only browser prototype for saved inventory snapshots; it is not needed
by the native mod. Shared tooling remains at the repository root.

See the [feature plan](../../docs/research/improved-inventory-plan.md) and
[native integration notes](../../docs/research/native-inventory-notes.md) for
test evidence, known limitations and release work.
