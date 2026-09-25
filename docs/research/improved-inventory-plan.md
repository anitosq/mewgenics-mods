# Improved Inventory: proposed mod plan

Plan: 24 September 2026. Implementation has begun with a saved-inventory browsing prototype and a native diagnostic build; stage 0 remains incomplete. See [implementation findings](inventory-feasibility.md). Confirmed user priority: the first playable version should provide readable inventory with scrolling and better filters. Batch cleanup and a richer set browser follow separately.

## Recommendation

Build one inventory mod with independently configurable features, delivered in stages. Browsing, set discovery, and cleanup share the same item records, selection rules, and screen. Separate mods modifying that screen would add integration work and potential conflicts. Keep implementation components separate internally; split distribution only if investigation finds a useful feature that truly works independently.

Focus first on the household inventory workflow after adventures. The goal is to find, compare, keep, and move items quickly even with hundreds of retained items. Adventure inventory, shops, cat loadout automation, and expanded storage capacity are outside the initial scope.

## What we know, and what we still need to establish

| Finding | Design consequence |
| --- | --- |
| User reports the grid shrinks instead of scrolling as item count increases | Maintain readable cell size and introduce scrolling; investigate pagination as a fallback if native scrolling proves impractical |
| Installed Visible Set Item Markers is v1.3.1 and adds numeric name prefixes for ABC sorting | Preserve compatibility, but use actual set membership for grouping rather than parsing display numbers |
| Installed item definitions contain `rarity`, single `set` values, and arrays such as `set [Bionic, Cyborg]` | A set filter must support multiple memberships; one item must still be moved only once |
| No Breaking Items' inspected weapon patch sets `degrade_after_adventure false` | Do not assume every break mechanism is disabled, or that existing worn/broken items become pristine |
| Consumables and some equipment both have limited uses | Classify consumables by the game's actual item category, not by the mere presence of charges |
| Household trash is emptied at day end | Batch cleanup should move to the existing trash container; recovery has a limited lifetime |
| Mewjector v3.0 and Bigger Wallet's hook have been verified locally | DLL delivery is available; a candidate renderer hook now matches reference bytes, but live inventory access and UI integration remain unverified |

The archive index contains `data/item_setbonuses.gon`, `data/items/consumables.gon`, and item definitions, plus `swfs/house.swf`, `swfs/house_interstitials.swf`, and `swfs/ui.swf`. These are investigation leads, not proof of which asset or function owns the inventory screen. Installed mod files demonstrate schema fields; effective values must account for the game's loaded definitions and other mods.

