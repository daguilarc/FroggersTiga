#include "AudioEngine.h"
#include "AudioSettingsComponent.h"
#include "DelayState.hpp"
#include "HostAudioConfig.hpp"
#include "MidiCvSettingsComponent.h"
#include "OwnedAllocationGuard.hpp"
#include "TanhSaturator.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kSilentPeakThreshold = 1.0e-4f;
} // namespace

AudioEngine::AudioEngine(bool pluginHosted)
    : m_pluginHosted(pluginHosted)
{
    m_host.setDelayState(&m_delay);
    m_host.m_hostKind = pluginHosted ? SimHostKind::VstV2 : SimHostKind::DesktopV2;
    m_host.Init();
    m_delay.init(m_hostSampleRate);
    m_host.m_midiOut = [this](uint8_t channel, uint8_t cc, uint8_t value) {
        if (m_midiOut)
        {
            m_midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(
                static_cast<int>(channel) + 1, static_cast<int>(cc), static_cast<int>(value)));
        }
    };
    m_midiCvTable.setHostPitchCallback([this](uint8_t page, uint8_t row, float value) {
        m_host.SetPageKnob(page, row, value);
    });
    m_midiCvTable.setHostGateCallback([this](bool high) { m_host.SetGate(high); });
    m_midiCvTable.setSequencerAdvanceHook([this]() {
        if (m_transportChangedCallback)
        {
            juce::MessageManager::callAsync(m_transportChangedCallback);
        }
    });
    if (m_pluginHosted)
    {
        m_host.SetSampleRate(m_hostSampleRate);
        m_delay.setSampleRate(m_hostSampleRate);
        m_audioRunning = true;
        prepareRenderBuffers(512);
        return;
    }

    // Stereo-default desktop init: equivalent to v1 initialiseWithDefaultDevices(0, 2).
    m_deviceManager.initialiseWithDefaultDevices(0, 2);
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_deviceManager.getAudioDeviceSetup(setup);
    setup.sampleRate = m_hostSampleRate;
    setup.inputDeviceName.clear();
    setup.inputChannels.clear();
    setup.useDefaultInputChannels = false;
    setup.outputChannels.clear();
    setup.outputChannels.setBit(0);
    setup.outputChannels.setBit(1);
    setup.useDefaultOutputChannels = false;
    m_deviceManager.setAudioDeviceSetup(setup, true);
    if (auto* device = m_deviceManager.getCurrentAudioDevice())
    {
        m_hostSampleRate = static_cast<float>(device->getCurrentSampleRate());
    }
    m_host.SetSampleRate(m_hostSampleRate);
    m_delay.setSampleRate(m_hostSampleRate);
    openDefaultMidi();
    prepareRenderBuffers(512);
}

AudioEngine::~AudioEngine()
{
    stopAudio();
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    routeMidiMessage(message, false);
}

void AudioEngine::routeMidiMessage(const juce::MidiMessage& message, bool fromQwerty)
{
    if (message.isMidiClock())
    {
        m_midiCvTable.onMidiClockTick(m_host.m_sequencer);
        return;
    }
    if (message.isMidiStart())
    {
        m_host.m_sequencer.m_playing = true;
        notifyTransportChanged();
        return;
    }
    if (message.isMidiStop())
    {
        m_host.m_sequencer.m_playing = false;
        notifyTransportChanged();
        return;
    }
    if (message.isMidiContinue())
    {
        m_host.m_sequencer.m_playing = true;
        notifyTransportChanged();
        return;
    }

    const uint8_t channel = static_cast<uint8_t>(message.getChannel());
    const uint8_t* raw = message.getRawData();
    if (raw == nullptr || message.getRawDataSize() < 1)
    {
        return;
    }
    const uint8_t status = raw[0];
    const uint8_t data1 = message.getRawDataSize() > 1 ? raw[1] : 0;
    const uint8_t data2 = message.getRawDataSize() > 2 ? raw[2] : 0;
    m_midiCvTable.processIncomingMessage(channel, status, data1, data2, fromQwerty);
}

void AudioEngine::applyMidiCvToHost()
{
    if (m_midiCvTable.gateEnabled)
    {
        m_host.SetGate(m_midiCvTable.gateHigh());
    }
    if (m_midiCvTable.pitchEnabled)
    {
        const MidiCvPitchTarget target = m_midiCvTable.pitchTarget();
        m_host.SetPageKnob(target.page, target.row, m_midiCvTable.pitchValue());
    }
}

void AudioEngine::openDefaultMidi()
{
    setMidiInputDevice(juce::String(kComputerKeyboardMidiId));
    setMidiOutputDevice(juce::String(kNoMidiOutId));
}

DesktopHostIO& AudioEngine::getHost()
{
    return m_host;
}

DelayState& AudioEngine::getDelay()
{
    return m_delay;
}

