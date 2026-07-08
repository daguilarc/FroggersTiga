#include "runtime/DesktopV2AudioStateProjection.hpp"

#include "AudioEngine.h"
#include "HostAudioConfig.hpp"

namespace
{
juce::String standaloneExternalInputState(const AudioEngine& engine)
{
    if (!engine.isAudioRunning())
    {
        return "unavailable";
    }
    if (!engine.isExternalInputEnabled())
    {
        return "muted";
    }
    switch (engine.getInputRouteStatus())
    {
        case InputRouteStatus::Ok:
            return "active";
        case InputRouteStatus::NoInputChannels:
            return "unavailable";
        case InputRouteStatus::SilentCapture:
            return "active";
        case InputRouteStatus::Idle:
        default:
            return "unavailable";
    }
}

juce::String standaloneOutputState(const AudioEngine& engine)
{
    if (!engine.isAudioRunning())
    {
        return "unavailable";
    }
    return "active";
}
} // namespace

DesktopV2AudioStateProjection buildStandaloneAudioStateProjection(const AudioEngine& engine)
{
    DesktopV2AudioStateProjection projection;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    engine.getDeviceManager().getAudioDeviceSetup(setup);

    projection.outputDeviceName = setup.outputDeviceName.isNotEmpty() ? setup.outputDeviceName : "None";
    projection.inputDeviceName = setup.inputDeviceName.isNotEmpty() ? setup.inputDeviceName : "None";
    projection.sampleRate = engine.getHostSampleRate();
    if (auto* device = engine.getDeviceManager().getCurrentAudioDevice())
    {
        projection.blockSize = device->getCurrentBufferSizeSamples();
        projection.activeOutputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
        projection.activeInputChannels = device->getActiveInputChannels().countNumberOfSetBits();
    }
    else
    {
        projection.blockSize = 512;
    }

    projection.busLayout = "stereo output, optional mono input";
    projection.externalInputState = standaloneExternalInputState(engine);
    projection.inputMeterLevel = engine.getInputPeakLevel();
    projection.outputState = standaloneOutputState(engine);

    if (setup.inputDeviceName.isEmpty())
    {
        projection.inputState = "unavailable";
    }
    else if (projection.activeInputChannels == 0)
    {
        projection.inputState = "unavailable";
    }
    else
    {
        projection.inputState = standaloneExternalInputState(engine);
    }

    return projection;
}

DesktopV2AudioStateProjection buildHostedAudioStateProjection(const AudioEngine& engine,
                                                              const juce::AudioProcessor& processor)
{
    DesktopV2AudioStateProjection projection;
    projection.sampleRate = static_cast<double>(engine.getHostSampleRate());
    projection.blockSize = processor.getBlockSize();

    const int inputBusCount = processor.getBusCount(true);
    const int outputBusCount = processor.getBusCount(false);
    projection.busLayout = "in buses " + juce::String(inputBusCount) + ", out buses "
                         + juce::String(outputBusCount);

    if (auto* inputBus = processor.getBus(true, 0))
    {
        projection.activeInputChannels = inputBus->getNumberOfChannels();
        projection.inputDeviceName = inputBus->getName();
    }
    if (auto* outputBus = processor.getBus(false, 0))
    {
        projection.activeOutputChannels = outputBus->getNumberOfChannels();
        projection.outputDeviceName = outputBus->getName();
    }

    projection.externalInputState = engine.isExternalInputEnabled() ? "active" : "unavailable";
    projection.inputState = projection.activeInputChannels > 0 ? projection.externalInputState : "unavailable";
    projection.outputState = projection.activeOutputChannels > 0 ? "active" : "unavailable";
    projection.inputMeterLevel = engine.getInputPeakLevel();
    return projection;
}
