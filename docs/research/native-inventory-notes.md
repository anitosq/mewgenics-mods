# Native inventory integration notes

24 September 2026. Research for the installed executable only: SHA-256
`4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`.
These are reverse-engineered implementation facts, not a supported game API.

## Presentation path

The existing Storage/Trash screen can display enlarged item icons while retaining
native tooltips and the installed set-marker art. The experimental implementation
is in `mods/improved-inventory/src/native/inventory_layout.h`, enabled only by the runner's fresh
`layout-test` marker. It does not change item IDs or container contents.

| Function RVA | Observed purpose |
| --- | --- |
| `211700` | Shared storage/trash grid layout |
| `211460` | Rebuild item-to-container lookup |
| `210590` | Rebuild the native item drawers, then lay out |
| `213A20` | Instantly position one item drawer from its panel's `ref_grid` bounds |
| `213AC0` | Update item drawer position/scale and native hover behavior |
| `213DC0` | Native single-item transfer callback; unfiltered transfers exercised on the temporary campaign copy |
| `2137E0` | Bind item identity and sort metadata to one drawer |
| `C36110` | Event dispatcher used to observe wheel input |
| `0ED390` | Find the house drawer manager from the panel's entity |

Grid owner offsets: `38`/`40` are Storage/Trash panels; `48` is mode;
`50`/`54` are native column counts; `64`/`68` are shared item-drawer count/array;
`74`/`78` are Storage background count/array; `84`/`88` are Trash background
count/array. Counts observed with this campaign: **92 drawers, 100 Storage
backgrounds, 16 Trash backgrounds**. The saved inventory has 94 serialized
records; the reason two records are not represented as native drawers remains
unresolved. Do not manufacture drawers from the saved count.

One item drawer: `38` grid owner; `48` target panel; `50` renderer;
`58` button; `60` transform; `68` 64-bit instance ID; `70` native sort ordinal;
`78`/`80` target x/y within the grid; `88` target scale. Presentation changes
operate on existing drawers and retain their identities.

Renderer `40` points to the transform and `51` controls visibility. Transform
`80`/`88` are x/y and `98`/`A0` are scale x/y. Native allocation generation is
stored eight bytes before the object. Restore retained background references
only when their generation and renderer-to-transform relationship still match.

Panel `70`/`78` delimit a vector of 40-byte animation tracking records:
transform pointer, allocation generation, x offset, y offset, flag. The panel
reapplies these relative positions each frame. Updating just transform positions
does not persist. Background layout must also update the tracking offsets.
Trash backgrounds are created bottom-up; Storage backgrounds are top-down.
Never infer a shared origin from the first background. The revised experiment
reads both panels' `ref_grid` bounds using `97CB30`, preserves each background's
scale signs, and normalizes their placement after every native rebuild.

## Input findings

The observed wheel event has type `1027`, floating-point vertical delta at
`1C`, and direction at `20`. An injected downward three-notch wheel event
contained `-3.0f` at `1C`. The earlier assumed integer delta at `14` was zero.
The current experiment clamps scrolling to complete rows with independent
Storage/Trash offsets and checks the active house panel before scrolling.

Disabling a hovered item's native button update leaves its tooltip stuck even
after closing the inventory. Keep native button updates running, place hidden
items offscreen, and reject hidden-item transfer callbacks instead. Tooltip
cleanup and empty-cell hover must be tested after every change to this path.

The current mouse region uses approximate client-window fractions for the
observed desktop layout. This is a test limitation: release code needs native
grid bounds transformed into input coordinates, a visible scroll indicator,
fractional wheel accumulation, and resolution/controller coverage.

## Release gates still open

- Broader search/set controls, result counts, and visible scroll-position feedback.
- Effective runtime metadata for sets and condition; consumables and rarity now have initial runtime evidence.
- Full sorting coverage, filtered transfers, drag and drop, controller tests, and exact input bounds across resolutions.
- Investigate exit-time C++ exception reports before calling shutdown clean.
- Object teardown, campaign switching, and failure restoration under invalid layouts.
- Full profile isolation; a temporary campaign copy now supports inventory transfer tests.
- Vortex packaging and installation/removal testing.

`tools/compare_inventory.py <backup save> <current save>` compares all three
serialized inventory blobs read-only. Whole save databases normally change on
game exit even when those inventory blobs are identical.

## Latest runtime evidence

