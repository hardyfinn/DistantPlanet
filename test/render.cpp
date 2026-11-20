// Offline checks for the DistantPlanet engine (no Rack needed).
//   make test     (or: c++ -O2 -std=c++17 test/render.cpp -o /tmp/dp && /tmp/dp out_dir)
// Prints a pass/fail line per check and writes a demo WAV into out_dir.
#include "../src/DistantPlanetDSP.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

using namespace distantplanet;
static const float SR = 48000.f;
static const double PI = 3.14159265358979;
static int failures = 0;

static void check(bool ok, const char* what, float v) {
	printf("%s  %-58s (%.3f)\n", ok ? "PASS" : "FAIL", what, v);
	if (!ok) failures++;
}

static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) return;
	uint32_t n = l.size(), dataBytes = n * 4, sr = (uint32_t) SR, br = sr * 4;
	uint16_t ch = 2, bits = 16, align = 4, fmt = 1;
	uint32_t riff = 36 + dataBytes, sixteen = 16;
	fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
	fwrite(&sixteen, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
	fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
	fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
	for (uint32_t i = 0; i < n; i++) {
		int16_t a = (int16_t) std::max(-32767.f, std::min(32767.f, l[i] / 10.f * 32767.f));
		int16_t b = (int16_t) std::max(-32767.f, std::min(32767.f, r[i] / 10.f * 32767.f));
		fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f);
	}
	fclose(f);
}

static float octOf(float hz) { return std::log2(hz / FREQ_MIN); }

// Gain for a sine of `amp` volts at hz fed to the inputs selected by mask
// (1 = LP, 2 = BP, 4 = HP), cutoff fc.
static float gain(int mask, float hz, float fc, float reso, float amp = 1.f) {
	Engine e; e.setSampleRate(SR);
	Channel c;
	Controls k; k.freqOct = octOf(fc); k.reso = reso; k.volume = 0.5f;
	double acc = 0, ref = 0;
	for (int i = 0; i < 48000; i++) {
		float x = amp * std::sin(2 * PI * hz * i / SR);
		e.process(c, k, (mask & 1) ? x : 0.f, (mask & 2) ? x : 0.f, (mask & 4) ? x : 0.f);
		if (i >= 24000) { acc += c.out * c.out; ref += x * x; }
	}
	return std::sqrt(acc / ref);
}

static float strongest(const std::vector<float>& x, float lo, float hi) {
	float best = 0, bestHz = 0;
	for (float hz = lo; hz <= hi; hz *= 1.002f) {
		double re = 0, im = 0;
		for (size_t i = 0; i < x.size(); i++) {
			re += x[i] * std::cos(2 * PI * hz * i / SR);
			im += x[i] * std::sin(2 * PI * hz * i / SR);
		}
		float m = re * re + im * im;
		if (m > best) { best = m; bestHz = hz; }
	}
	return bestHz;
}

// Self-oscillation with nothing patched: returns pitch, fills peak volts.
static float selfOsc(float fc, float reso, float& peak, bool& finite) {
	Engine e; e.setSampleRate(SR);
	Channel c;
	Controls k; k.freqOct = octOf(fc); k.reso = reso;
	std::vector<float> tail;
	peak = 0; finite = true;
	for (int i = 0; i < 96000; i++) {
		e.process(c, k, 0, 0, 0);
		if (!std::isfinite(c.out)) finite = false;
		if (i >= 96000 - 9600) { tail.push_back(c.out); peak = std::max(peak, std::fabs(c.out)); }
	}
	return strongest(tail, fc * 0.7f, fc * 1.4f);
}

