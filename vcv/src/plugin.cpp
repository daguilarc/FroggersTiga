// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 JoYoFresh and Diego Aguilar-Canabal
//
// VCV Rack 2 plugin — GPL wrapper. MIT DSP: ../src/core/

#include <rack.hpp>

#include <algorithm>
#include <cmath>

#include "CvMidiBridge.hpp"
#include "PagedHostIO.hpp"

struct FroggersTigaModule : rack::engine::Module
{
    PagedHostIO host;
    CvMidiBridge midiBridge;

    enum ParamIds
    {
        PAGE_PARAM,
        PARAM_COUNT
    };
    enum InputIds
    {
        AUDIO_INPUT,
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        CV4_INPUT,
        GATE_INPUT,
        MIDI_INPUT,
        INPUT_COUNT
    };
    enum OutputIds
    {
        AUDIO_OUTPUT,
        CV_OUT1,
        CV_OUT2,
        MIDI_OUTPUT,
        OUTPUT_COUNT
    };
    enum LightIds
    {
        LIGHT_COUNT
    };

    FroggersTigaModule()
    {
        config(PARAM_COUNT, INPUT_COUNT, OUTPUT_COUNT, LIGHT_COUNT);
        configParam(PAGE_PARAM, 0.f, 4.f, 0.f, "Page", " Page");
        configInput(AUDIO_INPUT, "audio");
        configInput(CV1_INPUT, "cv1");
        configInput(CV2_INPUT, "cv2");
        configInput(CV3_INPUT, "cv3");
        configInput(CV4_INPUT, "cv4");
        configInput(GATE_INPUT, "gate");
        configInput(MIDI_INPUT, "midi");
        configOutput(AUDIO_OUTPUT, "audio");
        configOutput(CV_OUT1, "cv1");
        configOutput(CV_OUT2, "cv2");
        configOutput(MIDI_OUTPUT, "midi");
        host.Init();
        host.SetSampleRate(APP->engine->getSampleRate());
    }

    void onReset(const ResetEvent& e) override
    {
        Module::onReset(e);
        host.Init();
        host.SetSampleRate(APP->engine->getSampleRate());
    }

    void onSampleRateChange(const SampleRateChangeEvent& e) override
    {
        host.SetSampleRate(e.sampleRate);
    }

    void syncPageParam()
    {
        const int page = static_cast<int>(std::round(params[PAGE_PARAM].getValue()));
        if (page != static_cast<int>(host.GetCurrentPage()))
        {
            while (static_cast<int>(host.GetCurrentPage()) < page)
            {
                host.PulsePageNext();
            }
            while (static_cast<int>(host.GetCurrentPage()) > page)
            {
                host.PulsePagePrevious();
            }
        }
    }

    void drainVcvMidiIn(const ProcessArgs& processArgs)
    {
        midi::Input input(inputs[MIDI_INPUT], processArgs.frame);
        float* mods = host.m_pageManager.m_modMgr.m_mods;
        for (midi::Message msg : input)
        {
            if (!msg.isController())
            {
                continue;
            }
            const uint8_t cc = static_cast<uint8_t>(msg.getController());
            const float value = static_cast<float>(msg.getValue()) / 127.0f;
            if (cc >= 1 && cc <= 4)
            {
                mods[cc - 1] = value;
            }
        }
    }

    void tickMidiOut()
    {
        midi::Output output(outputs[MIDI_OUTPUT]);
        midiBridge.tickMidiOut(host.m_engine.GetEnvelopeLevel(),
                               [&](uint8_t channel, uint8_t cc, uint8_t value) {
                                   output.send(midi::Message::controller(channel + 1, cc, value));
                               });
    }

    void process(const ProcessArgs& processArgs) override
    {
        syncPageParam();

        host.SetCv(0, inputs[CV1_INPUT].getVoltage() / 10.f);
        host.SetCv(1, inputs[CV2_INPUT].getVoltage() / 10.f);
        host.SetCv(2, inputs[CV3_INPUT].getVoltage() / 10.f);
        host.SetCv(3, inputs[CV4_INPUT].getVoltage() / 10.f);
        host.SetGate(inputs[GATE_INPUT].getVoltage() > 1.f);
        drainVcvMidiIn(processArgs);

        const float extIn = inputs[AUDIO_INPUT].isConnected()
                                ? inputs[AUDIO_INPUT].getVoltage() / 10.f
                                : 0.f;
        float out = 0.f;
        host.ProcessBlock(&extIn, &out, 1);

        outputs[AUDIO_OUTPUT].setVoltage(std::clamp(out, -1.f, 1.f) * 5.f);
        outputs[CV_OUT1].setVoltage(host.GetCvOut(4) * 10.f);
        outputs[CV_OUT2].setVoltage(host.GetCvOut(5) * 10.f);
        tickMidiOut();
    }
};

struct FroggersTigaModuleWidget : rack::engine::ModuleWidget
{
    FroggersTigaModuleWidget(FroggersTigaModule* module)
    {
        setModule(module);
        box.size = rack::math::Vec(12 * RACK_GRID_WIDTH, 16 * RACK_GRID_WIDTH);

        addParam(createParamCentered<rack::RoundBlackKnob>(
            rack::math::Vec(box.size.x * 0.5f, 3.f * RACK_GRID_WIDTH),
            module,
            FroggersTigaModule::PAGE_PARAM));

        const float jackY = 9.f * RACK_GRID_WIDTH;
        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(1.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::AUDIO_INPUT));
        addOutput(createOutputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(3.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::AUDIO_OUTPUT));

        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(5.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::CV1_INPUT));
        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(7.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::CV2_INPUT));
        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(9.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::CV3_INPUT));
        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(11.5f * RACK_GRID_WIDTH, jackY), module, FroggersTigaModule::CV4_INPUT));

        const float jackY2 = 12.f * RACK_GRID_WIDTH;
        addInput(createInputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(2.f * RACK_GRID_WIDTH, jackY2), module, FroggersTigaModule::GATE_INPUT));
        addInput(createInputCentered<rack::midi::Port>(rack::math::Vec(4.5f * RACK_GRID_WIDTH, jackY2),
                                                       module,
                                                       FroggersTigaModule::MIDI_INPUT));
        addOutput(createOutputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(7.f * RACK_GRID_WIDTH, jackY2), module, FroggersTigaModule::CV_OUT1));
        addOutput(createOutputCentered<rack::ThemedPJ301MPort>(
            rack::math::Vec(9.f * RACK_GRID_WIDTH, jackY2), module, FroggersTigaModule::CV_OUT2));
        addOutput(createOutputCentered<rack::midi::Port>(rack::math::Vec(11.f * RACK_GRID_WIDTH, jackY2),
                                                         module,
                                                         FroggersTigaModule::MIDI_OUTPUT));

        addChild(createWidget<ScrewSilver>(rack::math::Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(rack::math::Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(rack::math::Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(
            rack::math::Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }
};

Model* modelFroggersTiga =
    createModel<FroggersTigaModule, FroggersTigaModuleWidget>("FroggersTiga");

Plugin* plugin;

void init(Plugin* p)
{
    plugin = p;
    p->addModel(modelFroggersTiga);
}
