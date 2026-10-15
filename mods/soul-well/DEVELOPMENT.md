# Soul Well development

Version: `0.1.0-beta.2` (local candidate). Install identity: `SoulWell`. Tag prefix: `soul-well/v`.
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
python mods/soul-well/test_release.py --game "PATH/TO/Mewgenics" --archive "outputs/releases/soul-well/0.1.0-beta.2/SoulWell-0.1.0-beta.2.zip" --manifest "outputs/releases/soul-well/0.1.0-beta.2/SoulWell-0.1.0-beta.2.manifest.json"
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
If all hooks install but data validation fails, SoulWell spawn calls are
finished without spawning; other items still pass through. Unsupported builds,
entry mismatches and partial hook installation cannot promise that blocking.
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
It compares GON tokens against the built-in source definitions. Whitespace,
comments, CRLF and UTF-8 BOM do not affect matching in any of the three files.
Two ability positions are editable: the positive signed-32-bit integer value
of target.max_range, and the balanced additional_passives body. Duplicate
fields, extra definitions, changed core values, embedded NUL, unterminated
strings/comments and mismatched delimiters are rejected. Passive nesting is
bounded at 64 delimiters, file size below 64 KiB. The game's parser still owns
passive key/value semantics. Source generation embeds the complete baseline,
so nested passive definitions no longer confuse a first-closing-brace search.
Later mods can still override effective definitions; this is not a runtime
load-order validator. Other settings/order changes require a matching build.

Failures log a specific reason and queue a single Windows MessageBox on a new
thread. The DLL holds a module reference until the notice finishes. DllMain
does not call User32 or wait for the thread; see Microsoft's
[DLL guidance](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).
If thread creation fails, the error stays in the log and one extra module
reference survives until process exit. A DLL that never loads cannot notify.
Non-Mewgenics host processes skip initialization, including package fixtures.
The notification text is tested with a substituted UI callback; the actual
dialog and fullscreen focus still need an owner check.

Each cast logs scan totals before spawning and each jar ID requested by native
cloning. This does not count successfully placed shades. No names or save
contents are logged.

## Evidence and pending gameplay

The 2026-10-14 disposable prototype entered battle and summoned three unique
cats from storage, backpack and equipment. Two duplicate IDs and two invalid
entries were skipped. Repeated ordinary use was blocked. After normal save,
quit and reload, it summoned the same three cats again. The game resets the
fight on reload. Original saves/settings were restored with save-hash checks.

The recipient reported beta.1's random-looking, move-only shades after changing
max_range to 10, then reported normal behavior after restoring 1. The exact
published DLL reproduced that gate failure offline (1 accepted, 10 rejected).
Beta.2 fixes that check and adds notices, failure blocking and clone-request
logs. Tests model the reported 380 items/20 jars/five unique IDs. These changes
have offline coverage; beta.2 has not been played. Leave the installation and
saves unchanged under the owner-testing workflow.

Owner test: install this exact ZIP; refresh Tracy's stock and buy Soul Well;
equip it and summon with jars at home/backpack/equipped; save/reload and repeat;
edit a known passive and confirm new shades receive it. Check a crowded board,
no jars and Armor Specialist extra uses. Record logs separately from observed
behavior. For beta.2, test range 10 plus customized passives; deliberately
change a core value, confirm the startup dialog and that no generic shade
spawns, then restore it and restart. Full-board outcomes, large collections
and removal remain unverified.
