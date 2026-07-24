#include "FroggersAppCore.hpp"

#include "HostAudioConfig.hpp"

FroggersAppCore::FroggersAppCore()
    // pluginHosted = true: no JUCE AudioDeviceManager / device ownership here
    // (see class doc comment). Matches FroggersV2AppCoreFacade_test.cpp's
    // headless AudioEngine(true) construction.
    : m_audio(true)
    , m_facade(m_audio)
{
}

synth::RuntimeConfig FroggersAppCore::Config()
{
    synth::RuntimeConfig config;
    config.appName = "FroggersTiga";
    // Stereo out matches the product's existing device setup
    // (AudioEngine::AudioEngine's non-hosted branch requests 0 in / 2 out,
    // AudioEngine.cpp:113-123). One input channel matches the existing
    // external ring-mod input path (AudioEngine::m_externalInputEnabled /
    // setExternalInputEnabled); wiring an actual device input is the Runtime
    // shell's job (tasks.md section 10), not this packet's.
    config.numAudioInputs = 1;
    config.numAudioOutputs = 2;
    config.preferredSampleRate = HostAudioConfig::kDefaultSampleRate;
    config.preferredBlockSize = 512;
    // Must match froggers_v2::FroggersAppSurface's root bounds
    // (Source/ui/FroggersAppSurface.hpp's kSurfaceBounds, 1280x920):
    // RuntimeMainComponent::ValidateApplicationTree throws on the very first
    // render if the application root's bounds don't equal
    // Config().uiWidth/uiHeight (RuntimeMainComponent.hpp:378-384). Defaults
    // (AppContext.hpp) are 900x560, which do not match -- see
    // FroggersApp_test.cpp's test_config_ui_bounds_match_surface_root for the
    // cross-check that keeps these from drifting apart again.
    config.uiWidth = 1280;
    config.uiHeight = 920;
    return config;
}

void FroggersAppCore::Init(synth::AppContext* context)
{
    m_context = context;
    m_facade.initialize();
}

void FroggersAppCore::PrepareToPlay(double sampleRate, int blockSize)
{
    m_facade.prepare(static_cast<float>(sampleRate), blockSize);
}

void FroggersAppCore::ProcessBlock(synth::AudioBlock& block)
{
    float* outputLeft =
        (block.outputs != nullptr && block.numOutputChannels >= 1) ? block.outputs[0] : nullptr;
    if (outputLeft == nullptr)
    {
        return;
    }
    float* outputRight =
        (block.outputs != nullptr && block.numOutputChannels >= 2) ? block.outputs[1] : nullptr;
    const float* inputChannel0 =
        (block.inputs != nullptr && block.numInputChannels >= 1) ? block.inputs[0] : nullptr;
    const int numInputChannels = inputChannel0 != nullptr ? block.numInputChannels : 0;

    m_facade.processHostedBlock(inputChannel0,
                                numInputChannels,
                                outputLeft,
                                outputRight,
                                block.numOutputChannels,
                                static_cast<int>(block.numFrames));
}
