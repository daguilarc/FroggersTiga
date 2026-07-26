#include "HostedMainComponentV2.h"

#include "ui/DesktopV2ChromeLayout.hpp"

HostedMainComponentV2::HostedMainComponentV2(froggers_v2::FroggersV2AppCoreFacade& facade)
    : m_facade(facade)
    , m_hostedStatusPanel(m_facade.audioEngine(), nullptr)
    , m_hostCallbacks(m_facade.controlCore(),
                      m_facade.hostBridge(),
                      m_facade.host(),
                      m_carousel,
                      m_lastModRoutesVersion)
{
    m_globalOscilloscope.bindHost(&m_facade.host());
    m_globalOscilloscope.bindLaneHistory(&m_cvLaneHistory);
    m_globalStrip.bind(&m_facade.host(), &m_facade.controlCore());
    m_carousel.bindHost(&m_facade.host(), &m_facade.controlCore());
    m_carousel.submodulePanel().bindLaneHistory(&m_cvLaneHistory);
    m_carousel.adsrPanel().bindLaneHistory(&m_cvLaneHistory);
    m_performanceBand.bind(&m_facade.controlCore());
    m_performanceBand.bindHost(&m_facade.host());

    addAndMakeVisible(m_globalOscilloscope);
    addAndMakeVisible(m_globalStrip);
    addAndMakeVisible(m_performanceBand);
    addAndMakeVisible(m_carousel);
    addAndMakeVisible(m_hostedStatusPanel);

    wireCallbacks();
    m_carousel.selectPage(m_facade.config().defaultPage, false);

    setWantsKeyboardFocus(true);
    setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
    startTimerHz(15);
}

void HostedMainComponentV2::setHostedProcessor(juce::AudioProcessor* processor)
{
    m_hostedStatusPanel.setProcessor(processor);
}

void HostedMainComponentV2::wireCallbacks()
{
    desktop_v2::refreshAndWireHostCallbacks(m_hostCallbacks,
                                            m_facade.controlCore(),
                                            m_facade.hostBridge(),
                                            m_facade.host(),
                                            m_carousel,
                                            m_lastModRoutesVersion);
}

void HostedMainComponentV2::pushSelectPage(uint8_t page)
{
    desktop_v2::pushSelectPage(m_hostCallbacks, page);
}

bool HostedMainComponentV2::keyPressed(const juce::KeyPress& key)
{
    juce::ignoreUnused(key);
    return false;
}

bool HostedMainComponentV2::keyStateChanged(bool /*isKeyDown*/)
{
    return false;
}

void HostedMainComponentV2::timerCallback()
{
    const uint32_t prevModRoutesVersion = m_lastModRoutesVersion;
    m_facade.publishUiFrame();
    m_lastModRoutesVersion = m_facade.lastModRoutesVersion();
    if (m_lastModRoutesVersion != prevModRoutesVersion)
    {
        m_carousel.refresh();
    }

    const uint32_t version = m_facade.uiState().version.load(std::memory_order_acquire);
    if (version != m_lastUiVersion)
    {
        m_lastUiVersion = version;
        m_carousel.refresh();
        m_globalStrip.refresh();
        m_performanceBand.refresh();
    }

    m_globalOscilloscope.setAudioRunning(true);
    m_globalOscilloscope.setExternalAudioAvailable(m_facade.audioEngine().isExternalInputEnabled());
    m_facade.controlCore().setExternalAudioAvailable(m_facade.audioEngine().isExternalInputEnabled());
    DesktopHostIO& host = m_facade.host();
    for (uint8_t lane = 0; lane < CvLaneHistoryStore::kNumLanes; ++lane)
    {
        m_cvLaneHistory.pushLaneSample(lane, host.GetCvOut(lane));
    }
    m_globalOscilloscope.refreshTraces();
    m_carousel.submodulePanel().refresh();
    m_carousel.adsrPanel().refresh();
    m_performanceBand.refreshMarbles(true);
}

void HostedMainComponentV2::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds().reduced(kChromePad);

    auto transport = area.removeFromTop(kTransportRowH);
    m_globalOscilloscope.setBounds(transport);

    area.removeFromTop(kSectionGap);
    m_globalStrip.setBounds(area.removeFromTop(kGlobalCommandBandH));
    area.removeFromTop(kSectionGap);

    m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
    area.removeFromTop(kSectionGap);
    m_hostedStatusPanel.setBounds(area.removeFromTop(28));
    area.removeFromTop(kSectionGap);
    m_carousel.setBounds(area);
}