SequencerState& AudioEngine::getSequencer()
{
    return m_host.m_sequencer;
}

MidiCvAssignmentTable& AudioEngine::getMidiCvTable()
{
    return m_midiCvTable;
}

const MidiCvAssignmentTable& AudioEngine::getMidiCvTable() const
{
    return m_midiCvTable;
}

juce::AudioDeviceManager& AudioEngine::getDeviceManager()
{
    return m_deviceManager;
}

bool AudioEngine::isAudioRunning() const
{
    return m_audioRunning;
}

float AudioEngine::getInputPeakLevel() const
{
    if (!m_externalInputEnabled || !m_audioRunning)
    {
        return 0.0f;
    }
    return m_inputPeak;
}

InputRouteStatus AudioEngine::getInputRouteStatus() const
{
    return m_inputRouteStatus;
}

juce::String AudioEngine::getInputRouteMessage() const
{
    switch (m_inputRouteStatus)
    {
        case InputRouteStatus::NoInputChannels:
            return "No input channels — enable input in Audio Settings";
        case InputRouteStatus::SilentCapture:
            return "Input silent — check macOS Privacy → Microphone and line level";
        case InputRouteStatus::Idle:
        case InputRouteStatus::Ok:
            break;
    }
    return {};
}

void AudioEngine::syncInputChannelSetup()
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_deviceManager.getAudioDeviceSetup(setup);
    if (setup.inputDeviceName.isEmpty())
    {
        return;
    }
    setup.inputChannels.clear();
    setup.inputChannels.setBit(0);
    setup.useDefaultInputChannels = false;
    m_deviceManager.setAudioDeviceSetup(setup, true);
}

void AudioEngine::updateInputRouteStatus(int numInputChannels, float inputPeak, int numSamples)
{
    if (!m_externalInputEnabled || !m_audioRunning)
    {
        m_inputRouteStatus = InputRouteStatus::Idle;
        m_silentSampleCount = 0;
        return;
    }
    if (numInputChannels == 0)
    {
        m_inputRouteStatus = InputRouteStatus::NoInputChannels;
        m_silentSampleCount = 0;
        return;
    }
    if (inputPeak >= kSilentPeakThreshold)
    {
        m_inputRouteStatus = InputRouteStatus::Ok;
        m_silentSampleCount = 0;
        return;
    }
    m_silentSampleCount += numSamples;
    const int silentCaptureSamples = static_cast<int>(m_hostSampleRate);
    if (m_silentSampleCount >= silentCaptureSamples)
    {
        m_inputRouteStatus = InputRouteStatus::SilentCapture;
        return;
    }
    m_inputRouteStatus = InputRouteStatus::Ok;
}

void AudioEngine::setExternalInputEnabled(bool enabled)
{
    m_externalInputEnabled = enabled;
}

bool AudioEngine::isExternalInputEnabled() const
{
    return m_externalInputEnabled;
}

void AudioEngine::setTransportChangedCallback(std::function<void()> callback)
{
    m_transportChangedCallback = std::move(callback);
}

void AudioEngine::notifyTransportChanged()
{
    if (!m_transportChangedCallback)
    {
        return;
    }
    juce::MessageManager::callAsync(m_transportChangedCallback);
}

void AudioEngine::startAudio()
{
    if (m_audioRunning)
    {
        return;
    }
    if (!m_pluginHosted)
    {
        m_deviceManager.addAudioCallback(this);
    }
    m_audioRunning = true;
    notifyTransportChanged();
}

void AudioEngine::stopAudio()
{
    if (!m_audioRunning)
    {
        return;
    }
    if (!m_pluginHosted)
    {
        m_deviceManager.removeAudioCallback(this);
    }
    m_audioRunning = false;
    if (m_lastBlockNonFinite)
    {
        m_host.m_engine.SoftResetFxState();
        m_delay.softResetFx();
        m_lastBlockNonFinite = false;
    }
    notifyTransportChanged();
}

bool AudioEngine::isPluginHosted() const
{
    return m_pluginHosted;
}

bool AudioEngine::shouldDrainPendingUiMutations() const
{
    return !m_pluginHosted && !m_audioRunning;
}

void AudioEngine::notifyStateRestored()
{
    m_stateRestoreGeneration.fetch_add(1, std::memory_order_relaxed);
}

uint32_t AudioEngine::stateRestoreGeneration() const
{
    return m_stateRestoreGeneration.load(std::memory_order_relaxed);
}

void AudioEngine::showAudioSettings(juce::Component* parent)
{
    if (m_pluginHosted)
    {
        return;
    }
    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle = "Audio Settings";
    opts.dialogBackgroundColour = juce::Colour(0xff2b3038);
    opts.content.setOwned(new AudioSettingsComponent(*this, []() {}));
    opts.componentToCentreAround = parent;
    opts.useNativeTitleBar = true;
    opts.resizable = false;
    opts.launchAsync();
}

