# Development

Version 0.1.0 is published following owner approval. The exact artifact and
verification scope are recorded in docs/releases/good-genes/0.1.0-publication.md.
Detailed cases below remain useful for future changes; approval does not imply
exhaustive gameplay or manager lifecycle coverage.

Version 0.1.1 prepares the paired-comparison and pager-spacing update for owner
testing and later publication. It does not replace the public 0.1.0 archive.

## Build

From the repository root:

```powershell
python mods/good-genes/build.py --exe "C:/Program Files (x86)/Steam/steamapps/common/Mewgenics/Mewgenics.exe" --zig work/toolchains/zig-x86_64-windows-0.15.2/zig.exe
python mods/good-genes/package.py
```

The builder only reads game files and writes local build output. Packaging
requires committed source and does not install or load the DLL.

The current identity is GoodGenes (DLL and asset folder), with good_genes.swf
and GoodGenesMutationSelector. When replacing a pre-rename local prototype,
close the game and remove its DLL and enabled asset entry first. Renaming the
source does not migrate an installed package or its manager configuration.

## Independent Breeding Implementation

The supported executable SHA-256 is
`4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`.
Static inspection of that executable establishes:

- 0xa7a00 writes a selected parental base stat. Its seven callers belong to
  the birth constructor. The replacement writes the greater parental value.
- 0xa7820 writes a parental part ID at output+4. It receives part type at +0,
  ID at +4, and the active flag at +0x18. Definition type at +0xa8 distinguishes
  mutated from ordinary parts. The replacement prefers the sole mutated
  parent, and uses the native unbiased roll for equal occupancy.
- The first part call returns at 0xa993a. Coat selection is corrected there
  using the same policy, before the constructor copies coat IDs to each part.
  The constructor's later paired-part mirroring is retained.
- New-disorder construction is skipped from 0xa9580 to 0xa9636. Parent
  disorder inheritance has already completed.
- Part-inheritance loss is skipped from 0xa9728 to 0xa97c1. The existing
  sentinel -1 still permits all normal part-inheritance calls.
- New birth-defect generation is skipped from 0xa9e42 to 0xa9fad, preserving
  the constructor's final sprite refresh and cleanup.

These are two entry detours and three generated branch gates. No imported
patch table is present. Full hashes of the three relevant native routines
reject pre-existing modifications before any changes are applied.
New part/stat trampolines copy 23/17 complete bytes respectively, with no
relative branches or RIP-relative operands in those spans.

Random-number consumption differs from the previous local prototype.
Do not compare exact kitten outcomes across builds as a reproducibility test.

## Deferred Checks

### Local paired-comparison revision (2026-10-07)

The owner's duplicated eye comparison was a presentation problem: accepting
an offer already invokes the original SetPiece once with its original part/ID.
The supported executable's SetPiece at 0xcd080 writes both eye IDs for part 6
(0x2dc and 0x330); parts 15/16 write only one. Arms, legs, eyebrows and ears
have equivalent paired and single-side branches; part 5 writes all four limbs.
RandomMutation at 0xcc3f0 selects the paired or asymmetric pool from its
symmetry argument. Neither mutation scope nor roll/application policy changes.

This agrees with the wiki's [Symmetry section](https://mewgenics.wiki.gg/wiki/Mutations#Symmetry):
matching paired mutations count once, but some event/combat sources can mutate
asymmetrically. Direct fetch returned 403; the search-indexed page supplied
the section on 2026-10-07. The executable remains the basis for write scope.

The selector now groups affected counterparts only when current IDs match.
Different current IDs retain separate comparisons, including unmutated parts.
Matching pairs use Eyes/Arms/etc. with Keep Existing/Replace; single-side titles
remain single-sided. Different comparisons within a paired roll use Replace
Both (Replace All for all limbs), with Keep Existing unchanged. Matching pairs
and single-side changes have no comparison pager; long effects can still have
text pages. Effects are not doubled for the grouped preview. Pager-only
clearance is 16px above and below its 28px row; absent navigation retains the
existing 20px effects-to-button gap. This revision is local, not published.

### Checks to run with owner authorization

Owner instruction: do not execute tests, load the DLL, install it, launch the
game, or alter saves without explicit permission. Compilation and static
inspection are permitted.

- `test_policy.c`: mutation comparisons and parental occupancy policy.
- `test_hook_layout.py`: copied instructions, birth branch destinations,
  selector lifetime safeguards, and layout contracts.
- On a backed-up test slot, breed one-mutated and two-mutated parents;
  include coat mutations, paired parts, inherited defects/disorders and
  differing base stats. Check seven inherited base stats independently.
- Repeat Unstable DNA, special-to-stat and stat-to-special choices, battle
  end, room change, and exit with queued offers.
- For paired mutations, combine a special on one side with a stat downgrade
  on the other: offer Keep/Replace for the whole roll in either part order.
  Unknown definitions must still block it; stat-only downgrades stay automatic Keep.
- Navigate descriptions across pages and parts in both directions, including
  wraparound. Page counts reset per offer; each click renders only once.
- Matching eyes/arms/legs/eyebrows/ears show one grouped comparison with no pager
  unless effects need another text page. Different current IDs keep both
  comparisons and a shared Replace Both choice; a true single-side roll does
  not acquire a Both label. Matching pairs also use the plain group name and
  Keep Existing/Replace. Also check one side already matching the incoming
  ID and an all-limbs roll. Grouped effects must not be doubled.
- Inspect 16px pager clearance above/below, no hidden-row gap, stable footer
  position on mixed-length pages, and navigation hover/click sounds.
- Verify manager install and uninstall, DLL-only startup, disabled UI assets,
  mismatched UI assets, and duplicate enabled copies.

Preserve the published 0.1.0 archive and tag. Future payload changes require a
new version and owner confirmation of the affected checks before publication.
