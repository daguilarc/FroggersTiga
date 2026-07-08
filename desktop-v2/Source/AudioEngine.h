#pragma once

#include "AudioRecorder.h"
#include "control/MidiCvAssignmentTable.hpp"
#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostAudioConfig.hpp"
#include "ExportFormat.hpp"
#include "SimModSource.hpp"
#include "runtime/DesktopV2AudioStateProjection.hpp"
#include "runtime/FilePatchRuntimeState.hpp"
#include "runtime/DesktopV2AudioStateProjection.hpp"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

enum class InputRouteStatus
{
    Idle,
    Ok,
    NoInputChannels,
    SilentCapture
};

class AudioEngine : public juce::AudioIODeviceCallback,
                    private juce::MidiInputCallback
{
public:
    static constexpr const char* kNoMidiOutId = "__none__";
    static constexpr const char* kComputerKeyboardMidiId = "__computer_keyboard__";

    AudioEngine(bool pluginHosted = false);
    ~AudioEngine() override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& errorMessage) override;

    DesktopHostIO& getHost();
    DelayState& getDelay();
    SequencerState& getSequencer();
    MidiCvAssignmentTable& getMidiCvTable();
    const MidiCvAssignmentTable& getMidiCvTable() const;
    juce::AudioDeviceManager& getDeviceManager();
    const juce::AudioDeviceManager& getDeviceManager() const;
    bool isAudioRunning() const;
    float getInputPeakLevel() const;
    InputRouteStatus getInputRouteStatus() const;
    juce::String getInputRouteMessage() const;
    void startAudio();
    void stopAudio();
    void showAudioSettings(juce::Component* parent);
    void showMidiSettings(juce::Component* parent);
    void setMidiInputDevice(const juce::String& identifier);
    void setMidiOutputDevice(const juce::String& identifier);
    bool isComputerKeyboardMidiEnabled() const;
    bool isHardwareMidiInputOpenFailed() const;
    void feedComputerKeyboardCc1(uint8_t ccValue);
    void feedVirtualMidiMessage(const juce::MidiMessage& message);
    const juce::String& getMidiInputDeviceIdentifier() const;
    const juce::String& getMidiOutputDeviceIdentifier() const;
    void setTransportChangedCallback(std::function<void()> callback);
    void setExternalInputEnabled(bool enabled);
    bool isExternalInputEnabled() const;
    bool isPluginHosted() const;
    bool shouldDrainPendingUiMutations() const;
    void notifyStateRestored();
    uint32_t stateRestoreGeneration() const;

    bool startRecording();
    void stopRecording();
    bool isRecording() const;
    bool hasCapturedAudio() const;
    bool wasLastCaptureTruncated() const;
    void exportCapturedAudio(ExportFormat format,
                             juce::Component* parent,
                             std::function<void(bool success, const juce::String& message)> onDone);
    void exportCapturedAudio(juce::Component* parent,
                             std::function<void(bool success, const juce::String& message)> onDone);

    ExportFormat exportFormat() const;
    void setExportFormat(ExportFormat format);

    FilePatchRuntimeState& filePatchState();
    const FilePatchRuntimeState& filePatchState() const;
    bool savePatchToFile(const juce::File& file, juce::String& errorOut);
    bool loadPatchFromFile(const juce::File& file, juce::String& errorOut);
    void revertPatch(juce::String& messageOut);
    DesktopV2AudioStateProjection buildAudioStateProjection() const;

    void setHostSampleRate(float sampleRate);
    float getHostSampleRate() const;
    void prepareRenderBuffers(int maxExpectedBlockSize);
    void processHostedBlock(const float* inputChannelData,
                            int numInputChannels,
                            float* outputChannelDataLeft,
                            float* outputChannelDataRight,
                            int numOutputChannels,
                            int numSamples);
    void ingestMidiMessage(const juce::MidiMessage& message);

private:
    void renderSimOutputChunk(const float* inputChannel0,
                              int numInputChannels,
                              float* outL,
                              float* outR,
                              int numOutputChannels,
                              int numSamples);
    void renderSimOutputBlock(const float* inputChannel0,
                              int numInputChannels,
                              float* outL,
                              float* outR,
                              int numOutputChannels,
                              int numSamples);
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;
    void openDefaultMidi();
    void notifyTransportChanged();
    void syncInputChannelSetup();
    void updateInputRouteStatus(int numInputChannels, float inputPeak, int numSamples);
    bool writeCaptureToFile(const juce::File& file, ExportFormat format, juce::String& errorOut);
    void applyMidiCvToHost();
    void routeMidiMessage(const juce::MidiMessage& message, bool fromQwerty);

    DesktopHostIO m_host;
    DelayState m_delay;
    juce::AudioDeviceManager m_deviceManager;
    AudioRecorder m_recorder;
    std::unique_ptr<juce::MidiInput> m_midiIn;
    std::unique_ptr<juce::MidiOutput> m_midiOut;
    juce::String m_midiInDeviceId{kComputerKeyboardMidiId};
    juce::String m_midiOutDeviceId{kNoMidiOutId};
    std::function<void()> m_transportChangedCallback;
    bool m_audioRunning = false;
    bool m_pluginHosted = false;
    std::atomic<uint32_t> m_stateRestoreGeneration{0};
    bool m_externalInputEnabled = false;
    float m_inputPeak = 0.0f;
    InputRouteStatus m_inputRouteStatus = InputRouteStatus::Idle;
    int m_silentSampleCount = 0;
    bool m_lastBlockNonFinite = false;
    float m_hostSampleRate = static_cast<float>(HostAudioConfig::kDefaultSampleRate);
    std::vector<float> m_inBlock;
    std::vector<float> m_monoBlock;
    MidiCvAssignmentTable m_midiCvTable;
    FilePatchRuntimeState m_filePatchState;
    std::vector<uint8_t> m_revertSnapshot;
    size_t m_renderBlockCapacity = 512;
    ExportFormat m_exportFormat = ExportFormat::Wav;
};
