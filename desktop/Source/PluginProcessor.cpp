#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "SimPresetSnapshot.hpp"

#include <vector>

FroggersTigaAudioProcessor::FroggersTigaAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::mono(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

const juce::String FroggersTigaAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FroggersTigaAudioProcessor::acceptsMidi() const
{
    return true;
}

bool FroggersTigaAudioProcessor::producesMidi() const
{
    return false;
}

bool FroggersTigaAudioProcessor::isMidiEffect() const
{
    return false;
}

double FroggersTigaAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FroggersTigaAudioProcessor::getNumPrograms()
{
    return 1;
}

int FroggersTigaAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FroggersTigaAudioProcessor::setCurrentProgram(int)
{
}

const juce::String FroggersTigaAudioProcessor::getProgramName(int)
{
    return {};
}

void FroggersTigaAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void FroggersTigaAudioProcessor::prepareToPlay(double sampleRate, int)
{
    m_audio.setHostSampleRate(static_cast<float>(sampleRate));
}

void FroggersTigaAudioProcessor::releaseResources()
{
}

void FroggersTigaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* bypassParam = getBypassParameter())
    {
        if (bypassParam->getValue() >= 0.5f)
        {
            buffer.clear();
            return;
        }
    }

    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const float* inputData = numInputChannels > 0 ? buffer.getReadPointer(0) : nullptr;
    float* outL = buffer.getWritePointer(0);
    float* outR = numOutputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    m_audio.processHostedBlock(inputData,
                               numInputChannels,
                               outL,
                               outR,
                               numOutputChannels,
                               numSamples,
                               midiMessages);
}

bool FroggersTigaAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FroggersTigaAudioProcessor::createEditor()
{
    return new FroggersTigaAudioProcessorEditor(*this);
}

void FroggersTigaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    std::vector<uint8_t> bytes(SimPresetSnapshot::serializedSize());
    if (SimPresetSnapshot::write(m_audio.getHost(),
                                 m_audio.getDelay(),
                                 bytes.data(),
                                 bytes.size()))
    {
        destData.replaceAll(bytes.data(), bytes.size());
    }
}

void FroggersTigaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
    {
        return;
    }
    SimPresetSnapshot::read(m_audio.getHost(),
                            m_audio.getDelay(),
                            data,
                            static_cast<size_t>(sizeInBytes));
}

AudioEngine& FroggersTigaAudioProcessor::getAudioEngine()
{
    return m_audio;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FroggersTigaAudioProcessor();
}
