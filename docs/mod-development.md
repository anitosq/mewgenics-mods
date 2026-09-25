# Making Mewgenics mods in this project

Improved Inventory established a workable path from game research to a native
UI mod distributed through Vortex. These are the practices to carry forward.
Its addresses, layouts and loader versions are specific to the tested build.

## Start with a playable slice

Use the [mod brief](templates/mod-brief.md) to define the problem, first useful
result and what can wait. For Improved Inventory, readable scrolling and
filters were the first priority; bulk trash actions stayed out of 0.1.0.
Keep related controls in one mod when they share state and the same screen.
Separate unrelated features so players can choose them independently.

Choose the name before making the first installable package. Record a display
name, folder/DLL identity and tag prefix. Renaming Inventory QoL late meant
migrating source paths, exported names, diagnostic markers, assets, launch
paths and installed files. A label change shouldn't accidentally leave two
copies of the same hook loaded.

Use a prototype for a specific uncertainty. The browser prototype helped
test search and filtering, but could not prove native scrolling, tooltips,
input coordinates or transfers. Move into the game as soon as those become
the questions. Add normal startup and a Vortex ZIP to the first playable
milestone; keep diagnostic tools as a separate development path.

## Research the behavior that matters

Start with the wiki for player terminology, then check the installed build,
game data and active mods. Open-source mods can reveal useful entry points
and APIs; pin the source revision and verify it against the installed game.
Read licenses before reusing code, and record credits once in the notices.

The Legendary filter was a useful mistake: a file called
`legendary_items.gon` did not prove a Legendary rarity. The installed data
supported four ordinary rarity tiers. Likewise, a list of encountered sets
isn't a complete game catalogue. Preserve unknown metadata instead of
silently assigning it an ordinary category.

Choose the smallest implementation route:

| Need | First route to investigate |
| --- | --- |
| Existing item, event or balance values | Targeted data merge/patch |
| UI art or layout supported by existing symbols | Asset change using native symbols |
| New behavior or input unavailable in data/assets | Guarded native hooks |

Vortex deploys and launches mods; the game loads data/assets; a loader such
as Mewjector loads native code. A manager recommended on a Nexus page isn't
automatically a runtime dependency. Record only dependencies the mod uses.

## Build the native UI with native references

Use the game's font, rarity symbols and sounds before tuning dimensions.
Our late font switch changed text widths and exposed clipping in All sets.
Measure label widths, then inspect the result in-game, including long
selections, empty results and dynamic counts.

For inventory-style controls:

- Use a common font size, consistent option heights and sentence case.
  Center short button labels; right-align dropdown arrows independently of text.
- Keep hover rectangles and hit areas relative to their parent control.
  Animated panels need current coordinate transforms, not fixed screen boxes.
- An open popup must own clicks, wheel input and tooltips in its area. Preserve
  mouse-leave cleanup so blocked items don't leave tooltips stuck onscreen.
- Reapply popup layering after native drawer creation, transfers and sorting.
  A high layer at initial creation alone was insufficient.
- Validate retained native objects across scene destruction and pool reuse.
  Global input hooks outlive individual screens; readable memory alone does not
  prove an object is still alive. Test leaving the house for an adventure after
  opening inventory, and test each screen that shares a hooked drawer class.
- Test empty, exact-capacity, first-overflow and shrinking grids. For our
  36-cell view, moving from 36 to 37 items and back exposed missing Trash
  backgrounds, delayed scrolling and unclamped offsets.

Prefer a compact toolbar for frequent actions. Use popups for long lists or
multi-selection. Global search and specific filters should compose; set
names and descriptions should come from effective loaded metadata.

## Make each test session easy to repeat

Use the [session template](templates/test-session.md). Keep private paths,
saves and logs under `work/`; commit a short result with the build and scope.
The template records actual outcomes, including tests that weren't run.

Before launch, record the archive/DLL version, game build, loader, active mod
profile, asset paths and save slot. Back up the campaign and configuration.
Use a copied campaign for item transfers, day advancement or other mutations.
The existing inventory runner refuses an occupied test slot; inspect its
current behavior before reusing it. It is not a general sandbox for every mod.

Close through the game menu before deploying the next DLL. If the user is
testing, prepare edits and offline checks while they play, then replace the
files once they have quit. Hand over one exact build with a short test list.
Holding the left mouse button can skip cutscenes when supported by the UI tool.

