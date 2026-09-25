# Mewgenics Mods

Mewgenics mods by anitosq. Each mod has its own source, tests and documentation.

| Mod | Purpose | Status |
| --- | --- | --- |
| [Improved Inventory](mods/improved-inventory/) | Scrolling grids, search and filters for Storage, Trash and adventure equipment selection | [0.2.0 on GitHub](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.2.0) / [Nexus Mods](https://www.nexusmods.com/mewgenics/mods/526) |

## Repository layout

- `mods/<mod-name>/`: each mod's source, tests and documentation.
- `tools/`: build, inspection and development utilities, currently supporting Improved Inventory.
- `docs/research/`: gameplay research, implementation findings and dated test evidence.
- `assets/`: licensed prototype reference assets; see [third-party notices](THIRD_PARTY_NOTICES.md).
- `work/` and `outputs/`: ignored local dependencies, generated files, private snapshots and backups.

Start with the [project mod development guide](docs/mod-development.md), or
the [Improved Inventory development guide](mods/improved-inventory/README.md)
for its build commands. New mods can use the [mod brief](docs/templates/mod-brief.md)
and [test session template](docs/templates/test-session.md).
Publishing to Nexus Mods/Vortex and GitHub follows the shared
[release workflow](docs/releases/README.md).
The [gameplay brief](docs/research/mewgenics-research.md) and
[modding research](docs/research/mewgenics-modding-research.md) provide background.
Research notes record the game builds and mod combinations tested at the time.

## Distribution and licensing

Download the mod ZIP attached to its release page. GitHub's automatic source
archive is for development. The [0.2.0 publication record](docs/releases/improved-inventory/0.2.0-publication.md)
has the release checksums and download checks.

Code and documentation: [MIT License](LICENSE), Copyright (c) 2026 anitosq.
See [third-party notices](THIRD_PARTY_NOTICES.md) for other credits and licenses.
