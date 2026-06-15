#include "AudioEngine.h"
#include "AudioSettingsComponent.h"
#include "DelayState.hpp"
#include "HostAudioConfig.hpp"
#include "MidiSettingsComponent.h"
#include "TanhSaturator.hpp"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kSilentPeakThreshold = 1.0e-4f;

juce::String extensionForFormat(ExportFormat format)
{
    switch (format)
    {
        case ExportFormat::Wav:
            return "wav";
        case ExportFormat::Mp3:
            return "mp3";
        case ExportFormat::Flac:
            return "flac";
        case ExportFormat::Ogg:
            return "ogg";
    }
    return "wav";
}

juce::String formatDisplayName(ExportFormat format)
{
    switch (format)
    {
        case ExportFormat::Wav:
            return "WAV";
        case ExportFormat::Mp3:
            return "MP3";
        case ExportFormat::Flac:
            return "FLAC";
        case ExportFormat::Ogg:
            return "OGG";
    }
    return "WAV";
}

std::unique_ptr<juce::AudioFormat> createFormatWriter(ExportFormat format)
{
    switch (format)
    {
        case ExportFormat::Wav:
            return std::make_unique<juce::WavAudioFormat>();
        case ExportFormat::Mp3:
#if JUCE_USE_MP3AUDIOFORMAT
            return std::make_unique<juce::LameEncoderAudioFormat>();
#else
            return nullptr;
#endif
        case ExportFormat::Flac:
#if JUCE_USE_FLAC
            return std::make_unique<juce::FlacAudioFormat>();
#else
            return nullptr;
#endif
        case ExportFormat::Ogg:
            return std::make_unique<juce::OggVorbisAudioFormat>();
    }
    return nullptr;
}
} // namespace

AudioEngine::AudioEngine(bool pluginHosted)
    : m_pluginHosted(pluginHosted)
{
    m_host.setDelayState(&m_delay);
    m_host.Init();
    m_delay.init(m_hostSampleRate);
    m_host.m_engine.SetSimFxInsert(simDelayInsertCallback, &m_delay);
    m_host.m_midiOut = [this](uint8_t channel, uint8_t cc, uint8_t value) {
        if (m_midiOut)
        {
            m_midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(
                static_cast<int>(channel) + 1, static_cast<int>(cc), static_cast<int>(value)));
        }
    };
    if (m_pluginHosted)
    {
        m_host.SetSampleRate(m_hostSampleRate);
        m_delay.setSampleRate(m_hostSampleRate);
        m_audioRunning = true;
        return;
    }

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
}