After exit, capture the log before another launch, compare meaningful item
counts/properties, and finish session cleanup. A save hash change can come
from ordinary sorting or game bookkeeping; inspect the difference before
restoring anything. Keep intentional transfers. Never overwrite newer play
with an older backup just to make hashes match.

If automation cannot inspect the game, ask for a focused visual check and
label it as user verification. A successful loader log doesn't prove label
alignment, sound playback or tooltip behavior. If an exit crash appears,
compare with the same setup without our mod before attributing it to a hook.

Windows Defender flagged an editing command during setup. Preserve the
detection details and inspect the affected operation; don't assume a false
positive or add exclusions as a routine development step.

## Check compatibility and performance deliberately

Test the loader plus our mod in isolation, then the usual mod collection.
Record both DLL load order and asset/data order. The Used Princess Hat
tooltip problem came from full-file item replacements erasing earlier
merges. It wasn't an inventory rendering bug. Compare effective definitions
and changed leaf fields before rearranging unrelated mods.

Prefer narrow patches. Loading an old whole-file replacement earlier can
preserve later merges while still reverting unrelated current-game values.
The [compatibility audit](research/mod-compatibility-audit.md) has examples.
Don't maintain local repairs to other people's mods without a separate need.

For native code, validate executable identity and hook entries before
activation. Exercise partial hook failures, disabled assets and mismatched
DLL/SWF pairs. Release packages should use normal loader discovery without
shipping a replacement loader configuration.

Cache derived data when its inputs change, with explicit invalidation for
transfers, metadata rebinds and empty/repopulated views. Improved Inventory
stopped rebuilding set counts every frame and sampled hover coordinates
once per control/update. Its fixtures count work; they don't measure FPS.
Profile in-game before pursuing more complicated optimizations.

Global button hooks should reject unrelated controls before reading native
objects. Equipment selection exposed an expensive full-inventory scan inside
every hit test. Index presentation objects at grid rebuilds, then validate only
the matching reference. Keep lifetime checks at the point of use; performance
work must not bring back the scene-transition crash. Separate component timings
from measured frame times and user reports of responsiveness.

## Package and publish

Follow the [release workflow](releases/README.md). Start with the final archive
name/version, then test that exact package through Vortex. Cover installation,
disable/re-enable, removal, reinstall, updates and asset Load Order toggling.
A renamed-mod migration and a same-name version upgrade are different tests.

Use [the package verifier](../tools/verify_mod_archive.py) to compare a frozen
manifest with a local or downloaded ZIP. Nexus may rename a download; the
filename doesn't determine whether its contents match. See the release
workflow for commands. Verify deployed files and in-game behavior separately.

Keep the Nexus display name clean, such as Improved Inventory. Put the version
in Nexus's version fields and the archive filename. A GitHub release title
can include it so separate releases are easy to identify.

Review public copy before freezing the package. Describe what players get,
how to install and any limits that affect them. Put detailed test history in
release records, and licenses in their own sections. Screenshots should show
the features; a cover can use organized item imagery and readable native text.

Do the audience review before the humanizer pass. Our first edit made the
sentences shorter but left private test-build migration advice and ordinary
dropdown behavior in the listing. A new player had no reason to read either.
Write from the publicly available release history, not this conversation.
For each sentence, identify the player decision it helps or the benefit it
explains. Remove or relocate it if neither applies. The
[release audience check](releases/README.md#review-the-public-copy)
spells out what belongs on each page.

For browser publication, verify edits after saving. Nexus's file-description
field once showed new text without updating its form state. Its preview
panel also intercepted clicks on controls beneath it. Confirm the public
page, and distinguish upload success, scan completion and download checks.

## What to reuse next

Reuse the existing test/build patterns, guarded startup approach, packaging
manifest and these templates. Keep mod-specific hooks and addresses in each
mod. Extract a shared native library or generic packager once a second mod
shows which parts are actually common.

The highest-value follow-ups are a reusable session recorder with explicit
cleanup state, metadata-driven per-mod packaging, and the documented Nexus
mirror workflow. They remain follow-ups; this guide doesn't imply that they
are implemented. The published 0.1.0 Nexus-to-Vortex test also remains open
until its result is reported and checked.

## Evidence

- [Feasibility and source research](research/inventory-feasibility.md)
- [Native UI experiments and performance work](research/native-inventory-notes.md)
- [Set-tooltip diagnosis](research/set-tooltip-conflict.md)
- [Candidate and Vortex lifecycle tests](releases/improved-inventory/0.1.0-beta.2-candidate.md)
- [Publication and copy review](releases/improved-inventory/0.1.0-publication.md)
