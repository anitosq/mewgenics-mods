# GoodGenes

- Owner naming decision, 2026-10-06: Good Genes; Nexus title is
  Good Genes - Better Breeding and Mutation Control. Use GoodGenes for install
  identity and good-genes for repository/release paths. Update DLL, asset gate,
  SWF symbols and manager mappings together; never deploy two renamed copies.

- Owner release decision, 2026-10-06: replace the inherited third-party breeding
  patch table with our own implementation before distribution. No copied table
  may enter the source repository or release archive. Keep Nexus unpublished
  and avoid naming other mods in its description. Record fresh owner gameplay
  confirmation for the replacement implementation before freezing a release.

- Owner correction, 2026-10-06: mutation replacement must use the game's visual
  UI, with a native skill-replacement-like Keep/Replace flow. Do not substitute
  Windows dialogs. Before delivery, check that no OS-dialog path exists.
- Event/combat mutations only; empty slots stay vanilla; overnight behavior is
  outside scope. Preserve the original rolled candidate and compatible parts.
- Owner decision, 2026-10-06: show the selector for valid occupied-slot changes
  involving a special on either side. Automatically apply only clear stat-only
  improvements; silently keep equal/incomparable stat changes. Identical IDs
  never prompt. Check both special-to-stat and stat-to-special cases before delivery.
- Owner will test. Compilation and read-only inspection are allowed, but do not
  run tests, load the DLL, launch the game, install, or touch saves unless asked.
- Owner correction, 2026-10-06: keep the current version fixed during local
  testing. Before changing version strings or archive names, confirm that the
  task is release preparation or the owner explicitly requested a version bump.
  Use existing build hashes to distinguish local rebuilds.
- Owner visual feedback, 2026-10-06: keep the selector 640px wide in the native
  1280x720 canvas; size its height to the maximum effect rows across the offer.
  Keep 20px below action buttons and 20px above them when navigation is absent;
  reserve navigation space only when needed. Footer position must stay stable
  across pages. Center each effect line below its preview, including the combined
  stat value/icon width. Preserve 20px effects, black 22px native stat icons,
  mutation previews capped at 157.5x69 (25% smaller than the prior build),
  and smaller buttons. Center previews between headings and effects with space
  above and below. Use masked/visible preview bounds;
  unmasked coat textures previously made previews tiny. Preserve full effects
  through pagination and omit the redundant "Special mutation" label.
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
