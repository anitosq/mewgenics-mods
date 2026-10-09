"""Deferred static regression check; never loads the DLL or launches the game."""
from pathlib import Path
import re


def check_set_piece_branch():
    root = Path(__file__).resolve().parent
    header = (root / "build/guard.h").read_text(encoding="ascii")
    match = re.search(r"SET_BYTES\[\] = \{([0-9,]+)\};", header)
    assert match, "Build first to generate the guarded instruction bytes"
    code = bytes(map(int, match[1].split(",")))
    assert code[11:13] == b"\x75\x0f", "Expected SetPiece's short JNE"
    target = 13 + int.from_bytes(code[12:13], "little", signed=True)
    assert 17 + 6 <= target < 17 + 14, "Old span must reproduce the unsafe branch"
    assert len(code) == target == 28, "JNE must land exactly on the new jump back"
    assert code[-5:] == b"\x48\x83\xc4\x28\xc3", "Body case must retain stack cleanup and RET"
    native = (root / "native.c").read_text(encoding="ascii")
    assert "(void **)&original_set, sizeof(SET_BYTES)}" in native
    assert "install(hooks[i].rva, hooks[i].stolen," in native

    match = re.search(r"BUTTON_UPDATE_BYTES\[\] = \{([0-9,]+)\};", header)
    assert match
    code = bytes(map(int, match[1].split(",")))
    # Six complete instructions; no relative addressing or copied branches.
    assert code == bytes.fromhex("40 55 57 48 8b ec 48 83 ec 48 48 8b f9 48 8b 49 38")
    assert "(void **)&original_button_update, sizeof(BUTTON_UPDATE_BYTES)}" in native


def check_sprite_construction():
    source = (Path(__file__).resolve().parent / "selector.h").read_text(encoding="ascii")
    clone = source.split("static byte *clone_clip(", 1)[1].split("static byte *selector_button(", 1)[0]
    # MovieClip+0xd0 starts with a frame count, not a virtual-method table.
    # Regression: old code dereferenced it and crashed at DLL RVA 0x3390.
    assert "vtable[3]" not in clone
    assert "0x9694c0)(base+0x1420030)" in clone, "Use the native MovieClip pool"
    assert "0x9a7c00)(storage, timeline)" in clone, "Construct with raw timeline data"
    assert "frames < 1" in clone


def check_birth_hooks():
    root = Path(__file__).resolve().parent
    header = (root / "build/guard.h").read_text(encoding="ascii")
    for name, expected in (
        ("INHERIT_PART_BYTES", "48 8b c4 48 89 58 08 48 89 68 10 48 89 70 18 57 48 81 ec 80 00 00 00"),
        ("INHERIT_STAT_BYTES", "48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 20 8b 02"),
    ):
        match = re.search(name + r"\[\] = \{([0-9,]+)\};", header)
        assert match and bytes(map(int, match[1].split(","))) == bytes.fromhex(expected)
    gates = header.split("BIRTH_GATES[] = {", 1)[1].split("};", 1)[0]
    rows = re.findall(r"\{0x([0-9a-f]+), (\d+), \{([0-9,]+)\}, \{([0-9,]+)\}\}", gates)
    assert len(rows) == 3
    targets = {0xa9580: 0xa9636, 0xa9728: 0xa97c1, 0xa9e42: 0xa9fad}
    for address, size, before, after in rows:
        address, size = int(address, 16), int(size)
        code = bytes(map(int, after.split(",")))
        assert len(bytes(map(int, before.split(",")))) == size == len(code)
        assert code[0] == 0xe9 and code[5:] == bytes([0x90]) * (size-5)
        assert address + 5 + int.from_bytes(code[1:5], "little", signed=True) == targets[address]


def check_footer_layout_contract():
    root = Path(__file__).resolve().parent
    source = (root / "selector.h").read_text(encoding="ascii")
    assets = (root / "assets.py").read_text(encoding="ascii")
    # Runtime movement needs separately named captions and the divider in the SWF.
    for name in ("no_caption", "yes_caption", "effect_divider"):
        assert f'"{name}"' in source and f'"{name}"' in assets
    assert "float effects_bottom = 368 + effect_rows*22;" in source
    assert "float pager_y = stats_bottom + 16;" in source
    assert "float button_y = navigation_needed ? pager_y + 28 + 16 : stats_bottom + 20;" in source
    assert "if (navigation_needed) *(float *)(child+0x74) += pager_y-479;" in source
    assert "else child[8] &= (byte)~0x20;" in source
    assert "float height = button_y + 30 + 20 - 170;" in source
    assert "if (rows > effect_rows) effect_rows = rows;" in source
    assert "skin_selector(effect_rows);" in source
    assert "range(12)" in assets and "EFFECT_ROWS = 12" in source
    assert "if (rows > EFFECT_ROWS) described = 0;" in source, "Never offer truncated effects"
    # Text and icons share the same measured-row offset; redraws reset text X.
    assert "float shift = (272-(float)advance)/2;" in source
    assert "*(float *)(text_clip+0x70) = shift;" in source
    assert "*(float *)(stat_art[side][stat]+0x70) += shift;" in source


def check_selector_navigation():
    source = (Path(__file__).resolve().parent / "selector.h").read_text(encoding="ascii")
    navigation = source.split("if (choice->action >= 2) {", 1)[1].split("Offer verify = active;", 1)[0]
    assert navigation.count("render_comparison();") == 1, "Navigate first, then render once"
    assert "view_count-1" in navigation and "% view_count" in navigation
    assert "view_page" not in source and "text_page" not in source
    assert "result = combine_mutation_decisions(result, decision);" in source
    assert "result == INVALID_MUTATION) return KEEP_MUTATION;" in source