void AudioEngine::showMidiSettings(juce::Component* parent)
{
    if (m_pluginHosted)
    {
        return;
    }
    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle = "MIDI Settings";
    opts.dialogBackgroundColour = juce::Colour(0xff2b3038);
    opts.content.setOwned(new MidiCvSettingsComponent(*this, []() {}));
    opts.componentToCentreAround = parent;
    opts.useNativeTitleBar = true;
    opts.resizable = true;
    opts.launchAsync();
}

void AudioEngine::setMidiInputDevice(const juce::String& identifier)
{
    m_midiIn.reset();
    if (identifier.isEmpty())
    {
        m_midiInDeviceId.clear();
        return;
    }
    if (identifier == kComputerKeyboardMidiId)
    {
        m_midiInDeviceId = juce::String(kComputerKeyboardMidiId);
        return;
    }
    m_midiInDeviceId = identifier;
    m_midiIn = juce::MidiInput::openDevice(identifier, this);
    if (m_midiIn)
    {
        m_midiIn->start();
    }
}

bool AudioEngine::isComputerKeyboardMidiEnabled() const
{
    return m_midiInDeviceId == kComputerKeyboardMidiId;
}

bool AudioEngine::isHardwareMidiInputOpenFailed() const
{
    return !isComputerKeyboardMidiEnabled() && m_midiInDeviceId.isNotEmpty() && m_midiIn == nullptr;
}

void AudioEngine::feedComputerKeyboardCc1(uint8_t ccValue)
{
    if (!isComputerKeyboardMidiEnabled())
    {
        return;
    }
    const uint8_t channel = m_midiCvTable.qwertyMidiChannel;
    feedVirtualMidiMessage(juce::MidiMessage::controllerEvent(channel, 1, static_cast<int>(ccValue)));
}

void AudioEngine::feedVirtualMidiMessage(const juce::MidiMessage& message)
{
    if (!isComputerKeyboardMidiEnabled())
    {
        return;
    }
    routeMidiMessage(message, true);
}

const juce::String& AudioEngine::getMidiInputDeviceIdentifier() const
{
    return m_midiInDeviceId;
}

const juce::String& AudioEngine::getMidiOutputDeviceIdentifier() const
{
    return m_midiOutDeviceId;
}

void AudioEngine::setMidiOutputDevice(const juce::String& identifier)
{
    m_midiOut.reset();
    if (identifier.isEmpty() || identifier == kNoMidiOutId)
    {
        m_midiOutDeviceId = juce::String(kNoMidiOutId);
        return;
    }
    m_midiOutDeviceId = identifier;
    m_midiOut = juce::MidiOutput::openDevice(identifier);
}

