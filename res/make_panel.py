#!/usr/bin/env python3
# Draws res/DistantPlanet.svg. VCV Rack can't draw SVG <text>, so every label is
# turned into outline paths with fontTools. Positions are in mm and must match
# src/DistantPlanet.cpp (the numbers in the LAYOUT block are the same in both).
#   pip install fonttools
#   python3 res/make_panel.py
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_BOLD = os.environ.get("PANEL_FONT_BOLD", "/usr/share/fonts/opentype/inter/Inter-Bold.otf")
FONT_MED = os.environ.get("PANEL_FONT_MED", "/usr/share/fonts/opentype/inter/Inter-Medium.otf")

W, H = 40.64, 128.5  # 8 HP

BG = "#161B2C"       # deep space blue
BG_DARK = "#0E1220"
LINE = "#323A55"
INK = "#EEE8DC"
SOFT = "#A3AAC2"
LPC = "#F2A541"      # LP (amber)
BPC = "#8FE3B0"      # BP (pale green)
HPC = "#E8708A"      # HP (rose)
PLANET = "#9AD0F5"

# ---- LAYOUT (same numbers in src/DistantPlanet.cpp) ----
FREQ_X, FREQ_Y = 20.32, 28.0
KNOB_X = [10.5, 30.14]
KNOB_Y = 52.0
COL_X = [8.0, 20.32, 32.64]
CV_Y = 72.0
IN_Y = 94.0
OUT_Y = 113.5

CONTROL = 3.2   # control label size (project standard)
JACK = 2.9      # jack label size (project standard)

fonts = {}


def text(s, x, y, size, font=FONT_BOLD, fill=INK, anchor="middle", spacing=0.04):
    """SVG path for string s; (x, y) is the baseline point in mm."""
    if font not in fonts:
        f = TTFont(font)
        fonts[font] = (f, f.getGlyphSet(), f.getBestCmap(), f["head"].unitsPerEm)
    f, gs, cmap, upm = fonts[font]
    scale = size / upm
    track = spacing * upm
    names = [cmap[ord(c)] for c in s]
    width = sum(gs[n].width + track for n in names) - track
    x0 = {"middle": x - width * scale / 2, "end": x - width * scale}.get(anchor, x)
    pen = SVGPathPen(gs)
    cx = 0.0
    for n in names:
        gs[n].draw(TransformPen(pen, (scale, 0, 0, -scale, x0 + cx * scale, y)))
        cx += gs[n].width + track
    return f'<path fill="{fill}" d="{pen.getCommands()}"/>'


parts = []
add = parts.append
add(f'<rect width="{W}" height="{H}" fill="{BG}"/>')
# a few faint stars
for sx, sy, r in [(5.2, 19.0, 0.18), (35.6, 17.5, 0.22), (3.6, 40.0, 0.15), (37.4, 41.0, 0.16),
                  (20.3, 41.6, 0.14), (4.4, 104.5, 0.17), (36.3, 106.0, 0.2), (12.0, 121.0, 0.14), (29.0, 120.0, 0.15)]:
    add(f'<circle cx="{sx}" cy="{sy}" r="{r}" fill="{INK}" fill-opacity="0.55"/>')

# title + small ringed planet
add(text("DistantPlanet", 18.6, 10.6, 3.9, FONT_BOLD, INK, spacing=0.02))
px, py = 36.0, 9.2
add(f'<circle cx="{px}" cy="{py}" r="1.5" fill="{PLANET}"/>')
add(f'<ellipse cx="{px}" cy="{py}" rx="2.9" ry="0.75" transform="rotate(-18 {px} {py})" fill="none" stroke="{INK}" stroke-width="0.35"/>')

# FREQUENCY
add(text("FREQUENCY", FREQ_X, FREQ_Y - 11.4, CONTROL))
add(text("20", FREQ_X - 10.4, FREQ_Y + 10.2, 2.3, FONT_MED, SOFT))
add(text("20k", FREQ_X + 10.4, FREQ_Y + 10.2, 2.3, FONT_MED, SOFT))

# RESO / VOLUME
add(text("RESO", KNOB_X[0], KNOB_Y - 7.2, CONTROL))
add(text("VOLUME", KNOB_X[1], KNOB_Y - 7.2, CONTROL))

# CV row
add(f'<rect x="2.2" y="{CV_Y - 10.2}" width="{W - 4.4}" height="16.4" rx="1.4" fill="{BG_DARK}"/>')
add(text("1V/OCT", COL_X[0], CV_Y - 5.6, JACK, FONT_BOLD, SOFT))
add(text("CV", COL_X[1], CV_Y - 5.6, JACK, FONT_BOLD, SOFT))
add(text("AMT", COL_X[2], CV_Y - 5.6, CONTROL, FONT_BOLD, SOFT))
add(f'<path d="M{COL_X[1] + 4.0} {CV_Y} L{COL_X[2] - 4.4} {CV_Y}" stroke="{LINE}" stroke-width="0.4"/>')

# audio inputs
add(text("INPUTS", W / 2, IN_Y - 11.0, 2.3, FONT_MED, SOFT, spacing=0.18))
for x, n, c in zip(COL_X, ["LP", "BP", "HP"], [LPC, BPC, HPC]):
    add(f'<circle cx="{x}" cy="{IN_Y}" r="4.6" fill="{c}" fill-opacity="0.16"/>')
    add(text(n, x, IN_Y - 5.8, JACK, FONT_BOLD, c))
# little arrows from the three inputs down to OUT
for x in COL_X:
    if abs(x - FREQ_X) > 1:
        add(f'<path d="M{x} {IN_Y + 5.0} Q{x} {OUT_Y - 3.0} {FREQ_X + (4.6 if x > FREQ_X else -4.6)} {OUT_Y - 1.0}" stroke="{LINE}" stroke-width="0.4" fill="none"/>')
    else:
        add(f'<path d="M{x} {IN_Y + 5.0} L{x} {OUT_Y - 10.4}" stroke="{LINE}" stroke-width="0.4"/>')

# output
add(f'<rect x="{FREQ_X - 6.5}" y="{OUT_Y - 9.2}" width="13.0" height="14.6" rx="1.4" fill="{PLANET}" fill-opacity="0.16"/>')
add(text("OUT", FREQ_X, OUT_Y - 5.6, JACK, FONT_BOLD, PLANET))

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}mm" height="{H}mm" viewBox="0 0 {W} {H}">\n'
       + "\n".join(p for p in parts if p) + "\n</svg>\n")
open(os.path.join(HERE, "DistantPlanet.svg"), "w").write(svg)
print("ok")

# --- Haer maker logo (res/haer_logo.py) on the bottom rail
import sys as _sys
_sys.path.insert(0, HERE)
import haer_logo
haer_logo.add_to_file(os.path.join(HERE, "DistantPlanet.svg"), x=W / 2, y=126.0, cap=3.0, bg=BG)
