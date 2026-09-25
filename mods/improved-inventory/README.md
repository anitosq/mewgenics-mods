# Improved Inventory

A Mewgenics mod for sorting through a crowded inventory. Storage and Trash
scroll without shrinking your item icons, with search and filters built into
the existing screen.

- Fixed-size, scrolling item grids with directional scroll indicators.
- Search across item names, descriptions and set names.
- Item-type and rarity filters, including worn and broken items.
- Searchable set list with multiple selections and item counts.

## Download and install

Version 0.1.0: [GitHub download](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.1.0)
and [Nexus Mods / Vortex](https://www.nexusmods.com/mewgenics/mods/526).
Requires Windows x64, Mewgenics Steam build 25143593 (1.1.21239), and
Mewjector API v3 (tested with runtime v3.0). Mewjector is the only mod
dependency. Follow the [installation guide](INSTALL.md) for Vortex or manual setup.
The mod stays inactive on unsupported game builds.

See the [release notes](releases/0.1.0.md) for features and the
[changelog](CHANGELOG.md) for changes.

## Release records

The [candidate record](../../docs/releases/improved-inventory/0.1.0-beta.2-candidate.md)
contains the Vortex and gameplay test results. The
[release checklist](RELEASE_CHECKLIST.md) tracks remaining tests, and the
[publication record](../../docs/releases/improved-inventory/0.1.0-publication.md)
records download verification. Use the [publishing workflow](../../docs/releases/README.md)
for updates to Nexus Mods and GitHub.

## Development

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
startup guard checks. It writes the ZIP, checksum and payload manifest under
`outputs/releases/improved-inventory/<version>/` and refuses to overwrite an
existing candidate directory. Use `--output outputs/candidates/<new-name>`
for another local test build. See [installation instructions](INSTALL.md).

`src/native/` is the in-game implementation. `src/browser/` is an earlier,
read-only browser prototype for saved inventory snapshots; it is not needed
by the native mod. Shared tooling remains at the repository root.

See the [feature plan](../../docs/research/improved-inventory-plan.md) and
[native integration notes](../../docs/research/native-inventory-notes.md) for
test evidence, known limitations and release work.