Session `work/backups/inventory-observation-20260924-160745/` verified Storage
at 6 by 6, wheel offsets 0 → 3 → 10 → 0, and the last row with two items.
The empty Trash grid remains at its native 4 by 4. Tooltips clear when the
hovered item scrolls away and update correctly on the return scroll. Existing
set-marker art and set-bonus tooltips remain visible. ABC, rarity and chronological
sort controls rebuilt the grid; closing and reopening retained a valid layout.
No item-transfer callback was intentionally exercised and no day was advanced.

After Save and Quit to Desktop, the runner restored the original loader bytes
and removed the session marker. All 94 serialized records remain in Storage;
Backpack and Trash blobs are unchanged. **Storage is not byte-identical**: 99
changed bytes fall entirely within the `sequence` field of 91 records. All
other bytes match, including item names, charges and raw condition fields.
This occurred after native sorting and is consistent with ordering metadata,
but its semantics still need a control test. Evidence is saved in
`inventory-detail-comparison.json`; do not describe this session as an
unchanged inventory blob or restore the whole save merely to undo sort metadata.

Mewjector also recorded `0xE06D7363` on exit. The raw stack candidates contain
`uiautomationcore.dll`; the earlier passive session at `152258` has the same
exception and matching leading candidates. This suggests an automation/exit
interaction, but raw stack scanning does not establish causality. The control
session at `162114`, without our DLL and without a closing-window UI query,
reproduced the same exception and leading raw stack candidates. Our DLL is not
required to trigger it; the actual cause remains unresolved.
The game saved and the runner cleaned up; shutdown is not yet certified clean.

## Item recovery and new filter work

During session `163400`, the user intentionally moved 46 native items to Trash
and reported missing backgrounds and unreliable scrolling as Trash expanded.
At their request, session `164112` used the normal mod configuration to return
all 46 through native clicks. The saved campaign then contained 94 serialized
Storage records and zero Trash records. No day was advanced or save edited.
Session `164621` retained all three inventory blob hashes exactly.

Native All / Consumables / rarity / Reset controls have initial runtime evidence,
but remain experimental. New SWF assets require `swfs/swflist.gon.append`; missing export
registration throws a game exception. Generated art uses the OFL Patrick Hand
font. Native consumables use the runtime item definition's explicit boolean
`consumable` property; drawer `74` is a quest flag, `B0` quest metadata, and `B8`
slot sort order, not consumable or condition fields. `B4` rarity pairs normalize
1/2 common, 3/4 uncommon, 5/6 rare, 7/8 very rare, 10/11 legendary; 9 is quest.

`--test-slot` now copies campaign 1 into **absent** slot 3 for transfer testing.
Select the rightmost Home. It archives that copy to the session backup and
removes the temporary slot after exit. This isolates campaign inventory, not
the whole profile or Steam: settings/profile-level effects remain shared.

## Filter and populated Trash regression — session 165007

The generated bar renders at native layer 39. SWF pixel coordinates require
32 times the world-unit scale; omitting that conversion produced a tiny line.
All/Consumables, rarity selection, Reset, and Escape dismissal were exercised
through native input. Consumables showed 3; Consumables + Rare showed 2;
Consumables + Uncommon showed 0. Reset restored all 92 native drawers.
Reopening and filtering after transfers still showed the same 3 consumables.

The Trash background origin now comes from the panel's `ref_grid` bounds,
instead of assuming that its first background is the top-left cell. The game
creates Trash backgrounds bottom-up. We retain the original scale signs and
restore tracked offsets before each native rebuild. Both panels are captured
even when their native column count is below the six-column cap. Wheel handling
no longer rejects input based on the age of the latest drawer update.

On the temporary slot-3 copy, 37 items were moved into Trash through native
clicks. The grid retained backgrounds at 4, 5 and 6 columns. After item 37,
one wheel tick immediately produced `Scroll side=1 row=1/1`, without closing
the inventory, and revealed the final mushroom item. Clicking that last item
returned it and clamped the scroll position back to row 0. All 36 remaining
items were returned; Trash was empty with its background intact. Closing and
reopening preserved the layout. This verifies the reported expansion failure
for this build/session, not every input device or resolution.

