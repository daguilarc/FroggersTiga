#include "PluginProcessorV2.h"

#include "HostParameterStateEnvelopeV2.hpp"
#include "PluginEditorV2.h"
#include "SimPresetSnapshot.hpp"

#include <cstring>
#include <vector>

namespace
{
// v5: Pair-AR refactor — legacy page6 Sus* APVTS stable IDs are dropped (ignored on load):
// page6_row1_knob/depth, page6_row4_knob/depth, page6_row7_knob/depth.
constexpr uint8_t kLegacyPluginStateEnvelopeVersionV5 = 5;
// v6: appends a HostParameterStateEnvelopeV2 tail after the legacy SimPresetSnapshot payload so
// the manifest-owned host parameters SimPresetSnapshot never serializes (GlobalCrunchy, VcoMorph,
// SceneBlend) survive a DAW project save/reload instead of silently
// resetting to construction defaults. v5-tagged states (no tail present) still load correctly;
// the tail is only applied when the byte count indicates it is present.
constexpr uint8_t kPluginStateEnvelopeVersion = 6;
}

FroggersTigaAudioProcessorV2::FroggersTigaAudioProcessorV2()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::mono(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
    , m_facade(m_audio, [] {
          froggers_v2::FroggersV2AppCoreConfig config;
          config.pluginHosted = true;
          return config;
      }())
{
    m_facade.initialize();
    m_paramRegistry.registerParameters(*this, m_pendingParams);
    syncParametersFromHost();
}

void FroggersTigaAudioProcessorV2::syncParametersFromHost()
{
    m_paramRegistry.syncFromHost(m_audio.getHost(),
                                 m_audio.getDelay(),
                                 m_audio.getSequencer(),
                                 m_facade.controlCore(),
                                 m_pendingParams);
}

const juce::String FroggersTigaAudioProcessorV2::getName() const
{
    return JucePlugin_Name;
}

bool FroggersTigaAudioProcessorV2::acceptsMidi() const
{
    return true;
}

bool FroggersTigaAudioProcessorV2::producesMidi() const
{
    return false;
}

bool FroggersTigaAudioProcessorV2::isMidiEffect() const
{
    return false;
}

double FroggersTigaAudioProcessorV2::getTailLengthSeconds() const
{
    return 0.0;
}

bool FroggersTigaAudioProcessorV2::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    if (input != juce::AudioChannelSet::disabled() && input != juce::AudioChannelSet::mono())
    {
        return false;
    }
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

int FroggersTigaAudioProcessorV2::getNumPrograms()
{
    return 1;
}

int FroggersTigaAudioProcessorV2::getCurrentProgram()
{
    return 0;
}

void FroggersTigaAudioProcessorV2::setCurrentProgram(int)
{
}

const juce::String FroggersTigaAudioProcessorV2::getProgramName(int)
{
    return {};
}

void FroggersTigaAudioProcessorV2::changeProgramName(int, const juce::String&)
{
}

void FroggersTigaAudioProcessorV2::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_facade.prepare(static_cast<float>(sampleRate), samplesPerBlock);
}

void FroggersTigaAudioProcessorV2::releaseResources()
{
}

void FroggersTigaAudioProcessorV2::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    for (const auto metadata : midiMessages)
    {
        m_audio.ingestMidiMessage(metadata.getMessage());
    }

    m_paramRegistry.applyPending(m_audio.getHost(),
                                 m_audio.getDelay(),
                                 m_audio.getSequencer(),
                                 m_facade.controlCore(),
                                 m_pendingParams);

    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const float* inputData = numInputChannels > 0 ? buffer.getReadPointer(0) : nullptr;
    float* outL = buffer.getWritePointer(0);
    float* outR = numOutputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    m_facade.processHostedBlock(inputData,
                                numInputChannels,
                                outL,
                                outR,
                                numOutputChannels,
                                numSamples);
}

bool FroggersTigaAudioProcessorV2::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FroggersTigaAudioProcessorV2::createEditor()
{
    return new FroggersTigaAudioProcessorEditorV2(*this);
}