void AudioEngine::prepareRenderBuffers(int maxExpectedBlockSize)
{
    const size_t capacity = static_cast<size_t>(std::max(1, maxExpectedBlockSize));
    m_renderBlockCapacity = capacity;
    if (m_inBlock.size() < capacity)
    {
        m_inBlock.resize(capacity, 0.0f);
    }
    if (m_monoBlock.size() < capacity)
    {
        m_monoBlock.resize(capacity, 0.0f);
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    syncInputChannelSetup();
    if (device)
    {
        m_hostSampleRate = static_cast<float>(device->getCurrentSampleRate());
    }
    m_host.SetSampleRate(m_hostSampleRate);
    m_delay.setSampleRate(m_hostSampleRate);
    const int blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    prepareRenderBuffers(blockSize);
}

void AudioEngine::audioDeviceStopped()
{
    m_audioRunning = false;
    if (m_lastBlockNonFinite)
    {
        m_host.m_engine.SoftResetFxState();
        m_delay.softResetFx();
        m_lastBlockNonFinite = false;
    }
    notifyTransportChanged();
}

void AudioEngine::audioDeviceError(const juce::String& errorMessage)
{
    juce::Logger::writeToLog("FroggersTigaV2 audio error: " + errorMessage);
    m_audioRunning = false;
    if (m_lastBlockNonFinite)
    {
        m_host.m_engine.SoftResetFxState();
        m_delay.softResetFx();
        m_lastBlockNonFinite = false;
    }
    notifyTransportChanged();
}

void AudioEngine::ingestMidiMessage(const juce::MidiMessage& message)
{
    routeMidiMessage(message, false);
}

void AudioEngine::processHostedBlock(const float* inputChannelData,
                                     int numInputChannels,
                                     float* outputChannelDataLeft,
                                     float* outputChannelDataRight,
                                     int numOutputChannels,
                                     int numSamples)
{
    if (!m_audioRunning || outputChannelDataLeft == nullptr)
    {
        return;
    }

    const float* inputChannel0 =
        inputChannelData && numInputChannels > 0 ? inputChannelData : nullptr;
    renderSimOutputBlock(inputChannel0,
                         numInputChannels,
                         outputChannelDataLeft,
                         outputChannelDataRight,
                         numOutputChannels,
                         numSamples);

    if (m_lastBlockNonFinite)
    {
        m_host.m_engine.SoftResetFxState();
        m_delay.softResetFx();
        m_lastBlockNonFinite = false;
    }
}

void AudioEngine::setHostSampleRate(float sampleRate)
{
    m_hostSampleRate = sampleRate;
    m_host.SetSampleRate(sampleRate);
    m_delay.setSampleRate(sampleRate);
}

float AudioEngine::getHostSampleRate() const
{
    return m_hostSampleRate;
}

void AudioEngine::renderSimOutputChunk(const float* inputChannel0,
                                       int numInputChannels,
                                       float* outL,
                                       float* outR,
                                       int numOutputChannels,
                                       int numSamples)
{
    FROGGERS_OWNED_ALLOCATION_GUARD();
    applyMidiCvToHost();
    const size_t n = static_cast<size_t>(numSamples);
    std::fill(m_inBlock.begin(), m_inBlock.begin() + static_cast<ptrdiff_t>(n), 0.0f);
    float inputPeak = 0.0f;

    if (m_externalInputEnabled && inputChannel0 && numInputChannels > 0)
    {
        for (int i = 0; i < numSamples; i++)
        {
            const float sample = inputChannel0[i];
            m_inBlock[static_cast<size_t>(i)] = sample;
            inputPeak = std::max(inputPeak, std::fabs(sample));
        }
    }
    m_inputPeak = inputPeak;
    updateInputRouteStatus(numInputChannels, inputPeak, numSamples);

    m_host.tickControls();
    m_delay.beginBlock(&m_host.m_pageManager.m_modMgr);
    m_host.m_engine.ProcessBlock(m_inBlock.data(), m_monoBlock.data(), n);
    m_host.updateMarblesScopeAccum();
    const StereoFxSpread spread = makeStereoFxSpread(
        m_delay,
        m_host.m_engine.getReverbStereoDeltaL(),
        m_host.m_engine.getReverbStereoDeltaR(),
        m_host.m_engine.getLastRvMix());
    applyStereoBus(m_monoBlock.data(), outL, outR, n, spread, numOutputChannels);

    for (int i = 0; i < numSamples; i++)
    {
        outL[i] = TanhSaturator<false>::Saturate(outL[i]);
        if (outR != nullptr)
        {
            outR[i] = TanhSaturator<false>::Saturate(outR[i]);
        }
    }

    bool blockNonFinite = false;
    for (size_t i = 0; i < n; i++)
    {
        if (!std::isfinite(m_monoBlock[i]))
        {
            blockNonFinite = true;
            break;
        }
    }
    m_lastBlockNonFinite = blockNonFinite;
}

void AudioEngine::renderSimOutputBlock(const float* inputChannel0,
                                       int numInputChannels,
                                       float* outL,
                                       float* outR,
                                       int numOutputChannels,
                                       int numSamples)
{
    if (!outL || numSamples <= 0)
    {
        return;
    }

    const int capacity = static_cast<int>(m_renderBlockCapacity);
    if (numSamples <= capacity)
    {
        renderSimOutputChunk(inputChannel0, numInputChannels, outL, outR, numOutputChannels, numSamples);
    }
    else
    {
        int offset = 0;
        while (offset < numSamples)
        {
            const int chunk = std::min(numSamples - offset, capacity);
            const float* inPtr =
                inputChannel0 && numInputChannels > 0 ? inputChannel0 + offset : nullptr;
            float* chunkOutR = outR != nullptr ? outR + offset : nullptr;
            renderSimOutputChunk(
                inPtr, numInputChannels, outL + offset, chunkOutR, numOutputChannels, chunk);
            offset += chunk;
        }
    }

    if (m_midiOut)
    {
        m_host.m_midiBridge.tickMidiOut(m_host.m_engine.GetEnvelopeLevel(), m_host.m_midiOut);
    }
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    if (!outputChannelData || numOutputChannels < 1 || !outputChannelData[0])
    {
        return;
    }

    float* outL = outputChannelData[0];
    float* outR = numOutputChannels >= 2 && outputChannelData[1] ? outputChannelData[1] : nullptr;
    const float* inputChannel0 =
        inputChannelData && numInputChannels > 0 ? inputChannelData[0] : nullptr;
    renderSimOutputBlock(inputChannel0, numInputChannels, outL, outR, numOutputChannels, numSamples);

    if (m_lastBlockNonFinite)
    {
        m_host.m_engine.SoftResetFxState();
        m_delay.softResetFx();
        m_lastBlockNonFinite = false;
    }
}