After normal Save and Quit, every pre-existing save file was byte-identical;
only the temporary campaign changed. The main campaign still contains 94
Storage records and zero Trash/Backpack records. The test copy also ended with
94 Storage records; a multiset comparison conserved item names, subnames,
charges and all parsed raw properties except ordering sequence. Its inventory
blob differs after native transfers. See `transfer-verification.json` and
`save-comparison.json` in `work/backups/inventory-observation-20260924-165007/`.
The original loader bytes were restored, the marker removed, and slot 3
archived as `test-campaign-after.sav` then removed. No day was advanced.
The existing exit-time `0xE06D7363` report recurred after saving; this remains
an open investigation and is not evidence of a clean shutdown.

## Expanded filters and modal dropdowns — 24 September follow-up

The type dropdown replaces the separate All/Consumables buttons. It offers
All, Weapon consumables, Item consumables, Worn, Broken and Reusable weapons;
rarity remains an independent AND filter. Item consumables retain the previous
explicit `consumable` boolean behavior. Weapon consumables require runtime
`kind=weapon` plus positive `durability` (a number or two-number range).
Reusable weapons have no durability or nonpositive numeric durability.
These describe definition categories even when No Breaking Items changes
consumption behavior; they do not predict whether an item will actually break.
Unknown metadata is excluded from specific categories, but retained by All.

Condition is the per-item integer at `5C`: native renderer `7BB990` appends
`_Worn` for values 3/4 and `_Broken` for 5; tooltip routine `5BCE0` uses the
same cases. No save-field guesses or rarity inference are used. Metadata
comes from the effective runtime definitions through the same lookup as
native item accessor `2E0190`, so loaded data mods participate.

The guarded button hit-test hook at `97F0E0` returns false for inventory
item buttons while either dropdown is open. Native button updates continue,
allowing mouse-leave to clear tooltips. Popup input consumes mouse clicks
(including dismissing outside clicks) and wheel events; Escape dismisses it.
No item-container writes or synthetic transfers are involved.

