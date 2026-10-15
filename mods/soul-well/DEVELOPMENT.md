# Soul Well development

Version: `0.1.0-beta.1`. Install identity: `SoulWell`. Tag prefix: `soul-well/v`.
Windows x64, Mewgenics 1.1.21239 / Steam build 25143593, Mewjector API >= 3.
Executable SHA-256:
`4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`.

## Build and package

From the repository root, using Python 3.10 and Zig 0.15.2:

```powershell
python mods/soul-well/build.py --exe "PATH/TO/Mewgenics.exe" --zig "PATH/TO/zig.exe"
python mods/soul-well/test_release.py --game "PATH/TO/Mewgenics"
```

These commands read the game but do not launch it, install mods or touch saves.
The builder runs native assertions with assertions enabled in the optimized build.
Commit source, rebuild from that clean commit, then freeze the archive:

```powershell
python mods/soul-well/package.py
python mods/soul-well/test_release.py --game "PATH/TO/Mewgenics" --archive "outputs/releases/soul-well/0.1.0-beta.1/SoulWell-0.1.0-beta.1.zip" --manifest "outputs/releases/soul-well/0.1.0-beta.1/SoulWell-0.1.0-beta.1.manifest.json"
```

Use `tools/check_mewtator_package.py --mod-name SoulWell` with the frozen ZIP,
manifest and the upstream Mewtator checkout for its discovery/manifest fixture.
The Soul Well package test models the FOMOD required-file mapping in a temporary
directory. It does not establish actual manager installation or gameplay.
Do not overwrite a frozen archive or retag a published release.

## Native scope

The spawn callback at RVA `0x7a2ba0` handles only sources named `SoulWell`.
One scoped CastPoint hook (`0x43350`) repeats the chosen target coordinate.
The SourceItem hook (`0x475c0`) returns a temporary ItemInfo copy with the next
unique cat ID only at the native shade-cloning call site, return RVA `0x7a3c51`.
The game's spawn callback executes once; it owns cost, animation, placement,
cloning and configured additional passives. Other items pass through.

Inventory maps at Game inventory manager +0x38 and +0x78 supply backpack and
home storage. On-board allied campaign cats supply equipped jars. Negative
IDs are excluded; database lookup at `0xd7220` rejects absent cats. Capture
never writes source ItemInfo records. No campaign marker or save edit is required.

The SourceItem trampoline steals 31 bytes. Its outer short jump lands on the
jump-back boundary and its inner branch stays inside the copied span. The
native regression executes root and nested-parent paths. The other hook spans
are unchanged from the successful prototype. Hash and entry-byte checks guard
all native functions used by the mod.

Partial hook installation leaves active scope null and hooks_ready false;
installed hooks pass through. Tests exercise each install failure position.
Thread-attach notifications skip hashing to protect graphics threads with small
stacks. File hashing stays out of DllMain's stack frame.

## Data and startup contract

`trinkets.gon.merge` adds a new item with aux -1; the ordinary-campaign capture
test uses no marked item. `item_abilities.gon.merge` adds a separate spawn ability.
`tracy_house_shop.gon.patch` replaces only the consumable offer's pool, price and
duplicate flag in normal/special shops; the data fixture checks all eight stock
tiers and preservation of all other fields against the installed game archive.

The startup gate requires an explicit enabled `-modpaths` directory matching
either the DLL parent (Mewtator) or its SoulWell sibling (Vortex/manual).
It checks item/shop hashes and exact ability prefix/suffix around the editable
passive body, accepting CRLF and UTF-8 BOM for that ability file. Strings and
comments cannot escape the editable block. The body is left to the game's GON
parser. It is not hashed, so passive edits need no DLL rebuild. Changes to core
files require a new matching build. Later mods can still override effective
definitions; the file gate is not a proof of final runtime load order.

## Evidence and pending gameplay

The 2026-10-14 disposable prototype entered battle and summoned three unique
cats from storage, backpack and equipment. Two duplicate IDs and two invalid
entries were skipped. Repeated ordinary use was blocked. After normal save,
quit and reload, it summoned the same three cats again. The game resets the
fight on reload. Original saves/settings were restored with save-hash checks.

Those results apply to the prototype's summon flow. This release removes the
fixture restriction and adds startup checks, packaging and Tracy acquisition.
These changes have offline coverage; the packaged beta has not been played.
Under the owner workflow, leave the installation and saves unchanged.

Owner test: install this exact ZIP; refresh Tracy's stock and buy Soul Well;
equip it and summon with jars at home/backpack/equipped; save/reload and repeat;
edit a known passive and confirm new shades receive it. Check a crowded board,
no jars and Armor Specialist extra uses. Record logs separately from observed
behavior. Full-board outcomes, large collections and removal remain unverified.
