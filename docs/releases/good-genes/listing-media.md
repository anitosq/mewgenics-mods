# Good Genes listing media

Media uploaded and verified 2026-10-07. Nexus mod 547 remains unpublished.

## Saved Listing

The owner authorized adding the selected artwork, selector screenshot and a
stats-only inheritance comparison. The listing now has the v4 header and three
gallery images, in this order:

1. `00-good-genes-cover-v3.png`: Good Genes - Better Breeding and Mutation Control.
   Selected as the thumbnail. Nexus image `547-1791303128-763136010.png`.
2. `01-special-mutation-choice.png`: Special mutations - Keep Existing or Replace.
   Byte-identical copy of `Screenshot 2026-10-06 233322.png`.
   Nexus image `547-1791303208-1201799062.png`.
3. `02-higher-parent-stats.png`: Best of both parents - Higher base stats inherited.
   Nexus image `547-1791303209-2139749842.png`.

The stats comparison uses only the original stat panels from the owner's
`Screenshot 2026-10-06 235718.png`, `Screenshot 2026-10-06 235724.png` and
`Screenshot 2026-10-07 000004.png`. No numbers were regenerated or retouched.
It uses native game-font labels and a parent-to-child connector. The owner
requested larger, clearer text and removal of cat names; the uploaded version
has 90px title, 42px explanatory text, and 64px Parent/Child labels.
Keep these names absent from subsequent public versions of this comparison.

The child screenshot shows STR/DEX/CON/INT/SPD/CHA/LCK of 7/7/7/5/7/6/7,
matching the per-stat parental maxima. This is one owner-provided gameplay
example, not exhaustive breeding or manager verification.

Header crop, first-image thumbnail, three saved titles and the unpublished
status were verified on the saved mod page. Local proof:
`outputs/listing/good-genes/listing-media-proof.png`.
No mod archive uploaded; no Publish action taken. Other listing settings and
description remain unchanged. Historical preparation notes follow.

## Current Selection: V3 Cover And V4 Header

Owner approved the v3 thumbnail, but rejected the header's reduced DNA detail.
The cover is unchanged. The header was regenerated from scratch with built-in
image_gen using the approved cover and original DNA icon as references.

- Header: `outputs/listing/good-genes/header-good-genes-v4.png`, 1400 x 400.
- Prompt: adjacent `imagegen-header-v4-prompt.md`; hash in `media-manifest.json`.
- Original output is preserved as `header-good-genes-v4-original.png`.
- Future checks: compare both formats side by side; require all five DNA bars
  (two purple ends, three pink central bars), end openings and full contour.
  Keep the DNA separate from the body so details are not obscured.
- Header awaits owner approval; nothing uploaded or published.

## Clean Imagegen V3

Owner correction: foreground artifacts and discoloration were undesirable;
generate afresh with clean lettering and symbols, retaining background texture.
Future artwork checks must inspect text and symbol interiors for mottling,
scratches and discoloration separately from intentional paper texture.

- Cover: `outputs/listing/good-genes/00-good-genes-cover-v3.png`, 1920 x 1080.
- Header: `outputs/listing/good-genes/header-good-genes-v3.png`, 1400 x 400.
- Generated afresh with built-in image_gen from the original body-part/DNA
  references and v1 native-lettering reference. V2 artwork was not an input.
- Original outputs are preserved as adjacent `*-original.png` files. Export
  resizing and vertical header cropping do not redraw or retouch the artwork.
- Prompts are in `imagegen-v3-prompts.md`; hashes and provenance are in
  `media-manifest.json`, both beside the outputs. Lettering remains generated,
  not a direct rendering of the font. Previous versions are preserved.
- Prepared for owner review only. No upload or publication performed.

## Previous V2 Direction

Owner feedback, 2026-10-06: replace the repeated mutation grid with imagegen
art anchored on the supplied smiling body part. A supplied native DNA icon
may support the composition. Future revisions should retain that visual anchor
instead of returning to a generic repeated-parts background.

- Cover: `outputs/listing/good-genes/00-good-genes-cover-v2.png`, 1920 x 1080.
- Header: `outputs/listing/good-genes/header-good-genes-v2.png`, 1400 x 400.
- Caption for both: Good Genes.
- Built-in image_gen generated both compositions. The owner-provided native
  body-part image (`codex-clipboard-adf8afc1-de5d-4954-9f7e-5dc948667f24.png`)
  is the anchor; the native DNA reference
  (`codex-clipboard-043e4ab1-41e7-49b0-92b7-cd9d40c7b807.png`) is secondary.
- The v1 cover supplied the native lettering reference. V2 lettering is
  generated from that reference, not rendered with an embedded game font.
- Original imagegen outputs are preserved as adjacent `*-original.png` files.
  Final exports are resized; the header trims excess vertical paper only.
- Full prompts: `outputs/listing/good-genes/imagegen-v2-prompts.md`.
  Hashes and provenance: adjacent `media-manifest.json`.
- Both compositions and their 480-pixel-wide previews were visually inspected.
  Neither is uploaded. Owner review and live Nexus crop review remain pending.

## Previous V1 Deliverables

Files are kept locally under `outputs/listing/good-genes/`:

| File | Size | Caption | SHA-256 |
| --- | --- | --- | --- |
| `00-good-genes-cover.png` | 1920 x 1080 | Good Genes | `e5b6e8848fdca60ad9c50cda39b88bf0229c7f75ac25a0f2933e1672975ff49f` |
| `header-good-genes.png` | 1400 x 400 | Good Genes | `6671648e68ed3f6ead273b6d39696d595f9f8e552277fe2f241cffaf17117485` |

Both use the existing listing style: large native title lettering over a faded
grid of relevant game artwork. The shorter title is intentional; the Nexus page
retains Good Genes - Better Breeding and Mutation Control.

## Provenance

Direct composition, with no AI-generated imagery. Five mutation-part images
were cropped from owner-provided gameplay screenshots. Only the part artwork
is reused; these compositions do not depict the current selector layout.
The original screenshots were not modified or added as gallery screenshots.

- `codex-clipboard-3c6d6c3e-c04f-49a0-8627-059aa0f9111f.png`: two leg previews.
- `codex-clipboard-068e82bd-7f36-4644-a3af-25159c82bc6a.png`: two arm previews.
- `codex-clipboard-5f922894-7e7d-41f0-90fd-621a7d2a95a7.png`: incoming arm preview.
- Font: local game `fonts.swf`, DefineFont3 ID 6, export Edmundm, internal
  EdmundMcMillen_INTLv4 Regular. Only the seven title glyphs were extracted.

`media-manifest.json` beside the outputs records source hashes, crop coordinates,
dimensions, captions and output hashes. Editable compositions are adjacent SVGs.
Local recipe: `work/compose_good_genes.cjs`, using the existing glyph extractor.

Game artwork and lettering remain the property of their respective owners.
Listing compositions, cropped artwork and font outlines stay outside the player
archive and are not covered by the mod's original-code MIT grant.

## Review

Inspected both final images at full resolution and at 480 pixels wide. Title
lines are separated; mutation crops contain no old UI labels; titles remain
legible in the thumbnails. Live Nexus crop inspection is pending upload.
