#!/usr/bin/env python3
# Builds the DistantPlanet guide PDF:  python3 docs/make_guide.py out.pdf
# (needs Playwright + Chromium; uses panel-preview.png from the repo root)
import os, sys, base64, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "DistantPlanet-Guide.pdf")
img = base64.b64encode(open(os.path.join(ROOT, "panel-preview.png"), "rb").read()).decode()

html = f"""<!doctype html><html><head><meta charset="utf-8"><style>
@page {{ size: A4; margin: 16mm; }}
body {{ font-family: Inter, 'Helvetica Neue', Arial, sans-serif; color: #1d1d1f; font-size: 10.5pt; line-height: 1.45; }}
h1 {{ font-size: 26pt; margin: 0 0 2mm; letter-spacing: 0.02em; }}
h1 span {{ color: #3d7fb8; }}
h2 {{ font-size: 13.5pt; margin: 7mm 0 2mm; border-bottom: 1.5px solid #9ad0f5; padding-bottom: 1mm; }}
h3 {{ font-size: 11pt; margin: 4mm 0 1mm; }}
.lede {{ font-size: 11.5pt; color: #444; }}
.cols {{ display: flex; gap: 8mm; align-items: flex-start; }}
.cols img {{ width: 78mm; border-radius: 2mm; }}
table {{ border-collapse: collapse; width: 100%; font-size: 9.6pt; }}
td, th {{ border-bottom: 1px solid #e3e0d8; padding: 1.3mm 2mm; text-align: left; vertical-align: top; }}
th {{ background: #eef4fa; }}
td:first-child {{ font-weight: 700; white-space: nowrap; }}
.recipe {{ background: #f2f6fb; border-left: 3px solid #9ad0f5; padding: 2.5mm 4mm; margin: 3mm 0; page-break-inside: avoid; }}
.recipe b.t {{ display: block; font-size: 11pt; margin-bottom: 1mm; }}
.flow {{ font-weight: 700; background: #161b2c; color: #eee8dc; padding: 2.5mm 4mm; border-radius: 1.5mm; }}
.small {{ font-size: 9pt; color: #555; }}
.break {{ page-break-before: always; }}
</style></head><body>

<h1>Distant<span>Planet</span></h1>
<p class="lede">A Steiner-Parker style multimode filter for VCV Rack, in the spirit of the Tiptop Audio Forbidden Planet:
three audio inputs, one filter, one output.</p>

<div class="cols">
<img src="data:image/png;base64,{img}" style="width:44mm">
<div>
<h3>How it works</h3>
<p class="flow">LP IN + BP IN + HP IN &rarr; one 12 dB filter &rarr; VOLUME &rarr; OUT</p>
<p>Most multimode filters have one input and several outputs. This one works the other way round, like the
Steiner-Parker Synthacon filter it comes from. <b>Which input you patch sets the mode.</b> A sound into LP is
low-passed, into BP band-passed, and into HP high-passed.</p>
<p>All three inputs share the same filter, so you can use them together. Feed the <b>same</b> sound into two inputs and you get new shapes:
LP + HP gives a <b>notch</b>, and all three at low RESO gives a flat sound, with the filter only showing once you turn RESO up.
Feed <b>different</b> sounds in, such as a bass into LP and hats into HP, and they share one resonance and one sweep, so they move as one voice.</p>
<p>Turning FREQUENCY up removes more sound in the HP input and lets more through in the LP input, so an HP sweep feels backwards. That's normal.</p>
<p>Turn RESO up and it starts to <b>sing on its own from about 80 %</b>, as a sine at FREQUENCY. Past 90 % the sine
squares off, buzzes and wanders a little, which is the unstable top end of the hardware. Loud inputs tame the resonance,
so turn the source down if you want more ring.</p>
</div></div>

<h2>Controls and jacks</h2>
<table>
<tr><th>Control</th><th>What it does</th></tr>
<tr><td>FREQUENCY</td><td>Cutoff, 20 Hz to 20 kHz.</td></tr>
<tr><td>RESO</td><td>Resonance. About 80 %: self-oscillation starts. 90&ndash;100 %: saturated, buzzy and drifting.</td></tr>
<tr><td>VOLUME</td><td>Output level. Noon = unity, fully right = twice as loud (a soft limiter keeps it near &plusmn;10 V).</td></tr>
<tr><td>1V/OCT</td><td>Frequency CV at full strength, 1 V per octave, so the self-oscillation plays in tune from a sequencer.</td></tr>
<tr><td>CV + AMT</td><td>A second frequency CV through the AMT attenuator (fully right = 1 V/oct). For an LFO or envelope.</td></tr>
<tr><td>LP / BP / HP</td><td>Audio inputs into the low-pass, band-pass and high-pass parts of the filter. Use any combination.</td></tr>
<tr><td>OUT</td><td>The one output. Polyphonic: the number of channels follows the busiest input.</td></tr>
</table>
<p class="small">Stereo: merge L and R into one 2-channel cable (VCV Merge), patch it into an input, and split OUT. Each side gets its own filter.</p>

<h2 class="break">Recipes</h2>

<div class="recipe"><b class="t">1. Detroit stab with a moving notch</b>
Chord stab (Hapax chord into a poly saw) &rarr; a Mult &rarr; <b>LP and HP</b>. RESO 50 %, FREQUENCY noon.
Slow triangle LFO &rarr; CV, AMT 30 %. The notch sweeps through the chord like a phaser, but deeper and with only one notch.
<i>Variation:</i> add the same chord into BP too and push RESO to 70 %. Now it's flat except for a ringing peak that wanders.</div>

<div class="recipe"><b class="t">2. One filter, two parts</b>
Bass &rarr; LP, open hats or noise bursts &rarr; HP. FREQUENCY around 11 o'clock, RESO 60 %. Envelope from the bass &rarr; CV, AMT 50 %.
The hats open up when the bass closes, and both share one resonance, so they sound like one instrument.</div>

<div class="recipe"><b class="t">3. Sine voice from nothing</b>
Nothing patched into the inputs. RESO 82&ndash;85 %, VOLUME 2 o'clock. Sequencer pitch &rarr; 1V/OCT, then a VCA with an envelope after OUT.
Tune with FREQUENCY. <i>Variation:</i> trigger a short click into BP on each note, and the ringing picks up a pinged, woody attack.</div>

<div class="recipe"><b class="t">4. Unstable planet drone</b>
RESO at 95&ndash;100 %, very slow random CV (Marbles or Random8) &rarr; CV with AMT 20 %. Feed a quiet drum loop into HP:
the hits push the oscillation around and briefly choke it. Record a long take and keep the best minute.</div>

<div class="recipe"><b class="t">5. Acid line through BP</b>
Saw bass &rarr; BP, RESO 70 %, FREQUENCY 10 o'clock, accent envelope &rarr; CV at 70 %. BP keeps it thin and vocal,
so it sits above your kick. Try a quieter input level and listen to the squelch grow.</div>

<p class="small">DistantPlanet 2.0.0 &middot; Haer &middot; original code, not affiliated with Tiptop Audio. Plugin file: DistantPlanet-2.0.0-mac-arm64.vcvplugin. Drag it onto the Rack window, or copy it into Documents/Rack2/plugins-mac-arm64.</p>
</body></html>"""

tmp = out + ".html"
open(tmp, "w").write(html)
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage();
await p.goto('file://{tmp}'); await p.pdf({{path: '{out}', format: 'A4', printBackground: true}}); await b.close(); }})();"""
open(out + ".js", "w").write(js)
subprocess.run(["node", out + ".js"], check=True, cwd="/tmp")
os.remove(out + ".js"); os.remove(tmp)
print("wrote", out)
