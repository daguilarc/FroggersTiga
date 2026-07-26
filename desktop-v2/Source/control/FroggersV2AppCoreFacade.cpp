#include "control/FroggersV2AppCoreFacade.hpp"

namespace froggers_v2
{
FroggersV2AppCoreFacade::FroggersV2AppCoreFacade(AudioEngine& audio,
                                                 const FroggersV2AppCoreConfig& config)
    : m_config(config)
    , m_audio(audio)
    , m_bridge(m_core, audio.getHost())
{
}

FroggersV2AppCoreFacade::~FroggersV2AppCoreFacade()
{
    shutdown();
}

void FroggersV2AppCoreFacade::initialize()
{
    if (m_initialized)
    {
        return;
    }

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = m_config.defaultPage;
    ingestMessage(selectPage);

    m_bridge.syncToHost();
    m_lastModRoutesVersion = m_audio.getHost().modRoutesVersion();
    m_initialized = true;
}

void FroggersV2AppCoreFacade::prepare(float sampleRate, int maxBlockSize)
{
    m_audio.setHostSampleRate(sampleRate);
    m_audio.prepareRenderBuffers(maxBlockSize);
}

void FroggersV2AppCoreFacade::shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    if (!m_config.pluginHosted)
    {
        m_audio.stopAudio();
    }
    m_initialized = false;
}

bool FroggersV2AppCoreFacade::pushMessage(const MessageIn& message)
{
    return m_core.bus().push(message);
}

void FroggersV2AppCoreFacade::processMessages(bool syncToHostAfter)
{
    m_core.processBus();
    if (syncToHostAfter)
    {
        m_bridge.syncToHost();
    }
}

void FroggersV2AppCoreFacade::ingestMessage(const MessageIn& message)
{
    pushMessage(message);
    processMessages(true);
}

void FroggersV2AppCoreFacade::processHostedBlock(const float* inputChannelData,
                                                 int numInputChannels,
                                                 float* outputLeft,
                                                 float* outputRight,
                                                 int numOutputChannels,
                                                 int numSamples)
{
    m_audio.processHostedBlock(inputChannelData,
                               numInputChannels,
                               outputLeft,
                               outputRight,
                               numOutputChannels,
                               numSamples);
}

void FroggersV2AppCoreFacade::syncHostModRoutesIfNeeded()
{
    DesktopHostIO& hostIo = m_audio.getHost();
    const uint32_t version = hostIo.modRoutesVersion();
    if (version == m_lastModRoutesVersion)
    {
        return;
    }
    m_lastModRoutesVersion = version;
    m_bridge.syncFromHostModRoutes();
}

void FroggersV2AppCoreFacade::pushModSourceSamples()
{
    DesktopHostIO& hostIo = m_audio.getHost();
    for (uint8_t lane = 0; lane < kNumModSources; ++lane)
    {
        MessageIn message;
        message.type = MessageIn::Type::Clock;
        message.index = static_cast<uint8_t>(lane + 6);
        message.value = hostIo.GetCvOut(lane);
        m_core.bus().push(message);
    }
    m_core.processBus();
}

void FroggersV2AppCoreFacade::pushExternalMidiMods()
{
    MidiCvAssignmentTable& table = m_audio.getMidiCvTable();
    for (uint8_t slot = 0; slot < 2; ++slot)
    {
        if (slot == 0 && !table.externalModA.enabled)
        {
            continue;
        }
        if (slot == 1 && !table.externalModB.enabled)
        {
            continue;
        }
        MessageIn message;
        message.type = MessageIn::Type::Clock;
        message.index = slot;
        message.value = table.externalModLevel(slot);
        m_core.bus().push(message);
    }
    m_core.processBus();
}

void FroggersV2AppCoreFacade::publishUiFrame(bool drainHostMutations)
{
    if (drainHostMutations
        && (m_config.pluginHosted || m_audio.shouldDrainPendingUiMutations()))
    {
        m_audio.getHost().DrainPendingMutations();
    }
    syncHostModRoutesIfNeeded();
    if (!m_config.pluginHosted)
    {
        pushExternalMidiMods();
    }
    pushModSourceSamples();
    m_bridge.syncToHost();
}

FroggersV2ControlCore& FroggersV2AppCoreFacade::controlCore()
{
    return m_core;
}

const FroggersV2ControlCore& FroggersV2AppCoreFacade::controlCore() const
{
    return m_core;
}

FroggersV2HostBridge& FroggersV2AppCoreFacade::hostBridge()
{
    return m_bridge;
}

const FroggersV2HostBridge& FroggersV2AppCoreFacade::hostBridge() const
{
    return m_bridge;
}

AudioEngine& FroggersV2AppCoreFacade::audioEngine()
{
    return m_audio;
}

const AudioEngine& FroggersV2AppCoreFacade::audioEngine() const
{
    return m_audio;
}

DesktopHostIO& FroggersV2AppCoreFacade::host()
{
    return m_audio.getHost();
}

const DesktopHostIO& FroggersV2AppCoreFacade::host() const
{
    return m_audio.getHost();
}

const FroggersV2UIState& FroggersV2AppCoreFacade::uiState() const
{
    return m_core.uiState();
}

uint32_t FroggersV2AppCoreFacade::lastModRoutesVersion() const
{
    return m_lastModRoutesVersion;
}

const FroggersV2AppCoreConfig& FroggersV2AppCoreFacade::config() const
{
    return m_config;
}
} // namespace froggers_v2