int main(int argc, char** argv) {
	std::string dir = argc > 1 ? argv[1] : ".";

	check(std::fabs(gain(1, 60, 2000, 0) - 1) < 0.05f, "LP in passes lows at unity", gain(1, 60, 2000, 0));
	check(gain(1, 12000, 500, 0) < 0.01f, "LP in removes highs (12 kHz, fc 500)", gain(1, 12000, 500, 0));
	check(std::fabs(gain(4, 15000, 300, 0) - 1) < 0.08f, "HP in passes highs at unity", gain(4, 15000, 300, 0));
	check(gain(4, 40, 2000, 0) < 0.01f, "HP in removes lows (40 Hz, fc 2 kHz)", gain(4, 40, 2000, 0));
	check(std::fabs(gain(2, 1000, 1000, 0) - 1) < 0.08f, "BP in: unity at FREQUENCY", gain(2, 1000, 1000, 0));
	check(gain(2, 50, 2000, 0) < 0.06f, "BP in: quiet far below", gain(2, 50, 2000, 0));
	check(gain(5, 1000, 1000, 0.3f) < 0.08f, "LP + HP same sound: notch at FREQUENCY", gain(5, 1000, 1000, 0.3f));
	float f1 = gain(7, 100, 1000, 0), f2 = gain(7, 1000, 1000, 0), f3 = gain(7, 8000, 1000, 0);
	check(std::fabs(f1 - 1) < 0.08f && std::fabs(f2 - 1) < 0.08f && std::fabs(f3 - 1) < 0.08f,
	      "LP + BP + HP same sound at RESO 0: flat", f2);
	float quiet = gain(1, 1000, 1000, 0.75f, 0.2f), hot = gain(1, 1000, 1000, 0.75f, 8.f);
	check(quiet > 2.5f, "RESO 75 %: strong peak for a quiet input", quiet);
	check(hot < quiet * 0.6f, "hot input tames the resonance", hot);

	float pk; bool fin;
	float p440 = selfOsc(440, 0.85f, pk, fin);
	check(std::fabs(1200 * std::log2(p440 / 440)) < 40 && fin, "self-oscillates at FREQUENCY (440 Hz, RESO 85 %)", p440);
	check(pk > 1.5f && pk < 9.f, "self-oscillation level sensible (peak volts)", pk);
	float p880 = selfOsc(880, 0.85f, pk, fin);
	check(std::fabs(1200 * std::log2(p880 / p440) - 1200) < 25, "1 V/oct tracking (cents error from 1 oct)",
	      1200 * std::log2(p880 / p440) - 1200);
	float p110 = selfOsc(110, 0.85f, pk, fin);
	check(std::fabs(1200 * std::log2(p440 / p110) - 2400) < 30, "2 octaves down tracking (cents error)",
	      1200 * std::log2(p440 / p110) - 2400);
	selfOsc(2000, 1.0f, pk, fin);
	check(fin && pk < 11.f, "RESO 100 %: wild but bounded", pk);
	{
		Engine e; e.setSampleRate(SR); Channel c; Controls k; k.reso = 1.f; bool ok = true;
		for (int i = 0; i < 200000; i++) {
			k.freqOct = 10.f * (0.5f + 0.5f * std::sin(i * 0.0007f));
			float x = 10.f * std::sin(i * 0.37f);
			e.process(c, k, x, -x, x);
			if (!std::isfinite(c.out) || std::fabs(c.out) > 11.f) ok = false;
		}
		check(ok, "stress: full RESO, 10 V inputs, sweeping FREQUENCY", 0);
	}

	// ---- demo: 4 short scenes, ~16 s ----
	std::vector<float> L, R;
	Engine e; e.setSampleRate(SR);
	Channel cl, cr; cr.rng = 777u;
	float bpm = 128, step = 60.f / bpm / 4.f;
	int stepN = (int) (step * SR);
	const int seq[16] = {0, 0, 12, 0, 3, 0, 15, 0, 0, 7, 0, 10, 0, 0, 12, 3};
	double ph = 0;
	uint32_t nz = 1u;
	auto noise = [&]() { nz ^= nz << 13; nz ^= nz >> 17; nz ^= nz << 5; return (nz & 0xFFFF) / 32768.f - 1.f; };
	int total = (int) (16 * 4 * stepN);
	for (int i = 0; i < total; i++) {
		int s = i / stepN, scene = s / 16, pos = i % stepN;
		float env = std::exp(-pos / (0.06f * SR));
		float note = 41.2f * std::exp2(seq[s % 16] / 12.f);  // E1 bass line
		ph += note / SR; ph -= std::floor(ph);
		float saw = 5.f * (2.f * (float) ph - 1.f);
		Controls k; k.volume = 0.5f;
		float l = 0, r = 0;
		if (scene == 0) {        // LP in: acid-ish bass, cutoff opens over the bar
			k.freqOct = octOf(120) + 4.0f * env + 2.0f * (s % 16) / 16.f; k.reso = 0.72f;
			e.process(cl, k, saw, 0, 0); l = r = cl.out;
		} else if (scene == 1) { // same line into HP + LP: notch sweep
			k.freqOct = octOf(200) + 3.5f * (0.5f - 0.5f * std::cos(2 * PI * i / (16.f * stepN))); k.reso = 0.55f;
			e.process(cl, k, saw, 0, saw); l = r = cl.out;
		} else if (scene == 2) { // noise hats into BP, saw into LP: two sounds, one filter
			float hat = (s % 2 == 1) ? noise() * 4.f * std::exp(-pos / (0.02f * SR)) : 0.f;
			k.freqOct = octOf(900) + 2.0f * env; k.reso = 0.78f;
			e.process(cl, k, saw * 0.8f, hat, 0); l = r = cl.out;
		} else {                 // self-oscillating sine voice, 1 V/oct melody, stereo pair
			const int mel[8] = {0, 7, 12, 10, 3, 15, 7, 5};
			k.freqOct = octOf(220) + mel[(s / 2) % 8] / 12.f; k.reso = 0.84f;
			Controls k2 = k; k2.freqOct += 0.004f;
			e.process(cl, k, 0, 0, 0); e.process(cr, k2, 0, 0, 0);
			float ve = std::exp(-pos / (0.12f * SR));
			l = cl.out * ve; r = cr.out * ve;
		}
		L.push_back(l); R.push_back(r);
	}
	writeWav(dir + "/distantplanet-demo.wav", L, R);
	printf("%s (%d failed)\n", failures ? "SOME CHECKS FAILED" : "all checks passed", failures);
	return failures ? 1 : 0;
}