Session `174052` on temporary slot 3 confirmed Weapon consumables = 3
(including the Lighter's ranged durability), Worn = 7 with native W marks,
Broken = 0, and Reusable weapons = 16. Hovering within the type dropdown did
not show the covered item's tooltip. The main campaign and all pre-existing
save files remained byte-identical. Arrow positions required a correction to
follow each panel's animated transform, to be verified in the subsequent session.

## Arrow and dropdown regression — session 174525

Arrows now use grid bounds relative to their panel plus its live transform,
so the opening animation cannot leave them offscreen. Storage showed down-only
at the top, both directions in the middle, and up-only at its last row.
Arrow clicks and wheel input reached the expected rows. Opening/dismissing a
dropdown preserved scroll position; changing filters reset it to the top.
Item consumables showed 3, combined with Rare showed 2, and both hid arrows.

The rarity popup suppressed item tooltips and wheel scrolling over covered
items. Clicking outside it on a Storage item dismissed the popup without
transferring that item. Native hover resumed after dismissal.

On the temporary campaign copy, transferring item 37 immediately displayed
Trash's down arrow with all backgrounds intact. Subsequent live input reached
row 1/1 (also recorded in the log); the last-row screenshot showed up-only.
Returning that final mushroom clamped the grid to row 0 and removed its arrows.
All remaining items were returned through native clicks, leaving Trash empty.

After Save and Quit, every pre-existing save remained byte-identical. The
archived copy retained all 94 Storage records and no Trash records. A multiset
comparison conserved names, subnames, charges and all parsed raw properties
except sequence. See `transfer-verification.json` and `save-comparison.json`
in `work/backups/inventory-observation-20260924-174525/`. The loader was restored
byte-for-byte, the session marker removed, and temporary slot 3 archived and
removed. No day was advanced. The previously observed exit-time `0xE06D7363`
report recurred; teardown remains unresolved.

Validation also passed the native predicate/scroll-boundary tests, 9 Python
tests, 4 Node tests, warnings-as-errors DLL compilation and the executable
guard harness. Broken had no matching items in this campaign, so its populated
UI case remains untested. Exact input edges at other resolutions, controllers,
filtered transfers, and release packaging remain separate gates.

## Global search and set selection — sessions 181301 / 182122

The Storage toolbar now has a dedicated search row and a second row for type,
rarity, set selection and Reset. Storage's six-column grid is scaled to 90% of
the previous experimental footprint to leave room for controls and live
Storage/Trash match counts. Search matches every whitespace-separated term
across the item's runtime display name, description and localized set names.
Named set selections use OR membership; search, type, rarity and set criteria
combine with AND. The set popup has its own search, nine visible checkbox
rows, wheel/paging controls, Clear and Done, plus All items, Any set membership
and No set. Counts respect the other filters and span both containers.

Metadata comes from native item-name/description routines at RVAs 7B4BE0 and
7B5480, the effective definition's set field, and the set registry at manager
offset 668. Localization uses 962430. Returned wide strings are copied and
destroyed with the game's allocator. Native text assignment 98E8A0 consumes
the supplied wide string. Formatting tags are removed from the search index;
image-token names such as shield remain searchable. No save records are edited.

Generated dynamic text uses an OFL Patrick Hand DefineFont3 asset. The first
DefineFont2 attempt was rejected at startup before loading a campaign; session
181153 was cleaned up. Text input uses the game's SDL exports and consumes
keyboard events while focused. Ctrl+A replacement initially failed because a
character keydown cleared selection; this was fixed and retested in 182122.
Clicking away from text focus consumes that first click. Set popups also block
underlying item tooltips, clicks and wheel events.

Observed runtime results:

- Global bionic: 7 items; shield: 25 items.
- Shield plus Bionic set: 2; adding Bone: 3 (session 181301).
- Set-menu search bionic narrows the list without changing inventory until
  selection; it does not replace the global shield query.
- No set: 15; unfiltered menu reports 77 with membership and 92 total.
- Set-list wheel scrolling, Clear, Done, Escape and toolbar Reset worked.
- In 182122, one visible item under shield + Bionic was transferred into
  Trash and returned through native clicks. Counts changed 2/0 to 1/1 to 2/0.
  Reset restored 92/0 and the Storage scroll arrow.

Both completed sessions left all pre-existing saves byte-identical. The final
archived copy retained 94 Storage records and empty Trash; all parsed item
properties were conserved except native ordering metadata. Evidence is in
work/backups/inventory-observation-20260924-182122/transfer-verification.json
and save-comparison.json. Loader configuration was restored byte-for-byte;
the marker and temporary slot were removed. No day was advanced.

Native search predicate tests cover case-insensitive English matching,
multiple terms, formatting removal, bounded output, unknown membership and
multi-set OR through bit 255. Native filter tests, existing Python/Node tests,
warnings-as-errors compilation and the executable guard passed. UI checks ran
at the existing wide resolution and a narrower letterboxed window.

This remains an explicitly launched diagnostic build, not a Vortex package.
The existing exit-time 0xE06D7363 report recurred. Controller input, IME
composition, full Unicode case folding/editing, prolonged play and broader
resolution coverage remain unverified. Registry capacity is 256 sets, queries
127 UTF-16 code units, and searchable text 4095 code units per item. Sets are
discovered from bound runtime items; this is not a catalog of every game set.

## Dropdown presentation — session 184340

Toolbar dropdowns now use separate right-aligned vector chevrons. Search,
type, rarity, set and Reset text share size 22; widths reserve space for the
chevrons. Single-word action buttons use measured text advances to center
their labels. Set display labels omit the English "Set Bonus!" suffix while
the original localized text remains in the search index. Long single-set
toolbar labels abbreviate with an ellipsis.

Type and rarity popups have selected-state variants with a green active
option. Rarity's Close button is removed; selecting an option, outside click
and Escape retain the existing dismissal behavior. Set modes ALL / ANY /
NO SET are mutually exclusive buttons in one fixed row; choosing a named
checkbox switches to custom multi-select and clears the mode highlight.
The nine checkbox rows, counts, list search and footer controls remain.

The generated assets contain 64 exports and the DLL compiled with warnings
treated as errors. Predicate tests and the executable guard passed. Runtime
checks confirmed the longest type label fits, changing/reopening type and
rarity moves the active highlight, compact set names render correctly, ANY
returns 77, NO SET returns 15, Bionic returns 7, adding Bone returns 10, and
ALL restores 92 with checkboxes cleared. No items were transferred during
these checks. Session 184340 subsequently closed; only its temporary slot 3
changed, and the runner archived the copy and restored the loader.

## Counts, sound feedback and popup layers — session 191055

Counts move 30 local UI units right to sit inside the lighter background:
12 in renderer placement plus a further 18-unit text inset requested after
testing. The additional inset takes effect on the next asset reload/launch.
Popup renderers use layer 40, above native item layer 39. The layer setter
now invokes the renderer's virtual notification at vtable +30 when active
(component byte +11), matching native setters in 211700 and 215820.
Previously the raw layer write skipped notification and popups shared the
item layer; newly created drawers after transfers could draw over them.
Existing popup input/tooltip blocking remains in place.

Quest is removed from the rarity menu, which now has six choices and three
rows. In 210590 the Storage iterator uses predicate 2149D0 (vtable EFF2E8),
which requires `quest_item == (owner.mode == 1)`. Thus normal Storage
excludes quest items; mode 1 is their separate view. This does not remove
items or alter the native Quest tab. Raw save counts include records outside
the normal Storage view and must not determine its drawer count.

Sound feedback reuses installed audio/ui_sfx.gon events: selector click for
opening/selecting controls and arrow buttons, ToggleButton On/Off Press for
set checkboxes, CloseButton Click for dismissal, and Settings Back Click for
reset/clear. No dedicated reset click event exists in this event set; the
back sound supplies a distinct clearing cue. Typing and wheel scrolling are
silent. Audio component lookup is 4A300; Play is 95B770 with volume/pitch 1,
delay 0 and final byte 0. Play consumes an owned narrow string constructed
with 520D0. This uses the game's existing SFX mixer, not external playback.

Assets now contain 57 exports. Warnings-as-errors DLL compilation, image
guard, and native filter/search assertions passed. Compile assertion tests
with -O0 -UNDEBUG: Zig's optimized build otherwise defines NDEBUG.
The game loaded the new build. Live logs show subsequent user interaction
with rarity choices, item types and Reset without a runtime failure, but
visual stacking after transfers and audible feedback are not yet verified.
Computer Use capture failed twice with "foreground window did not report a
process id", so automated visual checks stopped; no UI inputs were issued.

Session 191055 remains open for testing. A pre-existing slot 3 was backed up
byte-for-byte at session 191026/saves/steamcampaign03.sav before temporarily
using that slot for a fresh copy of campaign 1. The enclosing runner restores
that original slot after archiving the test copy on normal game exit. All
other existing save hashes still matched the session baseline during checks.
Do not restore an older main campaign snapshot. The loader restoration and
test-slot cleanup have not yet run for this still-open session.

Follow-up: session 191055 closed successfully. Only its copied campaign
changed; loader bytes and the pre-existing slot 3 were restored.

Button alignment correction: all button labels and popup choices now default
to centered text, including multi-word rarity options (Any rarity / Very rare)
and item-type options. Previously centering depended on the label containing
no spaces, and the type menu explicitly forced left alignment. Toolbar field
values retain their left inset and separate right-aligned chevron. The 57
SWF exports were rebuilt; this follow-up awaits the next test launch.

## Native font and visual feedback pass — 24 September 2026

The generated SWF now imports the installed `fonts.swf` dependency and uses
the game's `Edmundm` font class for all text. It contains no embedded font
outlines or copied game artwork. Original irregular paper panels use warm
fills, dark ink and a muted sage selection state. Most labels use size 20;
popup headings use 22 and counters use 18. Button and menu-option labels are
centered; toolbar values retain a left inset and separate right chevron.

Toolbar geometry is type x0/w230, rarity x238/w135, sets x381/w114 and
Reset x503/w68. The All sets field is 80 units wide. The count line retains
the requested total 30-unit right inset (18 in artwork plus 12 in placement).
Long selected set names are shortened only in the toolbar; menu labels remain.

Hover and pressed overlays sit at layer 42, above popup layer 40. Disabled
set-page buttons sit at 41 and receive no hover feedback. Popup interaction
remains modal. Existing native click, toggle, clear and dismissal sounds are
unchanged. These controls use original artwork, not extracted native buttons.

The generated assets have 61 exports. `tools/validate_native_assets.py`
independently checks the installed font metrics: all 311 text fields resolve
to the native font, all initial labels fit, and no font is embedded. This
does not substitute for visual testing of dynamic text. Warnings-as-errors
DLL compilation, executable image guard and filter/search assertions passed.

The user confirmed that labels rendered correctly in session 221704. Runtime
logs recorded filter, reset, scrolling and set-paging interactions. Computer
Use capture failed after its permitted retry with "foreground window did not
report a process id", so no automated visual verification is claimed. That
session exited cleanly: only the test campaign changed, the loader was
restored, and the pre-existing slot 3 was restored byte-for-byte. A subsequent
font-metric check identified insufficient width for All sets; the final
geometry above corrects it and awaits the next user test.

Final-width launch attempt 222334 paused in Steam before game creation.
Steam's console reports SynchronizingCloud/cloudconflict; its cloud log
identifies `steamcampaign03.sav` as the conflicting file. No cloud/local
choice was made. The original slot 3 was preserved separately under
`work/backups/preserved-slot3-20260924-222334/`; all campaign files were
backed up by the session runner. Resolve the Steam prompt before claiming
this final build has loaded. Automated launch times out after 120 seconds
and restores the loader and prior slot 3, so a later launch needs a fresh
managed test session.

Attempt 222334 timed out without starting the game. Cleanup confirmed the
original loader and pre-existing slot 3 restored byte-for-byte; other save
files were unchanged. The Steam conflict prompt still needs dismissal.

The user cancelled that prompt and then disabled Steam Cloud for Mewgenics
for copied-save testing. Session 224135 launched successfully (PID 29916).
The actual process arguments include all 13 existing mods in their configured
order and our generated assets last. Mewjector reports integrity ALL OK,
our executable guard and layout hooks loaded, and BiggerWallet installed its
hook. Final-width visual checks remain with the user. Select rightmost Home
(slot 3), the temporary campaign-1 copy. Its pre-existing save is preserved in
`work/backups/preserved-slot3-20260924-224135/`. Runner session 3508 remains
active to restore the loader and original slot after the game exits. Keep
Cloud disabled through cleanup; reassess local/cloud state before reenabling.

The user verified that All sets fits and that the updated UI looks and sounds
right. This is user visual/audio confirmation, not an automated capture.

## Rarity and set-catalog audit

The user correctly challenged the Legendary choice. A read-only scan of this
installed resources.gpak finds item rarity declarations common, uncommon,
rare, very_rare, their consumable variants, quest and sidequest; none are
legendary. `data/items/legendary_items.gon` contains Fancy Bow with rarity
very_rare. The wiki's Fancy Bow page confirms the same distinction:
https://mewgenics.wiki.gg/wiki/Fancy_Bow
The earlier interpretation of drawer ranks 10/11 as Legendary is unsupported
and must not be treated as verified. The four-rarity update below removes
that incorrect label and excludes special ranks from ordinary tiers.
Cursed is an independent item property, not a fifth ordinary
rarity tier; it can coexist with ordinary rarity or sidequest.

The installed `data/item_setbonuses.gon` contains 101 top-level set-bonus
definitions. This is a definition count, not proof that every set is
obtainable. Our menu is not a full game catalogue: iq_search_metadata registers
set IDs from encountered item definitions, and iq_set_rows_build lists that
session's registered IDs. Names come from the loaded game definitions and
localization, so mod changes can be reflected. Missing inventory sets are
not evidence of missing game sets. Wildcard set membership (`*`, used by Rune
of Perthro) also needs explicit semantics before claiming complete coverage.

## Four-rarity tile menu

Legendary is removed. ANY is a full-width button above a 2x2 grid of 150-unit
square Common, Uncommon, Rare and Very rare choices, with centered size-20
labels, selected fill/checkmark and existing sounds/hover feedback. The panel
is 330x376. Pointer gaps do not select neighboring tiles; the pure predicate
test covers all choices and edge/gap coordinates. Unknown/special ranks stay
visible under ANY but do not masquerade as one of the four ordinary rarities.

Rarity art is reused directly from installed `catparts.swf`: HeadItemIcon
frame 4 is blank except its rarity and sloticon children. The latter is hidden,
and the rarity child is stopped on uncommon, rare or very_rare. Common is
unadorned. Native GotoFrame at 9A88A0 is zero-based; GotoAndStop by label at
9A8C00 consumes its short inline string. Each renderer is generation-checked,
hidden with the popup, and layered at 41 between panel 40 and hover 42. No
native art is extracted into the distributed UI or replaced in catparts.swf.
The validator checks the native blank-frame/child/label contract as well as
270 generated text fields; all initial labels fit. Assets now have 54 exports.
DLL compilation, image guard and filter/geometry assertions passed.

Sessions 224135, 225732 and 230226 closed and restored their loader and
original slot 3. The first rarity checks exposed hidden artwork: native draw
at 9B1000 requires flag byte +8 bits 0x20 and 0x40 both set. Clearing 0x20
hides the object, as used by the native item-preview path. The control now
sets 0x20 on the icon/rarity and clears it on the separate slot badge.
Sessions 231324 and 232240 also closed and restored the loader/original slot 3.
The second artwork issue was the camera boundary: layers 35..40 and 41..46
use different cameras. Overlays now convert inventory coordinates and scale
to their target camera every update. This also fixes placement of hover and
disabled-page feedback. Camera lookup follows the scene's camera list and
layer masks; this guarded build uses axis-aligned cameras with equal projection.
Session 232240 visually confirmed all three native symbols. Their differing
registration points are compensated to keep artwork inside the tiles.
Session 232810 (runner 66251) tests the final placement; its pre-existing slot
is preserved in `work/backups/preserved-slot3-20260924-232810/`.

Final in-game checks in that session passed at 2560x1381 with the 13 existing
mods active: Common 21, Uncommon 33, Rare 29, Very rare 7, ANY 92; Trash 0
throughout. Each tile closes on selection, updates the toolbar, and shows a
selected fill/checkmark when reopened. The native gray circle, yellow triangle
and red diamond fit inside their tiles; Common stays blank. Wheel input over
the open popup did not scroll underlying items or expose their tooltips. A
click in the gap dismissed the menu without selecting a tier or moving an item.
The game remains open on the copied slot for the user's review; no day advanced.

The user requested 70% of the initial rarity popup size. The popup now scales
uniformly by 0.70 (231x263.2 effective units, 105-unit tiles), including labels
and native rarity artwork. Toolbar dimensions stay unchanged. Hit testing uses
the renderer's inverse transform, so its original local choice rectangles
remain correct; hover feedback now uses its parent control's actual scale.
Session 232810 closed and restored its loader and original slot 3. The smaller
menu is being checked in session 233811 (runner 83865), with original slot 3
preserved in `work/backups/preserved-slot3-20260924-233811/`.
Build passed. In-game verification confirmed the reduced popup, readable labels
and aligned symbols. Clicking the smaller Very rare tile returned 7 items;
reopening showed its selected checkmark, and ANY restored 92. Trash stayed 0.

## Centered rarity tiles and parent-local hover feedback

The rarity menu retains the 70% scale. Labels now sit centrally over the native
symbols, with per-symbol registration offsets; the green selected background
is the only rarity selection marker. Labels render at layer 42 over native art
at 41 and the popup at 40. Common remains unadorned. Set-list checkboxes remain.

Hover and pressed effects are now named children of each generated control.
Their gray fill and underline use the actual button/row rectangle, with fixed
local insets and a two-unit underline instead of a stretched 100x36 overlay.
They inherit the parent's position, scale and camera without a second placement
calculation. Runtime lookup is lazy because native movie children may appear
after renderer creation. Each update hides these children before activating
only the hovered eligible control; popup modality and disabled paging persist.

Build passed with warnings as errors. Asset validation resolved all 254 text
fields against native fonts, checked rarity artwork and found no text overflow.
There are 53 exported control symbols; feedback children are private symbols.
Session 234942 caught the initial child-lookup timing issue and art offsets,
then closed with loader and original slot 3 restored byte-for-byte. Session
235459 (runner 69503) is checking the corrected build; its original slot 3 is
preserved in `work/backups/preserved-slot3-20260924-235459/`.

Session 235459 verified centered symbol/text placement, Very rare selection
(7 items), green-only selection and ANY restoring 92. Runtime inspection
confirmed feedback children resolve (4 on the toolbar, 5 on rarity) and only
the hovered child is visible. The remaining separate disabled-page renderer
was visibly offset, so disabled Up/Down now also live inside the set popup.
The session closed and restored loader and original slot 3. Rebuild/asset
validation passed: 51 exports and 260 text fields. Final verification is in
session `inventory-observation-20260925-000057` (runner 36058); original slot 3
is preserved in `work/backups/preserved-slot3-20260925-000057/`.

Final in-game verification confirmed the disabled Up label lies exactly in its
button, becomes active on the second set page and disables again on returning.
Set-row, paging-button, item-type option, search-clear and storage-arrow hover
effects remain inside their own controls. Storage scrolling down/up worked;
the first row and all filters were restored. The centered rarity menu is open
for user testing with Storage 92 / Trash 0, no transfers or day advancement.
All 13 existing mods are active alongside the test build. Runner 36058 remains
active and will restore the original loader and slot 3 after the game closes.

## Rarity heading and consistent top choices

Added a Rarity heading and changed the rarity Any and set All / Any / No set
choices to sentence case. Rarity Any is 36 effective units tall, matching the
item-type and set buttons despite the popup's 0.70 scale. Its text is 19.6
effective units, close to the other 20-unit choices. Tiles keep their compact
size and centered artwork/text; their local top moves from 56 to 126 to make
room. Hit-testing and hover rectangles move with them.

Build with warnings as errors, native asset validation (265 text fields) and
the updated native filter tests passed. Previous runner 36058 finished with
loader and original slot 3 restored. Session inventory-observation-20260925-003236
(runner 46259) launched with the updated build and preserved the original slot
3 in work/backups/preserved-slot3-20260925-003236/. Visual verification remains
pending: computer-use returned "failed to activate captured window" on the
initial attempt and the single recovery attempt. Game left running for user
testing; select the rightmost Home for the temporary campaign copy.

## Dependency review before packaging

Renamed the item dropdown heading from Show items to Item type; regenerated
assets and validated all 265 native-font text fields successfully. Session
003236 finished and restored the loader and original slot 3 byte-for-byte.

The native DLL requires Mewjector's version.dll and v3 API (MJ_GetVersion >= 3,
MJ_InstallHook and MJ_Log). v3.0 is the tested loader. No other gameplay mod
provides an API or data required by our implementation: item/set metadata,
localization, fonts, symbols and sounds come from the game. SetBonusTooltips
and VisibleSetItemMarkers add their own displays; our set filtering does not
depend on them. Vortex is a deployment/launch option, not a runtime dependency;
neither Mewtator nor Mewgenics Mod Manager is required by the code.

