# Inventory QoL

An experimental Mewgenics mod that improves the existing Storage/Trash screen.

- Fixed-size, scrolling item grids with directional scroll indicators.
- Search across item names, descriptions and set names.
- Item-type and rarity filters, including worn and broken items.
- Searchable set list with multiple selections and item counts.
- Game font, runtime rarity artwork and native sound feedback.
- Dropdowns that suppress interaction with items underneath them.

## Status and dependencies

This is development source, not an installable release. The current DLL uses
an exact game-binary compatibility guard and a temporary diagnostic session
marker. Normal startup, packaging and a clean installation test remain before
release. Bulk trash actions are planned separately and are not implemented.

The [release readiness checklist](RELEASE_CHECKLIST.md) tracks the remaining
work. See the shared [publishing workflow](../../docs/releases/README.md) for
keeping Nexus Mods and GitHub releases synchronized, and the
[changelog](CHANGELOG.md) for unreleased changes.

The native code requires **Mewjector v3 API** (tested with v3.0). Other gameplay
mods, Mewtator and Mewgenics Mod Manager are not required by the implementation.
Vortex is an optional deployment tool. Standalone installation with only
Mewjector and this mod still needs verification.

## Development

Run commands from the repository root. Python 3.10+, Node.js and Windows x64
are used for development. The pinned native compiler is Zig 0.15.2; download
it into the ignored `work/` directory with:

```powershell
python tools/bootstrap_zig.py
```

Run the fixture checks without opening the game or accessing a save:

```powershell
python tools/test_inventory_qol.py
```

This runs the Python reader tests, browser model tests and native filter,
search and set-menu cache tests. Native assertions are explicitly enabled.

The native build additionally requires a locally installed supported game and
the local compatibility report. Follow the
[research setup and build instructions](../../docs/research/inventory-feasibility.md#reproduce-locally).
Generated binaries stay in `work/native-build/`.

`src/native/` is the in-game implementation. `src/browser/` is an earlier,
read-only browser prototype for saved inventory snapshots; it is not needed
by the native mod. Shared tooling remains at the repository root.

See the [feature plan](../../docs/research/inventory-qol-plan.md) and
[native integration notes](../../docs/research/native-inventory-notes.md) for
test evidence, known limitations and release work.