def check_paired_comparisons():
    source = (Path(__file__).resolve().parent / "selector.h").read_text(encoding="ascii")
    parts = (Path(__file__).resolve().parent / "parts.h").read_text(encoding="ascii")
    # Group only counterparts, never arms with legs or different current IDs.
    assert "if (part >= 11 && part <= 14) return 3 + (part-11)/2;" in parts
    assert "if (part >= 15 && part <= 20) return 6 + (part-15)/2;" in parts
    assert "a->before_id == b->before_id && a->after_id == b->after_id;" in source
    assert "!same_comparison(&active.parts[view_indices[j]], part)" in source
    assert "view_labels[j] = part->part;" in source, "Single-sided rolls retain their side"
    assert "view_labels[j] = comparison_group(part->part);" in source
    assert 'panel_text("part", part_name(view_labels[view_part]));' in source
    assert 'L"Both eyes"' not in source and 'L"Keep Both"' not in source
    assert "if (scope >= 3 && scope <= 8 && view_count > 1) {" in source
    assert 'scope == 5 ? L"Replace All" : L"Replace Both"' in source
    assert "navigation_needed = view_count > 1;" in source
    assert source.count("navigation_needed =") == 1, "Only differing parts need navigation"
    assert "original_set(active.cat, active.pair);" in source, "Keep the original roll atomic"


def check_effective_bonuses_and_missing_parts():
    root = Path(__file__).resolve().parent
    source = (root / "selector.h").read_text(encoding="ascii")
    native = (root / "native.c").read_text(encoding="ascii")
    snapshot = source.split("static MutationDecision snapshot_offer(", 1)[1].split("static int snapshot_unchanged(", 1)[0]
    assert "id < 0" not in snapshot, "Definitions, not sign, validate missing-part IDs"
    assert "q.kind = id < 0 ? UNKNOWN : UNMUTATED;" in native
    assert "p == 11 + ((part-11)^1)" in snapshot, "Capture an unchanged counterpart"
    assert "if (!selected && !counterpart && !facial) continue;" in snapshot
    assert "effective_part_id(current, i)" in snapshot
    assert "effective_part_id(projected, i)" in snapshot
    assert "effective_stats_decision(before, after, offer->count)" in snapshot
    assert "return special ? CHOOSE_MUTATION : combined;" in snapshot
    assert "if (part == 10 && p != 10) {" in snapshot
    assert "if (part->part == 10 && part->offset != 0x78) continue;" in source
    assert "if (!part->selected) continue;" in source
    assert 'L"Missing part"' in source


def check_base_stat_preview():
    source = (Path(__file__).resolve().parent / "selector.h").read_text(encoding="ascii")
    preview = source.split("static int preview_base_stats(", 1)[1].split("static int snapshot_unchanged(", 1)[0]
    assert "+ 0x6f0" in preview and "read_parts(offer->cat, current)" in preview
    assert "effective_stats(parts[side], COUNT(mutation_slots), &total)" in preview
    assert "offer->parts[j].offset == offset) next = offer->parts[j].after_id" in preview
    assert "inherited[i] + total.stats[i]" in preview
    assert "original_set" not in preview, "Preview must not mutate the live cat"
    assert "base_stats_valid = preview_base_stats(&active, base_stats);" in source
    assert "memcmp(stats, base_stats, sizeof(stats))" in source, "Reject stale whole-cat previews"
    assert 'L"Stats unavailable"' in source
    assert "base_stats[1][stat] != base_stats[0][stat]" in source


def check_head_projection_and_roll_order():
    source = (Path(__file__).resolve().parent / "selector.h").read_text(encoding="ascii")
    head = source.split("static int project_head(int id, MutationPart parts[15]) {", 1)[1].split("static byte *first_clip", 1)[0]
    assert 'small_string("CatHeadPlacements")' in head
    assert '"leye", "reye", "leye", "reye", "lear", "rear", "mouth"' in head
    assert "vtable[2])(clip, 1)" in head and "original_set" not in head
    snapshot = source.split("static MutationDecision snapshot_offer(", 1)[1].split("static int preview_base_stats(", 1)[0]
    assert "part == 1 && !project_head(id, projected)" in snapshot
    assert "int facial = part == 1 && i >= 7 && i <= 13;" in snapshot
    assert "facial && next.kind == UNMUTATED ? KEEP_MUTATION" in snapshot
    assert 'panel_text("yes_caption", L"Replace Head")' in source
    offer = source.split("static int offer_mutation(", 1)[1].split("static void selector_panel", 1)[0]
    gate = offer.index("if (!active_prompt && !queue_count && !pumping) {")
    assert gate < offer.index("snapshot_offer(&offer)") < offer.index("if (decision == VANILLA_MUTATION) return 0;")
    assert "offers[(queue_head + queue_count++) % COUNT(offers)] = offer;" in offer
    assert "snapshot_offer(&active)" in source, "Every deferred candidate must be re-evaluated in order"


if __name__ == "__main__":
    check_set_piece_branch()
    check_birth_hooks()
    check_sprite_construction()
    check_footer_layout_contract()
    check_selector_navigation()
    check_paired_comparisons()
    check_effective_bonuses_and_missing_parts()
    check_base_stat_preview()
    check_head_projection_and_roll_order()
    print("Hook, sprite-construction, footer-layout, and selector source contracts match.")
