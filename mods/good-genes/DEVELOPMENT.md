# Development

Version 0.1.0 is published following owner approval. The exact artifact and
verification scope are recorded in docs/releases/good-genes/0.1.0-publication.md.
Detailed cases below remain useful for future changes; approval does not imply
exhaustive gameplay or manager lifecycle coverage.

Version 0.1.1 is also published; see docs/releases/good-genes/0.1.1-publication.md.
The owner approved promotion of 0.2.0.beta.2 to official 0.2.0 on 2026-10-09.
Only version labels and release documentation change for promotion; gameplay
code remains the beta.2 implementation. Publication approval is not gameplay
test confirmation. The game installation and saves stay unchanged. Preserve
all previous archives; see the 0.2.0 publication record for release status.

## Audit Fixes (2026-10-09)

- Head projection instantiates a detached `CatHeadPlacements` clip and uses
  the same child names as native 0x7393e0. Eyebrows follow eye presence. The
  native virtual destructor releases that clip; preview never calls SetPiece.
  Policy, comparison pages and totals include hidden/restored facial effects.
- Native 0xcaa70 emits ID -2 for both absent eyes, eyebrows or ears, then
  deduplicates each pair. A single absent side and an inactive mouth emit 0.
  Explicit active ID -2 remains a real mutation resolved through definitions.
- All subsequent candidates queue while a choice is pending, including rolls
  that initially appear automatic or rejected. Each is re-evaluated after the
  previous choice; original candidates and atomic native write scope remain.
- One bounded appearance read replaces per-part reads in the shared collector.
  Head-placement work occurs for head rolls only, never on the idle-frame path.
- Packaging validates build version, commit, source and shared generator
  hashes, and all three compiled outputs before creating metadata. A new build
  invalidates the previous build manifest first. Returned verified bytes are
  the bytes packaged, not a second disk read.

Deferred checks: head 700 -> 309 with a right-eye DEX mutation must offer a
trade-off; a special on a disappearing facial part must be shown. Check head
319 with no ears, 320 with no left eye, and restoration to a normal head.
Missing ears/eyebrows should contribute -2 DEX/-2 CHA once per pair. Use a
multi-roll combat effect that opens a choice followed by an automatic roll,
and test both Keep and Replace before battle exit. All regression execution
and gameplay remain with the owner; compilation and archive checks only.

## Local Stat Trade-Off Revision (2026-10-08)

Occupied stat-only rolls use effective before/after mutation totals: any gain
with no losses applies automatically, any gain with a loss opens the selector,
and equal totals or pure downgrades keep the current mutation. Special choices,
empty parts, birth inheritance, overnight scope, and atomic SetPiece writes
are preserved, with the head/facial exception documented above.

Read-only executable inspection: birth constructor 0xa89a0 passes +0x6f0
through +0x708 to the seven parental-stat calls. Combat initialization at
0xfcc32 adds these inherited stats to the separate bonus result from 0xc0b80.
Mutation collection at 0xcaa70 resolves inactive shape flags (ID offset+0x14)
as 0 or missing-pair -2, always includes the overall coat at +0x78, and
deduplicates canonical group/ID pairs before definition resolution at 0xcc300.
SetPiece changes facial flags for head rolls through 0x7393e0; other parts
retain their active flags.
The preview reads these values without calling SetPiece, adds only mutation
bonuses, and excludes class, levels, equipment, and temporary combat effects.
Coat's embedded per-shape copies must not be counted separately. Unknown
definitions make the totals unavailable. Acceptance rechecks the entire preview
as well as the affected IDs, flags, and scene lifetime.

UI: up to twelve 22px rows of 18px effects replace four text-paged rows.
Descriptions must fit in full or the offer is kept. Arrows navigate different
parts only. The seven-stat strip has native black icons and colored incoming
values; its numbers cover the whole roll even when navigating parts. Maximum
panel height is 658px on the native 720px canvas, centered vertically. The
footer remains stable across comparisons, with no hidden-pager space.

Deferred owner checks: reproduce three separate -1 CHA mutations on base 7,
then replace +2 STR/-1 CHA with +1 STR. Expect CHA 4 -> 5 green, STR decreasing
red, and other values unchanged. Check Keep/Replace, matching/split pairs,
inactive counterparts, coat changes, missing parts, seven-stat icon order,
mixed stat/special effects, long localized text, queued offers, and battle exit.
Regression checks were updated but not executed; compilation only is allowed.

## Build

From the repository root:

```powershell
python mods/good-genes/build.py --exe "C:/Program Files (x86)/Steam/steamapps/common/Mewgenics/Mewgenics.exe" --zig work/toolchains/zig-x86_64-windows-0.15.2/zig.exe
python mods/good-genes/package.py
```

The builder only reads game files and writes local build output. Packaging
requires committed source and a matching successful build manifest, and does
not install or load the DLL. Commit before building a release candidate.

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

### Published paired-comparison revision (2026-10-07)

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
existing 20px effects-to-button gap. The local 2026-10-08 revision above
supersedes text paging and adds the stat strip.

### Checks to run with owner authorization

The second 0.1.1 candidate fixes two owner review findings. SetPiece's per-side
comparison now has an additional combined-stat guard for automatic occupied-slot
replacements. It deduplicates by canonical part group and ID, including an
unchanged counterpart, matching the unique-list step at 0xcb5e6-0xcb685 in the
supported executable. The current game archive defines eyes.750 as +1 INT and
eyes.303 as +1 INT/+1 CHA; collapsing those into matching eyes.303 must be kept.
Special choices and entirely unmutated selected slots retain their policies.

The sign check on incoming IDs is removed. The definition lookup at 0x7bdac0
converts the ID to a signed string key; the current eyes, ears, eyebrows and
mouth tables have valid -2 definitions. Undefined negative keys remain UNKNOWN,
including when the current part is unmutated. Known -2 previews show Missing
part rather than a generic unavailable-art message. Neither fix changes hooks.

Owner instruction: do not execute tests, load the DLL, install it, launch the
game, or alter saves without explicit permission. Compilation and static
inspection are permitted.

- `test_policy.c`: mutation comparisons and parental occupancy policy.
- `test_package.py`: stale/missing source, version, commit and output rejection.
  Includes deduplicated before/after totals, unchanged counterparts, side order,
  distinct arms/legs groups, and unknown/negative-ID policy cases.
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
- Navigate different parts in both directions, including wraparound; each
  click renders only once. Descriptions no longer have text pages.
- Matching eyes/arms/legs/eyebrows/ears show one grouped comparison with no pager
  regardless of description length. Different current IDs keep both
  comparisons and a shared Replace Both choice; a true single-side roll does
  not acquire a Both label. Matching pairs also use the plain group name and
  Keep Existing/Replace. Also check one side already matching the incoming
  ID and an all-limbs roll. Grouped effects must not be doubled.
- Inspect 16px pager clearance above/below, no hidden-row gap, stable footer
  position on mixed-length pages, and navigation hover/click sounds.
- Try eyes.750 plus eyes.303 becoming a matching pair: automatic replacement
  must be rejected in either direction. Check a true combined improvement,
  paired writes, a matching pair splitting into different IDs, and an empty slot.
- Check defined -2 event outcomes against ordinary parts and special mutations;
  verify special comparisons, Missing part artwork labels, and unknown-ID safety.
- Verify manager install and uninstall, DLL-only startup, disabled UI assets,
  mismatched UI assets, and duplicate enabled copies.

Preserve the published 0.1.0 and 0.1.1 archives and tags. Future payload changes require a
new version and owner confirmation of the affected checks before publication.