void FroggersTigaAudioProcessorV2::getStateInformation(juce::MemoryBlock& destData)
{
    std::vector<uint8_t> bytes(
        1 + SimPresetSnapshot::serializedSize() + HostParameterStateEnvelopeV2::kTailBytes);
    if (!SimPresetSnapshot::write(m_audio.getHost(),
                                  m_audio.getDelay(),
                                  bytes.data() + 1,
                                  SimPresetSnapshot::serializedSize()))
    {
        return;
    }
    bytes[0] = kPluginStateEnvelopeVersion;
    HostParameterStateEnvelopeV2::writeTail(m_audio.getHost(),
                                            m_audio.getDelay(),
                                            m_audio.getSequencer(),
                                            m_facade.controlCore(),
                                            bytes.data() + 1 + SimPresetSnapshot::serializedSize());
    destData.replaceAll(bytes.data(), bytes.size());
}

void FroggersTigaAudioProcessorV2::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
    {
        return;
    }

    const auto* bytes = static_cast<const uint8_t*>(data);
    const size_t byteCount = static_cast<size_t>(sizeInBytes);

    uint32_t magic = 0;
    if (byteCount >= sizeof(magic))
    {
        std::memcpy(&magic, bytes, sizeof(magic));
    }

    if (magic == SimPresetSnapshot::kMagic)
    {
        SimPresetSnapshot::read(m_audio.getHost(), m_audio.getDelay(), data, byteCount);
        syncParametersFromHost();
        m_audio.notifyStateRestored();
        return;
    }

    if ((bytes[0] == kPluginStateEnvelopeVersion || bytes[0] == kLegacyPluginStateEnvelopeVersionV5)
        && byteCount > sizeof(magic) + 1)
    {
        const void* snapshotData = bytes + 1;
        const size_t snapshotBytes = byteCount - 1;
        uint32_t wrappedMagic = 0;
        std::memcpy(&wrappedMagic, snapshotData, sizeof(wrappedMagic));
        if (wrappedMagic == SimPresetSnapshot::kMagic
            && SimPresetSnapshot::read(m_audio.getHost(),
                                       m_audio.getDelay(),
                                       snapshotData,
                                       snapshotBytes))
        {
            // v6 only: replay the full-inventory tail (GlobalCrunchy/VcoMorph/
            // SceneBlend) if present. v5 states have no tail; those axes simply
            // keep whatever this AudioEngine instance already holds (construction defaults on a
            // freshly-created processor), matching the pre-existing v5 behavior exactly.
            const size_t tailOffset = 1 + SimPresetSnapshot::serializedSize();
            if (bytes[0] == kPluginStateEnvelopeVersion
                && byteCount >= tailOffset + HostParameterStateEnvelopeV2::kTailBytes)
            {
                HostParameterStateEnvelopeV2::readTail(bytes + tailOffset,
                                                       m_audio.getHost(),
                                                       m_audio.getDelay(),
                                                       m_audio.getSequencer(),
                                                       m_facade.controlCore());
            }
            syncParametersFromHost();
            m_audio.notifyStateRestored();
        }
        return;
    }

    SimPresetSnapshot::read(m_audio.getHost(), m_audio.getDelay(), data, byteCount);
    syncParametersFromHost();
    m_audio.notifyStateRestored();
}

AudioEngine& FroggersTigaAudioProcessorV2::getAudioEngine()
{
    return m_audio;
}

froggers_v2::FroggersV2AppCoreFacade& FroggersTigaAudioProcessorV2::getAppCoreFacade()
{
    return m_facade;
}

froggers_v2::FroggersV2ControlCore& FroggersTigaAudioProcessorV2::getControlCore()
{
    return m_facade.controlCore();
}

froggers_v2::FroggersV2HostBridge& FroggersTigaAudioProcessorV2::getHostBridge()
{
    return m_facade.hostBridge();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FroggersTigaAudioProcessorV2();
}
