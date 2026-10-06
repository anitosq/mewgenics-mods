"""Build the in-game selector. Reference the installed font; do not bundle it."""
import importlib.util
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / "auto-furniture"))
spec = importlib.util.spec_from_file_location("furniture_assets", ROOT.parent / "auto-furniture/assets.py")
assets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assets)


def build(game, tools):
    sys.path.insert(0, str(tools))
    from probe_inventory import Gpak
    from validate_native_assets import unpack_swf, font_advances
    # Read metrics for wrapping, without running that module's validation/tests.
    metrics = font_advances(unpack_swf(Gpak(game / "resources.gpak").read("swfs/fonts.swf")))["Edmundm"]
    header = "static const struct { unsigned code; double width; } glyphs[] = {\n"
    header += ",\n".join(f"{{{code},{max(0, advance) / 20480:.9f}}}" for code, advance in sorted(metrics.items()))
    (ROOT / "build/ui-font.h").write_text(header + "\n};\n", encoding="ascii")
    m = assets.Movie()

    def button(name, x, y, w, caption, h=30):
        frames = []
        # Native Button selects these labels for mouse and controller feedback.
        for label, color in (("up", (232,225,206,255)), ("over", (194,215,190,255)),
                             ("down", (164,193,161,255)), ("disabled", (195,191,181,255))):
            paper = m.sprite("fallback", m.paper(x, y, w, h, color), export=False)
            parts = [(paper, "fallback")]
            if name in ("previous", "next"):
                cx, cy = x+w/2, y+h/2
                direction = -1 if name == "previous" else 1
                parts.append(m.polygon([(cx-direction*5,cy-9),(cx+direction*6,cy),
                                        (cx-direction*5,cy+9)], (54,50,42,255)))
            frames.append((label, parts))
        ident = m.next
        m.next += 1
        body = struct.pack("<HH", ident, len(frames))
        for index, (label, parts) in enumerate(frames):
            body += assets.tag(43, label.encode() + b"\0")
            if index:
                for depth in range(1, len(parts)+1):
                    body += assets.tag(28, struct.pack("<H", depth))
            for depth, part in enumerate(parts, 1):
                ident_part, child = part if isinstance(part, tuple) else (part, "")
                body += assets.tag(26, bytes([0x26 if child else 0x06]) + struct.pack("<HH", depth, ident_part) + b"\0" + (child.encode()+b"\0" if child else b""))
            body += assets.tag(1)
        m.tags.append(assets.tag(39, body + assets.tag(0)))
        return ident, name

    preview = m.sprite("preview", [], export=False)
    paper = m.sprite("fallback_paper", m.paper(320, 170, 640, 380), export=False)
    parts = [(paper, "fallback_paper"), (preview, "native_paper"), (preview, "native_template")]
    parts += [(m.field(344, 180, 592, 28, 21, align=2), "prompt"),
              (m.field(344, 210, 592, 24, 18, align=2), "part"),
              m.box(344, 238, 592, 1, (54,50,42,100)),
              m.field(344, 244, 272, 26, 19, "Current", align=2),
              m.field(664, 244, 272, 26, 19, "Incoming", align=2),
              (m.box(639, 248, 1, 224, (54,50,42,70)), "effect_divider"),
              (preview, "old_art"), (preview, "new_art"),
              (m.field(344, 305, 272, 26, 18, align=2), "old_art_note"),
              (m.field(664, 305, 272, 26, 18, align=2), "new_art_note"),
              (m.field(440, 484, 400, 22, 14, align=2), "page"),
              button("previous", 404, 479, 28, "", h=28),
              button("next", 848, 479, 28, "", h=28),
              button("no", 390, 512, 180, "Keep Existing"),
              button("yes", 710, 512, 180, "Replace"),
              (m.field(396, 515, 168, 25, 18, "Keep Existing", align=2), "no_caption"),
              (m.field(716, 515, 168, 25, 18, "Replace", align=2), "yes_caption")]
    # Separate lines give native stat icons a stable baseline beside each value.
    for side, x in (("cur",344), ("inc",664)):
        for row in range(4):
            parts.append((m.field(x, 368+row*26, 272, 26, 20), f"{side}{row}"))
    m.sprite("BBPMutationSelector", parts)
    root = ROOT / "build/data-mod/swfs"
    m.write(root / "better_breeding_plus.swf")
    (root / "swflist.gon.append").write_text("game [ better_breeding_plus.swf ]\n", encoding="ascii")
