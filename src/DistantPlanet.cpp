// DistantPlanet: a Steiner-Parker style multimode filter for VCV Rack.
// Three audio INPUTS (LP, BP, HP) feed one shared 2-pole filter with a single
// OUT. Which input you patch sets the mode; patch several at once (same sound
// or different sounds) and they are filtered together. The sound engine is in
// DistantPlanetDSP.hpp.
#include "plugin.hpp"
#include "DistantPlanetDSP.hpp"

// ---- LAYOUT (same numbers in res/make_panel.py) ----
static const float W_MM = 40.64f;
static const float FREQ_X = 20.32f, FREQ_Y = 28.0f;
static const float KNOB_X[2] = {10.5f, 30.14f};
static const float KNOB_Y = 52.0f;
static const float COL_X[3] = {8.0f, 20.32f, 32.64f};
static const float CV_Y = 72.0f;
static const float IN_Y = 94.0f;
static const float OUT_Y = 113.5f;

struct DistantPlanet : Module {
	enum ParamIds {
		FREQ_PARAM, RESO_PARAM, VOLUME_PARAM, CV_AMT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT, CV_INPUT,
		LP_INPUT, BP_INPUT, HP_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	distantplanet::Engine engine;
	distantplanet::Channel ch[16];

	DistantPlanet() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "Frequency", " Hz", 1024.f, distantplanet::FREQ_MIN);
		configParam(RESO_PARAM, 0.f, 1.f, 0.4f, "Resonance (sings on its own from about 80%, wild near 100%)", "%", 0.f, 100.f);
		configParam(VOLUME_PARAM, 0.f, 1.f, 0.5f, "Volume (noon = unity)", "%", 0.f, 200.f)->randomizeEnabled = false;
		configParam(CV_AMT_PARAM, 0.f, 1.f, 0.f, "CV amount", "%", 0.f, 100.f);

		configInput(VOCT_INPUT, "1V/oct (frequency CV at full strength)");
		configInput(CV_INPUT, "Frequency CV (through AMT, 1 V/oct at full)");
		configInput(LP_INPUT, "LP: heard through the low-pass");
		configInput(BP_INPUT, "BP: heard through the band-pass");
		configInput(HP_INPUT, "HP: heard through the high-pass");
		configOutput(OUT_OUTPUT, "Filter");
		configBypass(LP_INPUT, OUT_OUTPUT);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		engine.setSampleRate(e.sampleRate);
	}
	void onAdd(const AddEvent& e) override {
		engine.setSampleRate(APP->engine->getSampleRate());
	}
	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (auto& c : ch) c.reset();
	}

	void process(const ProcessArgs& args) override {
		int channels = 1;
		for (int i = 0; i < NUM_INPUTS; i++)
			channels = std::max(channels, inputs[i].getChannels());

		float freqK = params[FREQ_PARAM].getValue() * distantplanet::FREQ_OCTAVES;
		float amt = params[CV_AMT_PARAM].getValue();
		distantplanet::Controls k;
		k.reso = params[RESO_PARAM].getValue();
		k.volume = params[VOLUME_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			k.freqOct = freqK + inputs[VOCT_INPUT].getPolyVoltage(c) + amt * inputs[CV_INPUT].getPolyVoltage(c);
			engine.process(ch[c], k,
			               inputs[LP_INPUT].getPolyVoltage(c),
			               inputs[BP_INPUT].getPolyVoltage(c),
			               inputs[HP_INPUT].getPolyVoltage(c));
			outputs[OUT_OUTPUT].setVoltage(ch[c].out, c);
		}
		outputs[OUT_OUTPUT].setChannels(channels);
	}
};

struct DistantPlanetWidget : ModuleWidget {
	DistantPlanetWidget(DistantPlanet* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/DistantPlanet.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(FREQ_X, FREQ_Y)), module, DistantPlanet::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(KNOB_X[0], KNOB_Y)), module, DistantPlanet::RESO_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(KNOB_X[1], KNOB_Y)), module, DistantPlanet::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(COL_X[0], CV_Y)), module, DistantPlanet::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(COL_X[1], CV_Y)), module, DistantPlanet::CV_INPUT));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(COL_X[2], CV_Y)), module, DistantPlanet::CV_AMT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(COL_X[0], IN_Y)), module, DistantPlanet::LP_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(COL_X[1], IN_Y)), module, DistantPlanet::BP_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(COL_X[2], IN_Y)), module, DistantPlanet::HP_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(FREQ_X, OUT_Y)), module, DistantPlanet::OUT_OUTPUT));
	}
};

Model* modelDistantPlanet = createModel<DistantPlanet, DistantPlanetWidget>("DistantPlanet");
