# <Mod display name>

## Player problem

<Describe the situation and what should become easier.>

## First playable version

<One small in-game outcome, where it appears, and how we'll know it works.>

Later: <features explicitly outside this first version.>

## Identity and implementation

- Display name (no version):
- Mod ID / folder / DLL name, if needed:
- Tag prefix:
- Supported game build and evidence:
- Implementation route (data, assets, native) and why:
- Required loader/API and tested runtime:
- Game symbols, fonts and sounds to reuse:
- Changed files or hook sites; likely overlaps:
- Reference repositories/revisions and applicable licenses:

## Questions to resolve

| Question | Evidence needed | Finding |
| --- | --- | --- |
| <Game mechanic or technical uncertainty> | <Data/runtime/source check> | <Known, inferred or unresolved> |

## Test and package plan

- First Vortex-installable milestone:
- Relevant empty/boundary/overflow cases:
- Save/profile isolation and cleanup:
- Isolated dependencies and normal-collection checks:
- Lifecycle and version-upgrade checks:
- Remaining input/platform limits:

Use [the session record](test-session.md) for individual runs and
[the release workflow](../releases/README.md) for publication.
