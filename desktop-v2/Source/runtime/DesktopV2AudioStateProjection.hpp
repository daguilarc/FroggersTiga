#pragma once

#include "runtime/DesktopV2RuntimeProjection.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

class AudioEngine;

struct DesktopV2AudioStateProjection
{
    juce::String outputDeviceName;
    juce::String inputDeviceName;
    int activeOutputChannels = 0;
    int activeInputChannels = 0;
    double sampleRate = 0.0;
    int blockSize = 0;
    juce::String busLayout;
    juce::String externalInputState;
    float inputMeterLevel = 0.0f;
    juce::String outputState;
    juce::String inputState;
};

DesktopV2AudioStateProjection buildStandaloneAudioStateProjection(const AudioEngine& engine);
DesktopV2AudioStateProjection buildHostedAudioStateProjection(const AudioEngine& engine,
                                                              const juce::AudioProcessor& processor);
