#pragma once

// FroggersAppCore -- packet 3 (openspec/changes/desktop-v2-sheaf-runtime-
// harmonization, tasks.md 3.1-3.2). Satisfies the vendored
// synth::SynthApplicationCore concept (Config/Init/ProcessBlock; see
// desktop-v2/External/Sheaf/include/synth/AppConcepts.hpp) so
// synth::Engine<App> can drive it.
//
// Design D2 (design.md): this is a host-boundary change only. ProcessBlock
// delegates entirely to the EXISTING AudioEngine + FroggersV2ControlCore via
// FroggersV2AppCoreFacade -- the same facade.initialize()/facade.prepare()/
// facade.processHostedBlock() call path FroggersV2AppCoreFacade_test.cpp
// already exercises headlessly (AudioEngine constructed with pluginHosted =
// true, i.e. no JUCE AudioDeviceManager / device ownership here -- that
// remains MainComponent's job until the Runtime shell cutover, tasks.md
// section 10, retires it). No DSP is rewritten or added here.

#include "AudioEngine.h"
#include "control/FroggersV2AppCoreFacade.hpp"

#include "synth/AppContext.hpp"

class FroggersAppCore
{
public:
    FroggersAppCore();

    // synth::SynthApplicationCore: static Config() -> RuntimeConfig.
    static synth::RuntimeConfig Config();

    // synth::SynthApplicationCore: Init(AppContext*). Delegates to
    // FroggersV2AppCoreFacade::initialize(), which wires the sequencer state,
    // selects the default page, and does the same initial syncToHost() the
    // MainComponent construction path performs today (MainComponent.cpp:48).
    void Init(synth::AppContext* context);

    // Optional synth::HasPrepareToPlay hook: synth::Engine<App>::Prepare()
    // invokes this with the negotiated sample rate/block size. Delegates to
    // FroggersV2AppCoreFacade::prepare(), mirroring
    // FroggersV2AppCoreFacade_test.cpp's headless setup.
    void PrepareToPlay(double sampleRate, int blockSize);

    // synth::SynthApplicationCore: ProcessBlock(AudioBlock&). Adapts the
    // engine's non-owning per-channel AudioBlock view onto
    // FroggersV2AppCoreFacade::processHostedBlock's left/right pointer
    // signature -- the same one AudioEngine::processHostedBlock uses for its
    // existing pluginHosted (VST-style) callers. No new DSP: this is purely a
    // pointer/shape adaptation.
    void ProcessBlock(synth::AudioBlock& block);

    froggers_v2::FroggersV2AppCoreFacade& facade() { return m_facade; }
    const froggers_v2::FroggersV2AppCoreFacade& facade() const { return m_facade; }
    AudioEngine& audioEngine() { return m_audio; }
    const AudioEngine& audioEngine() const { return m_audio; }

private:
    AudioEngine m_audio;
    froggers_v2::FroggersV2AppCoreFacade m_facade;
    synth::AppContext* m_context = nullptr;
};
