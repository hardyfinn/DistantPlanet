// DistantPlanet sound engine (no Rack dependency, so test/render.cpp can use it).
//
// A Steiner-Parker style 2-pole filter with three audio INPUTS and one output.
// Each input enters the same filter network at a different point:
//   LP IN  -> heard through the low-pass response
//   BP IN  -> heard through the band-pass response
//   HP IN  -> heard through the high-pass response
// and the three are summed inside the filter, so one sound into LP + HP gives a
// notch, into all three gives a flat (all-pass-like) response at low RESO.
//
// The maths: one filter with transfer function
//   OUT = (w^2 * LP + 2*w*s * BP + s^2 * HP) / (s^2 + k*w*s + w^2)
// built in "observer" form, so all three inputs share the same two integrators
// (and the same resonance and saturation, which is what makes it sound like one
// filter rather than three mixed together). Integrated with trapezoidal (TPT)
// steps at 2x the Rack sample rate.
//
// RESO: the damping k is 2 - r * sat(y)/y, where sat() is a slightly lopsided
// tanh. Small signals see k = 2 - r; when r passes 2 the damping goes negative
// and the filter rings up on its own until the saturation holds it, giving a
// sine at FREQUENCY. Loud inputs push sat() into its flat part, which raises the
// damping, so hot inputs tame the resonance (as on the hardware). Near the top
// of RESO the saturation is driven hard: the sine squares off, buzzes and wanders.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace distantplanet {

static const float PI_F = 3.14159265358979f;
static const float FREQ_MIN = 20.f;      // FREQUENCY knob fully left
static const float FREQ_OCTAVES = 10.f;  // 20 Hz .. 20.48 kHz
static const float RESO_MAX = 2.75f;     // r at RESO fully right (self-osc from r = 2)
static const float IN_SCALE = 0.2f;      // 5 V in = 1.0 inside the filter
static const float OUT_SCALE = 5.f;

struct Controls {
	float freqOct = 5.f;  // octaves above FREQ_MIN (knob + CV), already summed
	float reso = 0.f;     // 0..1 (knob)
	float volume = 0.5f;  // 0..1, 0.5 = unity
};

struct Channel {
	float s1 = 0.f, s2 = 0.f;  // integrator states
	float yPrev = 0.f;         // last output (for the saturation estimate)
	float out = 0.f;           // volts
	uint32_t rng = 22222u;
	float drift = 0.f;         // slow random wander used near full RESO
	void reset() { s1 = s2 = yPrev = out = drift = 0.f; }
};

struct Engine {
	float sr = 48000.f;
	void setSampleRate(float s) { sr = s; }

	// Slightly asymmetric soft limiter (diode pair mismatch): adds a touch of
	// even harmonics to the resonance.
	static float sat(float y) {
		const float b = 0.12f;
		return std::tanh(y + b) - std::tanh(b);
	}

	static float nextNoise(uint32_t& s) {
		s ^= s << 13; s ^= s >> 17; s ^= s << 5;
		return (s & 0xFFFFFF) / 8388608.f - 1.f;
	}

	// One Rack sample. lp/bp/hp are the input voltages.
	void process(Channel& c, const Controls& k, float lp, float bp, float hp) {
		float reso = std::min(std::max(k.reso, 0.f), 1.f);
		// r: 0 at RESO 0, 2.0 (self-osc threshold) at RESO 0.8, RESO_MAX at 1.
		float r = (reso <= 0.8f)
			? 2.0f * std::pow(reso / 0.8f, 1.3f)
			: 2.0f + (RESO_MAX - 2.0f) * (reso - 0.8f) / 0.2f;
		float wild = std::max(0.f, (reso - 0.9f) / 0.1f);  // 0..1 in the top 10 %

		// Top of RESO: the pitch wanders a little (unstable, as on the hardware).
		float n = nextNoise(c.rng);
		c.drift += (n * 0.6f - c.drift) * 0.0004f;
		float oct = k.freqOct + wild * c.drift * 0.35f;

		float os = 2.f * sr;
		float fc = FREQ_MIN * std::exp2(oct);
		fc = std::min(std::max(fc, 5.f), 0.45f * os);
		float g = std::tan(PI_F * fc / os);

		float xl = lp * IN_SCALE, xb = 2.f * bp * IN_SCALE, xh = hp * IN_SCALE;
		// A whisper of noise so self-oscillation starts with nothing patched.
		float tiny = n * 1e-4f;
		xl += tiny;

		float acc = 0.f;
		for (int i = 0; i < 2; i++) {
			// Saturation estimate from the last output (secant of sat at yPrev).
			float yp = c.yPrev;
			float ratio = (std::fabs(yp) > 1e-4f) ? sat(yp) / yp : 1.f / (std::cosh(0.12f) * std::cosh(0.12f));
			float kd = 2.f - r * ratio;
			float den = 1.f + g * kd + g * g;
			float y = (xh + g * xb + g * g * xl + g * c.s2 + c.s1) / den;
			float w = g * (xl - y) + c.s2;
			float z1 = y - xh;
			c.s2 = w + g * (xl - y);
			c.s1 = z1 + g * (w - kd * y + xb);
			// keep the states sane if something extreme is patched in
			c.s1 = std::min(std::max(c.s1, -8.f), 8.f);
			c.s2 = std::min(std::max(c.s2, -8.f), 8.f);
			c.yPrev = y;
			acc += y;
		}
		float y = acc * 0.5f;
		if (!std::isfinite(y)) { c.reset(); y = 0.f; }

		float vol = k.volume * 2.f;
		float v = y * OUT_SCALE * vol;
		// gentle output limiter at about +/-10 V
		float a = std::fabs(v);
		if (a > 8.f) v = (v < 0 ? -1.f : 1.f) * (8.f + 3.f * std::tanh((a - 8.f) / 3.f));
		c.out = v;
	}

	// Small-signal magnitude for one input path at frequency f (for tests/docs).
	// path: 0 = LP, 1 = BP, 2 = HP.
	static float response(int path, float f, float fc, float reso) {
		float r = (reso <= 0.8f) ? 2.0f * std::pow(reso / 0.8f, 1.3f)
		                         : 2.0f + (RESO_MAX - 2.0f) * (reso - 0.8f) / 0.2f;
		float kd = std::max(0.02f, 2.f - r);
		float x = f / fc;
		float reD = 1.f - x * x, imD = kd * x;
		float num = (path == 0) ? 1.f : (path == 1) ? 2.f * x : x * x;
		return num / std::sqrt(reD * reD + imD * imD);
	}
};

}  // namespace distantplanet
