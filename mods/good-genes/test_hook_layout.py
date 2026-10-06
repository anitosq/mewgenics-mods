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
    assert "float effects_bottom = 368 + effect_rows*26;" in source
    assert "float button_y = effects_bottom + (navigation_needed ? 41 : 20);" in source
    assert "float height = button_y + 30 + 20 - 170;" in source
    assert "if (rows > effect_rows) effect_rows = rows;" in source
    assert "skin_selector(effect_rows);" in source
    # Text and icons share the same measured-row offset; page redraws reset text X.
    assert "float shift = (272-(float)advance)/2;" in source
    assert "*(float *)(text_clip+0x70) = shift;" in source
    assert "*(float *)(stat_art[side][stat]+0x70) += shift;" in source


if __name__ == "__main__":
    check_set_piece_branch()
    check_birth_hooks()
    check_sprite_construction()
    check_footer_layout_contract()
    print("Hook, sprite-construction, and footer-layout source contracts match.")
