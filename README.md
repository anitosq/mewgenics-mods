# Mewgenics Mods

Mewgenics mod development by **anitosq**. Each mod has its own source, tests and documentation in this repository.

| Mod | Purpose | Status |
| --- | --- | --- |
| [Inventory QoL](mods/inventory-qol/) | Readable, scrolling Storage/Trash grids with search, item type, rarity and set filters | Experimental; not yet packaged for installation |

## Repository layout

- `mods/<mod-name>/`: each mod's source, tests and documentation.
- `tools/`: build, inspection and development utilities, currently supporting Inventory QoL.
- `docs/research/`: gameplay research, implementation findings and dated test evidence.
- `assets/`: licensed prototype reference assets; see [third-party notices](THIRD_PARTY_NOTICES.md).
- `work/` and `outputs/`: ignored local dependencies, generated files, private snapshots and backups.

Start with the [Inventory QoL development guide](mods/inventory-qol/README.md).
The [gameplay brief](docs/research/mewgenics-research.md) and
[modding research](docs/research/mewgenics-modding-research.md) provide background.
Research notes describe the builds and mod combinations tested at the time;
they are not installation instructions or guarantees of current compatibility.

## Distribution and licensing

No installable release has been published from this repository. Game files,
saves, extracted game assets, third-party mod binaries and compiler downloads
are excluded. Current native UI assets reference the game's own fonts and
artwork at runtime rather than embedding them.

A project license has not yet been selected. Third-party material retains its
own terms in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