Sources: [Visible Set Item Markers](https://www.nexusmods.com/mewgenics/mods/225), [Items](https://mewgenics.wiki.gg/wiki/Item), [House](https://mewgenics.wiki.gg/wiki/House), and read-only inspection of the installed mods and archive index.

## Proposed user workflow

```mermaid
flowchart LR
    A[Open household inventory] --> B[Choose container and filters]
    B --> C[Browse readable items or sets]
    C --> D[Select matching items]
    D --> E[Review exclusions and destination]
    E --> F[Move to storage or trash]
    F --> G[Review result or undo while valid]
```

Extend the existing in-game Storage/Trash screen, retaining its two visible destinations and familiar item interactions. Expand the filter bar for search, item type, and clear filters. Put slot, rarity, and set choices behind a “More filters” button that opens a focused popup; show applied choices as removable chips on the main screen. Cancel/Escape discards pending popup changes. The popup must capture input so clicks cannot reach items behind it. A separate in-game panel remains a technical fallback, not the preferred workflow.

The browser development harness uses a location selector for saved containers; that selector is not a decision to replace the native two-panel layout. Incoming/backpack inventory should appear only where the game actually exposes it. Equipped items, if shown, are reference-only in the first version.

An example cleanup task: choose Storage, show Common and Uncommon consumables, keep “Protect set items” enabled, select all matches, review the count, then move to Trash. A second task: open Sets, choose a named set, and see its available pieces with their slots and locations. These should take a few deliberate actions without searching tiny icons.

## Feature design

### 1. Readable browsing

- Fixed minimum icon and text size, with a density option and a scrollable viewport. Increasing inventory size must add rows rather than shrink every item.
- Optional compact list view: icon, name, slot/type, rarity, condition, uses, sets, location. This is useful for comparisons that icons alone cannot explain.
- Search by item name and set name initially. Description/effect search can follow once localization and effective descriptions are understood.
- Stable sorting and scroll position after selection or transfers. Preserve existing tooltips and single-item interactions where possible.
- Show counts such as “42 matching / 386 total” and “18 selected.” Keep total items and displayed groups distinct if duplicate grouping is added later.

### 2. Filters that compose predictably

| Filter | Proposed choices |
| --- | --- |
| Type | Equipment, consumables; equipment slots separately |
| Rarity | Common, Uncommon, Rare, Very Rare; separate Quest/Side Quest/Unclassified choices where supported |
| Condition | Worn, broken, neither; remaining uses as a separate filter if exposed |
| Sets | Any set, no set, or one/more named sets |
| Location | Storage, Trash, incoming inventory if present |
| Personal state | Protected/favorite; later new-since-last-review and duplicate groups |

Selections within a filter use OR; different filters use AND. Example: `(Common OR Uncommon) AND Consumable AND Not in any set`. Protection rules then remove items from the action selection. Show active filter chips and a clear-all control; do not require a query language.

“Low tier/quality” initially means explicitly chosen rarities, not an invented strength score. A common item can be valuable for a build. Worn, broken, cursed, remaining uses, and rarity are distinct properties. Unknown metadata must remain visibly unknown rather than being classified as low-value or non-set.

### 3. Set discovery

- Group by set name with its bonus available in a details panel. Support searching by the existing marker number only as an optional compatibility convenience.
- Show owned copies, available slots, condition, and location; make it easy to isolate all matching pieces.
- Include a multi-set item in each relevant set view, but retain one underlying item identity for actions.
- Separate “pieces owned” from “can equip enough matching slots.” Three copies of a single-slot item are not automatically a usable set.
- First release can show membership and slot coverage. Later, evaluate completion for a selected cat, including locked slots and abilities that change set requirements. Do not label a set ready for that cat until those rules are verified.
- Treat broken pieces as owned but not necessarily usable; expose this distinction.

The wiki describes ordinary set activation through matching equipment in three slots. Runtime rules should remain authoritative for exceptions and other mods. [Item sets](https://mewgenics.wiki.gg/wiki/Item_sets/Table)

### 4. Batch selection and trash transfer

- Individual selection, range selection, select all filtered results, deselect, and clear selection. Clearly distinguish all filtered results from just the visible page.
- Changing filters clears the action selection in the first version, avoiding invisible selected items. Sorting preserves selections by item identity.
- Batch actions: Move to Trash and Restore to Storage. Use the same game operations and capacity rules as normal transfers.
- Default bulk protections: set items, explicitly protected items, and quest/special items. Set protection is a visible option the user can switch off deliberately.
- Equipped items and records with unknown eligibility are excluded from bulk actions initially. Show why an item was excluded.
- Before moving, show an inspectable selection, count, destination, and exclusions. The action button says “Move 18 items to Trash.” Do not introduce automatic permanent deletion.
- Revalidate the selected item instances and destination capacity when the action executes. A stale view must not move the item that happens to occupy an old index.
- Report moved and skipped counts. Never silently displace other stored items when restoring to a full container.
- Aim for undo of the most recent batch while the same inventory session and item identities remain valid. If reliable undo cannot be implemented, omit the promise and retain normal manual recovery from Trash. Day advance, item consumption, or reload can invalidate undo.

For a persisted “Protected” feature, decide whether protection applies to one physical copy or every copy of an item type; label the choice clearly. Persistent per-copy protection depends on finding stable identities across saves. Start with session protection if that prerequisite fails, and label it as temporary.

### 5. Later QoL worth considering

1. Saved views and cleanup presets, such as “Non-set consumables” or “Worn equipment.” Preview results each time; no unattended cleanup in the first releases.
2. Duplicate grouping and an explicit “keep N copies” rule. Copies with different uses, condition, or special state are not interchangeable by default.
3. A “new since last review” view, once we can track arrivals reliably. Do not infer arrival time from a changing sort order.
4. Watchlists for desired sets and an indication that a newly acquired item fills a missing slot.
5. Restore protected or set items from Trash before day end, subject to available storage space.
6. Remembered filters, sort, and density per container; keyboard/controller navigation where the UI route permits it.
7. Effect search, item comparison, and later cat-specific set/loadout planning.

## Implementation route and feasibility gates

This is likely to require executable/UI work, potentially a Mewjector DLL plus assets. Data patches alone should not be assumed to provide scrolling, new input handling, live per-copy state, or batch transfers.

Investigate extending the existing household screen first. If its layout/input code is too tightly coupled, a separate in-game inventory panel may be more practical. Choose only after inspecting the rendering and input paths. An external live save editor is not the proposed solution.

Keep four implementation responsibilities separate: reading effective item definitions and instances; filtering/grouping; presentation and selection; validated transfers. This allows changing the UI approach without rewriting cleanup rules.

Before committing to a playable version, establish:

1. Where storage, trash, and return-from-adventure items live, and how each is identified during a session.
2. How to read actual category, rarity, set memberships, condition, uses, and transfer restrictions, including unknown/custom items.
3. Which function performs the game's own item transfer, its capacity behavior, and the appropriate point to call it.
4. How to render readable rows and intercept scrolling without leaking clicks into the underlying game.
5. How item changes, save/reload, and day advance affect pointers/identities and any protection metadata.
6. How to detect an unsupported game build and disable unsupported functionality without moving items.

If item identity or transfer behavior cannot be established, stop at a read-only browser until it can. If native scrolling cannot be added reliably, assess pagination or a separate in-game panel before writing the rest of the UI.

## Delivery sequence

| Stage | Deliverable | Completion evidence |
| --- | --- | --- |
| 0: Feasibility | Read-only item listing and a minimal UI/scroll experiment on a test setup | Counts, identities, set metadata, conditions, and supported game build verified; transfer route identified |
| 1: First playable browser | Readable scrollable inventory, consumables filter, composed filters/search, stable sorting, basic named-set grouping | Hundreds of items remain accessible and readable; filtering never changes inventory contents |
| 2: Cleanup | Multi-select, set protection, review, batch move/restore; undo only if proven reliable | Correct physical items move once, capacity respected, protected/quest/equipped items excluded, save/reload retains valid inventory |
| 3: Set tools and convenience | Rich set browser, persistent protections where supported, saved presets, duplicate policies | Multi-set membership, slot coverage, and per-copy differences handled correctly |

The first release is stage 1, matching the user's confirmed priority. Preserve ordinary single-item actions if integration allows, but do not hold this release for batch transfers, undo, or rich set completion logic. Stage 2 is a subsequent release after transfer behavior is validated.

## Compatibility and acceptance checks

Use an isolated test save/setup established before write tests. Do not treat the user's current household as disposable test data.

- Test empty, small, 500-item, and 1,000-item views where the test setup permits those counts; verify all items remain reachable and measure responsiveness on the actual machine. Render only visible rows if needed.
- Check normal/worn/broken instances, exhausted or partially used items, consumables versus limited-use equipment, quest/no-rarity/custom items, and single/multi-set membership.
- Exercise duplicates and filter changes: selection must refer to physical instances, and a multi-set item must never be moved twice.
- Check full storage, partial batch failures, return from adventure, day advance, reopening the panel, and save/reload. Verify no missing/duplicated items and no use/condition changes caused by sorting.
- Start with the mod alone, then add No Breaking Items, No Curses On Items, Better Loot, Bigger Storage Early, Visible Set Item Markers, and Set Bonus Tooltips. Their changed data should be reflected rather than silently replaced with vanilla assumptions.
- Avoid replacing item names or `catparts.swf` merely to label sets. Use original UI assets or permitted integration; do not copy and redistribute other mod authors' files without permission.
- Test alongside Bigger Wallet/Mewjector and record exact game/loader versions. Check tooltip behavior, display scaling, and whichever input methods are claimed supported.
- Removing the mod should leave ordinary inventory usable. Keep UI preferences separate from gameplay data where feasible; validate save effects before offering persistent item marks.

Package through Vortex using the existing project workflow. If a DLL is required, register its actual path under Mewjector's `[LoadOrder]` without narrowing `ScanPath` to a single mod folder. Do not assume Vortex's asset checkbox alone controls every DLL load path.

## Decisions still open

- First priority is decided: readable browsing, scrolling, and better filters. The order of later cleanup and richer set tools can be revisited after using the first release.
- Native screen extension versus a separate in-game panel: a feasibility decision.
- Persistent protection per copy versus per item type: decide after identity research.
- Whether worn-item cleanup is useful with the user's current degradation mod: keep it optional, never a default disposal rule.

No schedule estimate is justified until stage 0 establishes how much inventory/UI functionality is accessible.
