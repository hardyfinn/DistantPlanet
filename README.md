# DistantPlanet

A Steiner-Parker style multimode filter for VCV Rack, in the spirit of the
Tiptop Audio Forbidden Planet. Part of the Haer plugins. 8 HP.

The filter has **three audio inputs and one output**. LP, BP and HP are
different ways into the same 12 dB, 2-pole filter. Patching one picks the
mode. Patch two or three at once (the same sound or different sounds) and
they are filtered together. One sound into LP and HP gives a notch, and all
three at low RESO gives a flat response.

Controls: FREQUENCY (20 Hz to 20 kHz), RESO (self-oscillates as a sine from
about 80 %, gets wild and unstable near 100 %), VOLUME (noon = unity),
1V/OCT input (full-strength frequency CV, calibrated so the self-oscillation
plays in tune), CV input with AMT attenuator. Loud inputs tame the resonance,
as on the hardware. Everything is polyphonic. For stereo, merge L and R into a
2-channel cable.

Build: `make dist RACK_DIR=path/to/Rack-SDK`. GitHub Actions builds the Mac
(Apple Silicon) plugin on every push. Engine checks without Rack: `make test`.
Panel: `python3 res/make_panel.py`; preview: `python3 tools/preview.py out.png`;
guide: `python3 docs/make_guide.py out.pdf`.

Original code (GPL-3.0-or-later). Not affiliated with Tiptop Audio.
