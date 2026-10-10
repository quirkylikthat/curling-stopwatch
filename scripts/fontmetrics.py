#!/usr/bin/env python3
"""Print the ink extents of the M5GFX fonts the timing screen uses.

The vertical layout in src/ui/Theme.h / src/ui/Layout.h is computed from these
numbers (ink = pixels actually lit, not the font's line box). Re-run this if a
font, a text size, or the M5GFX version changes:

    python3 scripts/fontmetrics.py .pio/libdeps/stopwatch/M5GFX/src/lgfx/Fonts

"height"/"baseline" are LovyanGFX's metrics (what text datums align to);
"ink_*_rel_baseline" are relative to the baseline (negative = above).
"""
import re
import sys
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".pio/libdeps/stopwatch/M5GFX/src/lgfx/Fonts")


def source(path):
    return re.sub(r"//.*", "", path.read_text())  # drop comments (they contain braces)


def array(text, name):
    m = re.search(re.escape(name) + r"\s*\[[^\]]*\]\s*(?:PROGMEM\s*)?=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise SystemExit(f"array {name} not found")
    return m.group(1)


def gfx_font(fname, chars):
    text = source(root / "GFXFF" / fname)
    glyphs = [tuple(int(v, 0) for v in g.split(","))
              for g in re.findall(r"\{\s*([^{}]+?)\s*\}", array(text, fname[:-2] + "Glyphs"))]
    tail = re.search(r"\(GFXglyph\s*\*\)\s*\w+,\s*(\w+),\s*\w+,\s*(\d+)", text)
    first, y_advance = int(tail.group(1), 0), int(tail.group(2))
    above = max(-g[5] for g in glyphs)        # LovyanGFX: max ascent over ALL glyphs
    below = max(g[2] + g[5] for g in glyphs)  # ... and max descent
    sel = [glyphs[ord(c) - first] for c in chars]
    return dict(height=above + below, baseline=above, y_advance=y_advance,
                ink_top_rel_baseline=min(g[5] for g in sel),
                ink_bottom_rel_baseline=max(g[5] + g[2] for g in sel))


def font7(chars):
    text = source(root / "Font7srle.h")
    widths = [int(v, 0) for v in array(text, "widtbl_f7s").split(",") if v.strip()]
    rows = set()
    for c in chars:
        code = ord(c)
        data = [int(v, 0) for v in array(text, f"chr_f7s_{code:02X}").split(",") if v.strip()]
        pixels = []
        for b in data:  # 8-bit RLE: bit7 = ink, low 7 bits = run length - 1
            pixels += [1 if b & 0x80 else 0] * ((b & 0x7F) + 1)
        rows |= {i // widths[code - 32] for i, v in enumerate(pixels) if v}
    return dict(height=48, ink_top_row=min(rows), ink_bottom_row=max(rows))


print("Font7 (big number) digits:      ", font7("0123456789."))
print("FreeSansBold24pt7b (band) caps: ", gfx_font("FreeSansBold24pt7b.h", "PEELTAKOUBRDHCWG"))
print("FreeSansBold12pt7b (secondary): ", gfx_font("FreeSansBold12pt7b.h", "SPLITHOG-0123456789.DRAWKEUBC"))
print("FreeSans12pt7b (history) digits:", gfx_font("FreeSans12pt7b.h", "0123456789."))
