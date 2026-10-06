# Development

Version 0.1.0 is a release candidate, not a published or gameplay-verified
release. The approved selector presentation is carried over from local
testing; the replacement breeding implementation and asset startup gate
need fresh owner testing.

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
- Verify manager install and uninstall, DLL-only startup, disabled UI assets,
  mismatched UI assets, and duplicate enabled copies.

Keep the Nexus page unpublished and do not tag/freeze the archive until
the candidate's affected checks are confirmed.
