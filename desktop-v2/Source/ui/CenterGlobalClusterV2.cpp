#include "ui/CenterGlobalClusterV2.hpp"

#include "ParamDisplayNames.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

CenterGlobalClusterV2::CenterGlobalClusterV2()
    : m_randAll(ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandAll))
    , m_randMods(ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandMods))
    , m_randWaveforms(
          ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandWaveforms))
    , m_randResample(
          ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::MarblesStep))
{
    m_crunchyLabel.setText("Crunchy", juce::dontSendNotification);
    m_crunchyLabel.setJustificationType(juce::Justification::centred);

    m_crunchyRing.onTurn = [this](uint8_t, float delta) {
        if (!m_core)
        {
            return;
        }
        m_core->bus().push(
            froggers_v2::MessageIn::ParamTurn(froggers_v2::kNumHostPages, 0, delta));
        m_core->processBus();
    };
    m_crunchyRing.onPress = [this](uint8_t) {
        if (!m_core)
        {
            return;
        }
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::ParamPress;
        message.page = froggers_v2::kNumHostPages;
        message.slot = 0;
        m_core->bus().push(message);
        m_core->processBus();
    };

    m_shift.onClick = [this]() { pushShift(m_shift.getToggleState()); };
    m_randAll.onClick = [this]() { pushRandAll(); };
    m_randMods.onClick = [this]() { pushRandMods(); };
    m_randWaveforms.onClick = [this]() {
        if (m_host)
        {
            m_host->RandomizeVcoMorphs();
        }
    };
    m_randResample.onClick = [this]() {
        if (m_host)
        {
            m_host->PressButton(0);
        }
    };

    for (juce::Component* c : {static_cast<juce::Component*>(&m_randAll),
                               static_cast<juce::Component*>(&m_randMods),
                               static_cast<juce::Component*>(&m_randWaveforms),
                               static_cast<juce::Component*>(&m_randResample),
                               static_cast<juce::Component*>(&m_crunchyLabel),
                               static_cast<juce::Component*>(&m_crunchyRing),
                               static_cast<juce::Component*>(&m_shift)})
    {
        addAndMakeVisible(c);
    }
}

void CenterGlobalClusterV2::bind(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core)
{
    m_host = host;
    m_core = core;
    refresh();
}

void CenterGlobalClusterV2::setShiftHeld(bool held)
{
    m_shift.setToggleState(held, juce::dontSendNotification);
    pushShift(held);
}

void CenterGlobalClusterV2::pushShift(bool held)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ShiftHeld;
    message.value = held ? 1.0f : 0.0f;
    m_core->bus().push(message);
    m_core->processBus();
}

void CenterGlobalClusterV2::pushRandAll()
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::RandAll;
    m_core->bus().push(message);
    m_core->processBus();
}

void CenterGlobalClusterV2::pushRandMods()
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::RandSequencerMods;
    message.page = resolveRandSeqScope != nullptr ? resolveRandSeqScope() : froggers_v2::kRandSeqScopeStep;
    m_core->bus().push(message);
    m_core->processBus();
}

void CenterGlobalClusterV2::refresh()
{
    if (!m_core)
    {
        return;
    }
    m_crunchyRing.refreshFromCrunchyState(m_core->uiState());
}

void CenterGlobalClusterV2::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds();
    const int btnH = kTextButtonH;
    const int gap = kSectionGap;
    int y = area.getY();

    struct ClusterButton
    {
        juce::Component* component;
        int height;
    };
    const ClusterButton stack[] = {
        {&m_randAll, btnH},
        {&m_randMods, btnH},
        {&m_randWaveforms, btnH},
        {&m_randResample, btnH},
        {&m_crunchyLabel, btnH},
        {&m_crunchyRing, kEncoderRingSize},
        {&m_shift, btnH},
    };

    for (const ClusterButton& entry : stack)
    {
        entry.component->setBounds(area.getX(), y, area.getWidth(), entry.height);
        y += entry.height + gap;
    }
}