Release packaging still needs to include our generated SWF and swflist append,
register their folder with the game asset loader, and replace the temporary
diagnostic session-marker activation with normal mod startup. The exact game
binary guard remains a compatibility requirement. A clean setup with just
Mewjector plus our mod must be verified before claiming standalone installation
support; current gameplay evidence uses the user's existing mod collection.

## Performance pass: derived set data and hover coordinates

The set menu previously rebuilt counts and alphabetical rows every UI update,
including two linear metadata lookups for every matching item. It now caches
those results by view revision, metadata revision, item type, rarity, global
query and set query. Capture attempts (including failed/empty captures) and
successful item metadata rebinds advance revisions. Set selections and paging
reuse the same counts because counts intentionally ignore the set selection;
page bounds still clamp each update. A rebuild reuses the metadata it already
found for global search matching. Normal renderer generation checks remain.

Hover feedback samples native mouse/inverse-transform coordinates once per
control per update, rather than once per option (up to 18 for the set popup).
The sample is local to that update, so moving panels/cameras are not cached
across frames. Scrolling at an unchanged boundary now skips drawer layout work.

The production set-menu builder is in inventory_set_rows.h so the fixture in
mods/improved-inventory/tests/native_set_rows.c exercises the actual code. With 1,024 items and 600
unchanged updates it performs 1,024 metadata lookups; the old all-matching
two-lookup path would perform 1,228,800. This is an operation-count comparison,
not an FPS or whole-game timing benchmark. Assertions cover search, type and
rarity changes, alphabetical rows, set-query edits, selection, paging, metadata
rebinds, missing/unknown metadata and empty/repopulated views. All three native
test programs passed with assertions explicitly enabled (-UNDEBUG); the DLL
build and supported/unsupported-image guard checks also passed.

Remaining profiling candidates include per-drawer linear lookup, repeated
button-pointer reads while dropdowns are open, and native grid capture costs.
These remain unchanged pending measured evidence and lifecycle validation.
Runtime test uses session inventory-observation-20260925-005206 (runner 2574),
with original slot 3 preserved in work/backups/preserved-slot3-20260925-005206/.

Runtime verification passed for set-row hover, paging, selecting Fighter (3
items) and restoring All (92), then Very rare (7) with fresh set counts (Bone
2, Bionic 0 versus the unfiltered 3 and 7). Reset restored Storage 92 / Trash 0.
The Rarity heading and sentence-case choices also render correctly. No items
were transferred and no day was advanced. The test copy remains open for the
user; runner 2574 will restore the original loader and slot 3 after exit.
