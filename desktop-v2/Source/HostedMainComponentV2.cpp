#include "HostedMainComponentV2.h"

#include "DesktopV2HostCallbacks.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "V2ModTapBank.hpp"

HostedMainComponentV2::HostedMainComponentV2(AudioEngine& audio,
                                             froggers_v2::FroggersV2ControlCore& core,
                                             froggers_v2::FroggersV2HostBridge& bridge)
    : m_audio(audio)
    , m_core(core)
    , m_bridge(bridge)
{
    m_vcoEfScope.bindHost(&m_audio.getHost());
    m_carousel.bindCore(&m_core);
    m_performanceBand.bind(&m_core, &m_audio.getSequencer());
    m_performanceBand.bindHost(&m_audio.getHost());
    m_globalStrip.bind(&m_audio.getHost(), &m_core);
    m_sequencerPanel.bind(&m_audio.getSequencer(), &m_core, &m_bridge);

    addAndMakeVisible(m_vcoEfScope);
    addAndMakeVisible(m_performanceBand);
    addAndMakeVisible(m_carousel);
    addAndMakeVisible(m_globalStrip);
    addAndMakeVisible(m_sequencerPanel);

    wireCallbacks();
    pushSelectPage(0);
    m_carousel.setActivePage(0);

    setWantsKeyboardFocus(true);
    setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
    startTimerHz(15);
}

void HostedMainComponentV2::wireCallbacks()
{
    desktop_v2::wireCallbacks({m_core, m_bridge, m_audio.getHost(), m_carousel, m_lastModRoutesVersion});
}

void HostedMainComponentV2::pushRandomizeMod(uint8_t page)
{
    desktop_v2::pushRandomizeMod({m_core, m_bridge, m_audio.getHost(), m_carousel, m_lastModRoutesVersion}, page);
}

void HostedMainComponentV2::syncHostModRoutesIfNeeded()
{
    DesktopHostIO& host = m_audio.getHost();
    const uint32_t version = host.modRoutesVersion();
    if (version == m_lastModRoutesVersion)
    {
        return;
    }
    m_lastModRoutesVersion = version;
    m_bridge.syncFromHostModRoutes();
    m_carousel.refresh();
}

void HostedMainComponentV2::pushSelectPage(uint8_t page)
{
    desktop_v2::pushSelectPage({m_core, m_bridge, m_audio.getHost(), m_carousel, m_lastModRoutesVersion}, page);
}

void HostedMainComponentV2::pushModSourceSamples()
{
    DesktopHostIO& host = m_audio.getHost();
    for (uint8_t engine = V2ModTapBank::kFirstIndex; engine <= V2ModTapBank::kLastIndex; ++engine)
    {
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::Clock;
        message.index = static_cast<uint8_t>(engine - V2ModTapBank::kFirstIndex + 6);
        message.value = host.GetCvOut(engine);
        m_core.bus().push(message);
    }
    m_core.processBus();
}

void HostedMainComponentV2::updateShiftFromKeyboard()
{
    const bool shift = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    m_globalStrip.setShiftHeld(shift);
}

bool HostedMainComponentV2::keyPressed(const juce::KeyPress& key)
{
    juce::ignoreUnused(key);
    updateShiftFromKeyboard();
    return false;
}

bool HostedMainComponentV2::keyStateChanged(bool /*isKeyDown*/)
{
    updateShiftFromKeyboard();
    return false;
}

void HostedMainComponentV2::timerCallback()
{
    if (m_audio.isPluginHosted() || m_audio.shouldDrainPendingUiMutations())
    {
        m_audio.getHost().DrainPendingMutations();
    }
    syncHostModRoutesIfNeeded();
    pushModSourceSamples();
    m_bridge.syncToHost();

    const uint32_t version = m_core.uiState().version.load(std::memory_order_acquire);
    if (version != m_lastUiVersion)
    {
        m_lastUiVersion = version;
        m_carousel.refresh();
        m_performanceBand.refresh();
        m_globalStrip.refresh();
    }

    m_vcoEfScope.refresh(true);
    m_performanceBand.refreshMarbles(true);
    m_sequencerPanel.refresh();
}

void HostedMainComponentV2::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds().reduced(kChromePad);
    m_globalStrip.setBounds(area.removeFromBottom(kGlobalStripH));
    area.removeFromBottom(kSectionGap);

    if (m_sequencerVisible)
    {
        m_sequencerPanel.setBounds(area.removeFromBottom(kSequencerH));
        area.removeFromBottom(kSectionGap);
    }

    m_vcoEfScope.setBounds(area.removeFromTop(kVstScopeStripH));
    area.removeFromTop(kSectionGap);
    m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
    area.removeFromTop(kSectionGap);
    m_carousel.setBounds(area);
}
