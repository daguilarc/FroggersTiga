#include "ui/GlobalStripV2.hpp"

#include "ParamDisplayNames.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

namespace
{
void wireScopePair(juce::ToggleButton& allChoice,
                   juce::ToggleButton& currentChoice,
                   int radioGroupId,
                   bool& allScopeFlag)
{
    allChoice.setRadioGroupId(radioGroupId);
    currentChoice.setRadioGroupId(radioGroupId);
    allChoice.setClickingTogglesState(false);
    currentChoice.setClickingTogglesState(false);
    allChoice.onClick = [&allChoice, &currentChoice, &allScopeFlag]() {
        allScopeFlag = true;
        allChoice.setToggleState(true, juce::dontSendNotification);
        currentChoice.setToggleState(false, juce::dontSendNotification);
    };
    currentChoice.onClick = [&allChoice, &currentChoice, &allScopeFlag]() {
        allScopeFlag = false;
        allChoice.setToggleState(false, juce::dontSendNotification);
        currentChoice.setToggleState(true, juce::dontSendNotification);
    };
}
} // namespace

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

    wireScopePair(m_scopeAllScenes, m_scopeCurrentScene, 9101, m_allScenesScope);
    wireScopePair(m_scopeAllSteps, m_scopeCurrentStep, 9102, m_allStepsScope);

    m_scopeAllScenes.setToggleState(true, juce::dontSendNotification);
    m_scopeCurrentScene.setToggleState(false, juce::dontSendNotification);
    m_scopeAllSteps.setToggleState(false, juce::dontSendNotification);
    m_scopeCurrentStep.setToggleState(true, juce::dontSendNotification);

    for (juce::Component* c : {static_cast<juce::Component*>(&m_randAll),
                               static_cast<juce::Component*>(&m_randMods),
                               static_cast<juce::Component*>(&m_randWaveforms),
                               static_cast<juce::Component*>(&m_randResample),
                               static_cast<juce::Component*>(&m_crunchyLabel),
                               static_cast<juce::Component*>(&m_crunchyRing),
                               static_cast<juce::Component*>(&m_shift),
                               static_cast<juce::Component*>(&m_scopeAllScenes),
                               static_cast<juce::Component*>(&m_scopeCurrentScene),
                               static_cast<juce::Component*>(&m_scopeAllSteps),
                               static_cast<juce::Component*>(&m_scopeCurrentStep)})
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
    const uint8_t sceneScope = m_allScenesScope ? froggers_v2::kRandSceneScopeAll
                                                 : froggers_v2::kRandSceneScopeCurrent;
    m_core->executeRandomization(
        froggers_v2::FroggersV2ControlCore::RandomizationCommand::RandAll, sceneScope, 0);
}

void GlobalStripV2::pushRandMods()
{
    if (!m_core)
    {
        return;
    }
    uint8_t stepScope = froggers_v2::kRandSeqScopeStep;
    if (m_allStepsScope)
    {
        stepScope = froggers_v2::kRandSeqScopePattern;
    }
    else if (resolveRandSeqScope != nullptr)
    {
        stepScope = resolveRandSeqScope();
    }
    m_core->executeRandomization(
        froggers_v2::FroggersV2ControlCore::RandomizationCommand::RandMods, 0, stepScope);
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
    const int gap = kSectionGap;
    const int btnH = kTextButtonH;
    const int scopeH = kTextButtonH;
    auto commandRow = area.removeFromTop(btnH);
    auto scopeRow = area.removeFromTop(scopeH);

    int x = commandRow.getX();
    const int btnY = commandRow.getY();

    struct StripButton
    {
        juce::Component* component;
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
        entry.component->setBounds(x, btnY, entry.width, btnH);
        x += entry.width + gap;
    }

    m_crunchyLabel.setBounds(x, btnY, kGlobalStripCrunchyLabelW, btnH);
    x += kGlobalStripCrunchyLabelW + gap;

    const int ringSide = kEncoderRingSize;
    const int ringY = commandRow.getCentreY() - ringSide / 2;
    m_crunchyRing.setBounds(x, ringY, ringSide, ringSide);
    x += ringSide + gap;
    m_shift.setBounds(x, btnY, kGlobalStripShiftW, btnH);

    int sceneX = commandRow.getX();
    m_scopeAllScenes.setBounds(sceneX, scopeRow.getY(), kGlobalScopeSceneAllW, scopeH);
    sceneX += kGlobalScopeSceneAllW + gap;
    m_scopeCurrentScene.setBounds(sceneX, scopeRow.getY(), kGlobalScopeSceneCurrentW, scopeH);

    int stepX = commandRow.getX() + kGlobalStripRandAllW + gap;
    m_scopeAllSteps.setBounds(stepX, scopeRow.getY(), kGlobalScopeStepAllW, scopeH);
    stepX += kGlobalScopeStepAllW + gap;
    m_scopeCurrentStep.setBounds(stepX, scopeRow.getY(), kGlobalScopeStepCurrentW, scopeH);
}
