#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "AudioEngine.h"
#include "control/FroggersV2AppCoreFacade.hpp"
#include "HostParameterRegistryV2.hpp"

class FroggersTigaAudioProcessorV2 : public juce::AudioProcessor
{
public:
    FroggersTigaAudioProcessorV2();
    ~FroggersTigaAudioProcessorV2() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    AudioEngine& getAudioEngine();
    froggers_v2::FroggersV2AppCoreFacade& getAppCoreFacade();
    froggers_v2::FroggersV2ControlCore& getControlCore();
    froggers_v2::FroggersV2HostBridge& getHostBridge();

private:
    void syncParametersFromHost();

    AudioEngine m_audio{true};
    froggers_v2::FroggersV2AppCoreFacade m_facade;
    HostParameterRegistryV2 m_paramRegistry;
    HostParameterPendingStoreV2 m_pendingParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FroggersTigaAudioProcessorV2)
};