AudioEngine::~AudioEngine()
{
    stopAudio();
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput*,
                                            const juce::MidiMessage& message)
{
    if (!message.isController())
    {
        return;
    }
    const uint8_t channel = static_cast<uint8_t>(message.getChannel() - 1);
    m_host.m_midiBridge.PushMidiCc(
        channel,
        static_cast<uint8_t>(message.getControllerNumber()),
        static_cast<uint8_t>(message.getControllerValue()));
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

juce::AudioDeviceManager& AudioEngine::getDeviceManager()
{
    return m_deviceManager;
}

bool AudioEngine::isAudioRunning() const
{
    return m_audioRunning;
}

float AudioEngine::getEnvelopeLevel() const
{
    if (!m_externalInputEnabled)
    {
        return 0.0f;
    }
    return m_host.m_engine.GetEnvelopeLevel();
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
    if (m_pluginHosted)
    {
        m_audioRunning = true;
        notifyTransportChanged();
        return;
    }
    m_deviceManager.addAudioCallback(this);
    m_audioRunning = true;
}

void AudioEngine::stopAudio()
{
    if (!m_audioRunning)
    {
        return;
    }
    if (m_recorder.isActive())
    {
        m_recorder.stop();
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
    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle = m_pluginHosted ? "MIDI CC Settings" : "MIDI Settings";
    opts.dialogBackgroundColour = juce::Colour(0xff2b3038);
    opts.content.setOwned(new MidiSettingsComponent(*this, []() {}, m_pluginHosted));
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
    auto& bridge = m_host.m_midiBridge;
    if (!bridge.isCcPairEnabled(0))
    {
        return;
    }
    bridge.PushMidiCc(bridge.m_inChannel1, bridge.m_inCc1, ccValue);
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

bool AudioEngine::startRecording()
{
    if (!m_audioRunning)
    {
        return false;
    }
    m_recorder.start();
    return true;
}

void AudioEngine::stopRecording()
{
    m_recorder.stop();
}

bool AudioEngine::isRecording() const
{
    return m_recorder.isActive();
}

bool AudioEngine::hasCapturedAudio() const
{
    return m_recorder.getSampleCount() > 0;
}

bool AudioEngine::wasLastCaptureTruncated() const
{
    return m_recorder.wasTruncated();
}

bool AudioEngine::writeCaptureToFile(const juce::File& file,
                                     ExportFormat format,
                                     juce::String& errorOut)
{
    const size_t sampleCount = m_recorder.getSampleCount();
    if (sampleCount == 0)
    {
        errorOut = "No audio captured.";
        return false;
    }

    auto audioFormat = createFormatWriter(format);
    if (!audioFormat)
    {
        errorOut = formatDisplayName(format) + " encoder is not available in this build. Try WAV.";
        return false;
    }

    juce::AudioBuffer<float> buffer(2, static_cast<int>(sampleCount));
    const auto& interleaved = m_recorder.getInterleaved();
    for (size_t i = 0; i < sampleCount; ++i)
    {
        buffer.setSample(0, static_cast<int>(i), interleaved[i * 2]);
        buffer.setSample(1, static_cast<int>(i), interleaved[i * 2 + 1]);
    }

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
    if (!stream)
    {
        errorOut = "Could not open file for writing.";
        return false;
    }

    juce::StringPairArray metadata;
    std::unique_ptr<juce::AudioFormatWriter> writer(audioFormat->createWriterFor(
        stream.get(),
        m_hostSampleRate,
        2,
        format == ExportFormat::Wav ? 24 : 0,
        metadata,
        format == ExportFormat::Ogg ? 6 : 0));
    if (!writer)
    {
        errorOut = "Could not create " + formatDisplayName(format) + " writer.";
        return false;
    }
    stream.release();
    if (!writer->writeFromAudioSampleBuffer(buffer, 0, static_cast<int>(sampleCount)))
    {
        errorOut = "Write failed.";
        return false;
    }
    return true;
}

void AudioEngine::exportCapturedAudio(ExportFormat format,
                                      juce::Component* parent,
                                      std::function<void(bool success, const juce::String& message)> onDone)
{
    const juce::String ext = extensionForFormat(format);
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save recording",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*." + ext);

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, format, onDone, chooser](const juce::FileChooser& fc) {
            const juce::File file = fc.getResult();
            if (file == juce::File())
            {
                if (onDone)
                {
                    onDone(false, "Save cancelled.");
                }
                return;
            }
            juce::File target = file;
            if (!target.hasFileExtension(extensionForFormat(format)))
            {
                target = target.withFileExtension(extensionForFormat(format));
            }
            juce::String error;
            const bool ok = writeCaptureToFile(target, format, error);
            if (onDone)
            {
                onDone(ok, ok ? ("Saved to " + target.getFullPathName()) : error);
            }
        });
    juce::ignoreUnused(parent);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    syncInputChannelSetup();
    if (device)
    {
        m_hostSampleRate = static_cast<float>(device->getCurrentSampleRate());
        juce::Logger::writeToLog(
            "FroggersTiga: audio input channels active: "
            + juce::String(device->getActiveInputChannels().countNumberOfSetBits())
            + " at "
            + juce::String(m_hostSampleRate, 0)
            + " Hz");
    }
    m_host.SetSampleRate(m_hostSampleRate);
    m_delay.setSampleRate(m_hostSampleRate);
}

void AudioEngine::audioDeviceStopped()
{
    m_audioRunning = false;
    if (m_recorder.isActive())
    {
        m_recorder.stop();
    }
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
    juce::Logger::writeToLog("FroggersTiga audio error: " + errorMessage);
    m_audioRunning = false;
    if (m_recorder.isActive())
    {
        m_recorder.stop();
    }
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
    handleIncomingMidiMessage(nullptr, message);
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

void AudioEngine::renderSimOutputBlock(const float* inputChannel0,
                                       int numInputChannels,
                                       float* outL,
                                       float* outR,
                                       int numOutputChannels,
                                       int numSamples)
{
    if (!outL)
    {
        return;
    }

    const size_t n = static_cast<size_t>(numSamples);
    if (m_inBlock.size() < n)
    {
        m_inBlock.resize(n, 0.0f);
    }
    if (m_monoBlock.size() < n)
    {
        m_monoBlock.resize(n, 0.0f);
    }
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
    m_host.m_pairAr.beginBlock(m_host.m_pageManager.m_modMgr.m_mods);
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

    if (m_recorder.isActive())
    {
        if (outR != nullptr)
        {
            m_recorder.appendStereo(outL, outR, n);
        }
        else
        {
            m_recorder.appendStereo(outL, outL, n);
        }
    }

    if (m_midiOut)
    {
        m_host.m_midiBridge.tickMidiOut(m_host.m_engine.GetEnvelopeLevel(), m_host.m_midiOut);
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

void AudioEngine::processHostedBlock(const float* inputChannelData,
                                     int numInputChannels,
                                     float* outputChannelDataLeft,
                                     float* outputChannelDataRight,
                                     int numOutputChannels,
                                     int numSamples,
                                     const juce::MidiBuffer& midiIn)
{
    if (!m_audioRunning || !outputChannelDataLeft)
    {
        return;
    }

    for (const juce::MidiMessageMetadata metadata : midiIn)
    {
        ingestMidiMessage(metadata.getMessage());
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
