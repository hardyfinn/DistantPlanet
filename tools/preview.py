#!/usr/bin/env python3
# Picture of the panel with Rack's own knob/jack graphics, so the layout can be checked without Rack.
#   python3 tools/preview.py out.png [path/to/Rack/res/ComponentLibrary]
import os, sys, math, re, base64, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "preview.png")
LIB = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "..", "..", "Rack-src", "res", "ComponentLibrary")
svg = open(os.path.join(HERE, "..", "res", "DistantPlanet.svg")).read()
L = {}
src = open(os.path.join(HERE, "..", "res", "make_panel.py")).read()
exec(src.split("# ---- LAYOUT")[1].split("CONTROL =")[0].split("\n", 1)[1], L)
W = 40.64
PX = 25.4 / 75.0
g = []

def comp(name, x, y, angle=0.0):
    p = os.path.join(LIB, name + ".svg")
    s = open(p).read()
    m = re.search(r'width="([0-9.]+)(px|mm)?"', s); n = re.search(r'height="([0-9.]+)(px|mm)?"', s)
    w = float(m.group(1)) * (1 if m.group(2) == "mm" else PX); h = float(n.group(1)) * (1 if n.group(2) == "mm" else PX)
    data = base64.b64encode(s.encode()).decode()
    g.append(f'<g transform="rotate({angle} {x} {y})"><image href="data:image/svg+xml;base64,{data}" x="{x - w/2}" y="{y - h/2}" width="{w}" height="{h}"/></g>')

def knob(base, x, y, v):
    if os.path.exists(os.path.join(LIB, base + "_bg.svg")):
        comp(base + "_bg", x, y)
    comp(base, x, y, -135 + 270 * v)

knob("RoundHugeBlackKnob", L["FREQ_X"], L["FREQ_Y"], 0.5)
knob("RoundBlackKnob", L["KNOB_X"][0], L["KNOB_Y"], 0.4)
knob("RoundBlackKnob", L["KNOB_X"][1], L["KNOB_Y"], 0.5)
comp("PJ301M", L["COL_X"][0], L["CV_Y"]); comp("PJ301M", L["COL_X"][1], L["CV_Y"])
knob("Trimpot", L["COL_X"][2], L["CV_Y"], 0.0)
for x in L["COL_X"]: comp("PJ301M", x, L["IN_Y"])
comp("PJ301M", L["FREQ_X"], L["OUT_Y"])
for x in (5.08, W - 2 * 5.08):
    for y in (0.0, 128.5 - 5.08):
        comp("ScrewSilver", x + 2.54, y + 2.54)

svg = svg.replace("</svg>", "\n".join(g) + "\n</svg>")
tmp = OUT + ".svg"; open(tmp, "w").write(svg)
pw, ph = 203, 643
open(tmp + ".html", "w").write(f'<html><body style="margin:0;background:#111"><img src="{os.path.basename(tmp)}" style="width:{pw}px;height:{ph}px"></body></html>')
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage({{viewport: {{width: {pw}, height: {ph}}}, deviceScaleFactor: 2}});
await p.goto('file://{tmp}.html'); await p.waitForTimeout(400);
await p.screenshot({{path: '{OUT}'}}); await b.close(); }})();"""
open(OUT + ".js", "w").write(js)
subprocess.run(["node", OUT + ".js"], check=True, cwd="/tmp")
os.remove(OUT + ".js"); os.remove(tmp); os.remove(tmp + ".html")
print("wrote", OUT)
