#include "ui/GlobalStripV2.hpp"

#include "ParamDisplayNames.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

GlobalStripV2::GlobalStripV2()
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
    m_randMods.onClick = [this]() {
        if (m_host)
        {
            m_host->EnqueueRandomizeAllMod();
        }
    };
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

    m_lfo.setVisible(false);
    m_vco.setVisible(false);

    for (juce::Component* c : {static_cast<juce::Component*>(&m_randAll),
                               static_cast<juce::Component*>(&m_randMods),
                               static_cast<juce::Component*>(&m_randWaveforms),
                               static_cast<juce::Component*>(&m_randResample),
                               static_cast<juce::Component*>(&m_crunchyLabel),
                               static_cast<juce::Component*>(&m_crunchyRing),
                               static_cast<juce::Component*>(&m_shift),
                               static_cast<juce::Component*>(&m_lfo),
                               static_cast<juce::Component*>(&m_vco)})
    {
        addAndMakeVisible(c);
    }
}

void GlobalStripV2::bind(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core)
{
    m_host = host;
    m_core = core;
    refresh();
}

void GlobalStripV2::setShiftHeld(bool held)
{
    m_shift.setToggleState(held, juce::dontSendNotification);
    pushShift(held);
}

void GlobalStripV2::pushShift(bool held)
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

void GlobalStripV2::pushRandAll()
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

void GlobalStripV2::refresh()
{
    if (!m_core)
    {
        return;
    }
    m_crunchyRing.refreshFromCrunchyState(m_core->uiState());
}

void GlobalStripV2::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds();
    const int btnH = kTextButtonH;
    const int btnY = area.getCentreY() - btnH / 2;
    int x = area.getX();

    struct StripButton
    {
        juce::TextButton* button;
        int width;
    };
    const StripButton leftButtons[] = {
        {&m_randAll, kGlobalStripRandAllW},
        {&m_randMods, kGlobalStripRandModsW},
        {&m_randWaveforms, kGlobalStripRandWaveformsW},
        {&m_randResample, kGlobalStripRandResampleW},
    };
    for (const StripButton& entry : leftButtons)
    {
        entry.button->setBounds(x, btnY, entry.width, btnH);
        x += entry.width + kSectionGap;
    }

    m_crunchyLabel.setBounds(x, btnY, kGlobalStripCrunchyLabelW, btnH);
    x += kGlobalStripCrunchyLabelW + kSectionGap;

    const int ringSide = kEncoderRingSize;
    const int ringY = area.getCentreY() - ringSide / 2;
    m_crunchyRing.setBounds(x, ringY, ringSide, ringSide);
    x += ringSide + kSectionGap;

    m_shift.setBounds(x, btnY, kGlobalStripShiftW, btnH);
}
