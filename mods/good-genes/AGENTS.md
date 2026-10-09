# GoodGenes

- Owner naming decision, 2026-10-06: Good Genes; Nexus title is
  Good Genes - Better Breeding and Mutation Control. Use GoodGenes for install
  identity and good-genes for repository/release paths. Update DLL, asset gate,
  SWF symbols and manager mappings together; never deploy two renamed copies.
  Owner Vortex feedback, 2026-10-07: avoid a repeated Good Genes suffix. Keep
  the Nexus main-file display name identical to the full Nexus title so Vortex
  does not concatenate dissimilar page/file names. Check both names before
  future uploads; retain GoodGenes as the asset association, not a UI label.

- Owner release decision, 2026-10-06: replace the inherited third-party breeding
  patch table with our own implementation before distribution. No copied table
  may enter the source repository or release archive. Avoid naming other mods
  in its description. Record fresh owner gameplay
  confirmation for the replacement implementation before freezing a release.
- Owner release approval, 2026-10-07: approved the review-fix ZIP and requested
  public publication, superseding the earlier hidden-listing restriction.
  Version 0.1.0 is now frozen and public; never replace its archive or retag it.
  Use docs/releases/good-genes/0.1.0-publication.md for release evidence.
- Owner release approval, 2026-10-07: revised 0.1.1 candidate approved for
  publication without fresh gameplay confirmation. Version 0.1.1 is now frozen
  and public; never replace its archive or retag it. Publication approval is
  not testing evidence. See docs/releases/good-genes/0.1.1-publication.md.

- Owner correction, 2026-10-06: mutation replacement must use the game's visual
  UI, with a native skill-replacement-like Keep/Replace flow. Do not substitute
  Windows dialogs. Before delivery, check that no OS-dialog path exists.
- Event/combat mutations only; empty slots stay vanilla; overnight behavior is
  outside scope. Preserve the original rolled candidate and compatible parts.
- Owner decision, 2026-10-06: show the selector for valid occupied-slot changes
  involving a special on either side. Automatically apply only clear stat-only
  improvements; silently keep equal/downgrade stat changes. Owner extension,
  2026-10-08: stat trade-offs (some gain, some loss) also require a choice, using
  effective bonuses across the whole roll, not independent per-side decisions. Identical IDs
  never prompt. Check both special-to-stat and stat-to-special cases before delivery.
  Owner review, 2026-10-07: paired rolls must still offer that choice when another
  affected side rejects a stat-only change. Check both part orders; unknown
  definitions must block the whole roll, not be treated as optional trade-offs.
- Owner review, 2026-10-07: automatic replacements must also improve combined
  bonuses after deduplicating identical part-group/ID pairs. Include an unchanged
  counterpart in the comparison and stale-offer snapshot, but not as a changed
  UI comparison. Retain empty-slot vanilla behavior and explicit special choices.
  Validate negative IDs through definitions: -2 can be a valid missing part;
  undefined negative IDs must not be mistaken for ordinary unmutated parts.
  Cover the eyes.750/eyes.303 collapse, reverse side order, matching pairs,
  valid missing parts, and undefined negatives in deferred regression checks.
- Owner will test. Compilation and read-only inspection are allowed, but do not
  run tests, load the DLL, launch the game, install, or touch saves unless asked.
- Owner-requested audit fixes, 2026-10-09: derive effective IDs before comparing
  or previewing (absent facial pairs use -2). Head changes can hide/restore facial
  effects through CatHeadPlacements; include them in policy and visible choices.
  Check a head gain that loses an eye bonus, missing-pair penalties, and sequential
  rolls after a modal opens. Do not let later automatic writes stale an open offer.
  Package only outputs matching a recorded build, including shared asset helpers.
- Owner workflow correction, 2026-10-08: keep updated source, leave the game
  installation unchanged, and deliver an installable next-version `.beta` ZIP
  for owner testing. This supersedes the earlier fixed-version local-build
  handoff. Keep VERSION, startup log, archive name and package metadata aligned;
  check all four before handoff. Do not remove `.beta` before owner testing.
- Owner release override, 2026-10-09: explicitly approved promoting the beta.2
  implementation to official 0.2.0 and publishing it. This permits removing the
  beta label for this release, without implying gameplay-test confirmation.
  Keep overnight changes out, and preserve the installed game and saves.
  Official 0.2.0 is now public and frozen; never replace its archive or retag it.
  See docs/releases/good-genes/0.2.0-publication.md for evidence and limits.
- Owner visual feedback, 2026-10-06: keep the selector 640px wide in the native
  1280x720 canvas; size its height to the maximum effect rows across the offer.
  Keep 20px below action buttons and 20px above them when navigation is absent;
  reserve navigation space only when needed. Owner feedback, 2026-10-07:
  when present, give the 28px pager 16px clearance above and below; otherwise
  hide the entire row without reserving its space. Footer position must stay stable
  across pages. Center each effect line below its preview, including the combined
  stat value/icon width. Owner extension, 2026-10-08: use 18px effects in up to
  twelve 22px rows, black 20px effect stat icons, and a taller content-sized panel.
  Show inherited base stats plus active mutation bonuses in both columns;
  exclude class, levels, gear and temporary effects. Incoming increases are green,
  decreases red, unchanged values default. Never mutate a cat to generate a preview.
  Recheck the whole preview before applying and show unavailable totals honestly.
  Preserve
  mutation previews capped at 157.5x69 (25% smaller than the prior build),
  and smaller buttons. Center previews between headings and effects with space
  above and below. Use masked/visible preview bounds;
  unmasked coat textures previously made previews tiny. Preserve full effects
  without text paging; keep an offer if its full description will not fit.
  Omit the redundant "Special mutation" label.
- Owner paired-mutation feedback, 2026-10-07: identical left/right comparisons
  should appear once as Eyes/Arms/etc. Owner clarification: do not label a
  matching pair "Both"; it represents one mutation. Merge only within an affected pair
  with matching current IDs; preserve different current mutations and genuinely
  single-sided rolls. Keep Existing/Replace are the default actions. Use
  Replace Both only for multiple distinct comparisons in a paired roll (All
  for all limbs), since navigation never changes the original roll's write scope.
  Matching pairs and single-sided changes have no comparison pager; retain
  arrows only between different parts (owner clarification, 2026-10-08).
  Check matching pairs, differing pairs, and single-sided offers before delivery.
- Owner UI feedback, 2026-10-06: retain native panel artwork and UI audio, but
  restore the simple outlined action buttons; the native button skin was rejected.
  Check that hover/click uses the native Button sound prefix, panel-open audio
  fires once per offer, and the scale-in stops at the final layout. Do not bundle
  extracted game assets. Leave runtime appearance/audio checks to the owner.
- Owner crash report, 2026-10-06: Unstable DNA exposed an unrelocated short JNE
  in Mewjector v3's SetPiece trampoline. Before adding or changing native hooks,
  inspect every copied relative branch and its destination, not just instruction
  boundaries. SetPiece requires the guarded 28-byte span; retain its deferred
  regression check in `test_hook_layout.py`.
- Owner crash report, 2026-10-06 22:16: native button cloning crashed at DLL
  RVA 0x3390. MovieClip+0xd0 points to raw timeline data, not a polymorphic library
  definition. Match native allocation/construction and destruction ownership;
  never reuse a library-definition virtual call on an internal data pointer.
  Retain the deferred sprite-construction check in `test_hook_layout.py`.
