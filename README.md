# Mewgenics Mods

Mewgenics mod development by **anitosq**. Each mod has its own source, tests and documentation in this repository.

| Mod | Purpose | Status |
| --- | --- | --- |
| [Improved Inventory](mods/improved-inventory/) | Readable, scrolling Storage/Trash grids with search, item type, rarity and set filters | [0.1.0 on GitHub](https://github.com/anitosq/mewgenics-mods/releases/tag/improved-inventory/v0.1.0) / [Nexus Mods](https://www.nexusmods.com/mewgenics/mods/526) |

## Repository layout

- `mods/<mod-name>/`: each mod's source, tests and documentation.
- `tools/`: build, inspection and development utilities, currently supporting Improved Inventory.
- `docs/research/`: gameplay research, implementation findings and dated test evidence.
- `assets/`: licensed prototype reference assets; see [third-party notices](THIRD_PARTY_NOTICES.md).
- `work/` and `outputs/`: ignored local dependencies, generated files, private snapshots and backups.

Start with the [Improved Inventory development guide](mods/improved-inventory/README.md).
Publishing to Nexus Mods/Vortex and GitHub follows the shared
[release workflow](docs/releases/README.md).
The [gameplay brief](docs/research/mewgenics-research.md) and
[modding research](docs/research/mewgenics-modding-research.md) provide background.
Research notes describe the builds and mod combinations tested at the time;
they are not installation instructions or guarantees of current compatibility.

## Distribution and licensing

Download the installable ZIP from a mod's release page, not GitHub's automatic
source archive. See the [0.1.0 publication record](docs/releases/improved-inventory/0.1.0-publication.md)
for download verification and Nexus processing status. Game files, saves,
extracted game assets, third-party mod binaries and compiler downloads
are excluded. Current native UI assets reference the game's own fonts and
artwork at runtime rather than embedding them.

Original project code and documentation are licensed under the
[MIT License](LICENSE), Copyright (c) 2026 anitosq. This applies across the
repository's mods unless otherwise noted. Third-party material retains its
own terms in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md); game assets are
not covered by the project license.
