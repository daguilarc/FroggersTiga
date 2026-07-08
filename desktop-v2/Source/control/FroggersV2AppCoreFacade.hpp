#pragma once

#include "AudioEngine.h"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "HostAudioConfig.hpp"

#include <cstdint>

namespace froggers_v2
{
struct FroggersV2AppCoreConfig
{
    bool pluginHosted = false;
    float sampleRate = static_cast<float>(HostAudioConfig::kDefaultSampleRate);
    int maxBlockSize = 512;
    uint8_t defaultPage = 0;
};

class FroggersV2AppCoreFacade
{
public:
    explicit FroggersV2AppCoreFacade(AudioEngine& audio,
                                     const FroggersV2AppCoreConfig& config = {});
    ~FroggersV2AppCoreFacade();

    void initialize();
    void prepare(float sampleRate, int maxBlockSize);
    void shutdown();

    bool pushMessage(const MessageIn& message);
    void processMessages(bool syncToHostAfter = true);
    void ingestMessage(const MessageIn& message);

    void processHostedBlock(const float* inputChannelData,
                            int numInputChannels,
                            float* outputLeft,
                            float* outputRight,
                            int numOutputChannels,
                            int numSamples);

    void publishUiFrame(bool drainHostMutations = true);
    void syncHostModRoutesIfNeeded();

    FroggersV2ControlCore& controlCore();
    const FroggersV2ControlCore& controlCore() const;
    FroggersV2HostBridge& hostBridge();
    const FroggersV2HostBridge& hostBridge() const;
    AudioEngine& audioEngine();
    const AudioEngine& audioEngine() const;
    DesktopHostIO& host();
    const DesktopHostIO& host() const;
    const FroggersV2UIState& uiState() const;
    uint32_t lastModRoutesVersion() const;
    const FroggersV2AppCoreConfig& config() const;

private:
    void pushModSourceSamples();
    void pushExternalMidiMods();

    FroggersV2AppCoreConfig m_config;
    AudioEngine& m_audio;
    FroggersV2ControlCore m_core;
    FroggersV2HostBridge m_bridge;
    uint32_t m_lastModRoutesVersion = 0;
    bool m_initialized = false;
};
} // namespace froggers_v2
