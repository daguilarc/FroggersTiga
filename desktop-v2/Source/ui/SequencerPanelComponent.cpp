#include "ui/SequencerPanelComponent.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include <memory>

namespace
{
constexpr juce::uint32 kGateOnColour = 0xff238636;
constexpr juce::uint32 kGateOnDimColour = 0xff1a3d22;
constexpr juce::uint32 kGateOffColour = 0xff30363d;
constexpr juce::uint32 kPlayheadColour = 0xff58a6ff;
constexpr juce::uint32 kEditStepColour = 0xffffa657;
constexpr juce::uint32 kCombinedHighlightColour = 0xff79c0ff;

juce::Colour gateFillColour(bool gate, bool dimmed)
{
    if (!gate)
    {
        return juce::Colour(kGateOffColour);
    }
    return juce::Colour(dimmed ? kGateOnDimColour : kGateOnColour);
}
} // namespace

SequencerPanelComponent::SequencerPanelComponent()
{
    m_title.setText("Sequencer", juce::dontSendNotification);
    m_title.setJustificationType(juce::Justification::centredLeft);
    m_title.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    addAndMakeVisible(m_title);

    m_prevStep.setImages(makeChevron(true));
    m_prevStep.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        m_sequencer->prevEditStep();
        refresh();
    };
    addAndMakeVisible(m_prevStep);

    m_nextStep.setImages(makeChevron(false));
    m_nextStep.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        m_sequencer->nextEditStep();
        refresh();
    };
    addAndMakeVisible(m_nextStep);

    m_dice.setImages(makeDiceFace());
    m_dice.onClick = [this]() { pushDiceRand(); };
    addAndMakeVisible(m_dice);

    m_scopeStep.setRadioGroupId(9001);
    m_scopeStep.setToggleState(true, juce::dontSendNotification);
    m_scopeStep.onClick = [this]() {
        m_patternScope = false;
        m_scopeStep.setToggleState(true, juce::dontSendNotification);
        m_scopePattern.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(m_scopeStep);

    m_scopePattern.setRadioGroupId(9001);
    m_scopePattern.onClick = [this]() {
        m_patternScope = true;
        m_scopePattern.setToggleState(true, juce::dontSendNotification);
        m_scopeStep.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(m_scopePattern);

    for (int i = 0; i < SequencerState::kMaxSteps; ++i)
    {
        StepCell& cell = m_steps[static_cast<size_t>(i)];
        cell.stepIndex = i;
        cell.setButtonText(juce::String(i + 1));
        cell.setVisible(i < 16);
        addAndMakeVisible(cell);

        cell.onStepClick = [this](int step) { setEditStep(step); };
        cell.onStepDoubleClick = [this](int step) { toggleStepGate(step); };
        cell.onStepMouseDown = [this](int step, const juce::MouseEvent& event) {
            if (!event.mods.isPopupMenu())
            {
                return;
            }
            setEditStep(step);
            showStepContextMenu(step);
        };
    }
}

void SequencerPanelComponent::bind(SequencerState* sequencer,
                                   froggers_v2::FroggersV2ControlCore* core,
                                   froggers_v2::FroggersV2HostBridge* bridge)
{
    m_sequencer = sequencer;
    m_core = core;
    m_bridge = bridge;
    if (m_core != nullptr)
    {
        m_core->setSequencerState(sequencer);
    }
    refresh();
}

void SequencerPanelComponent::setEditStep(int step)
{
    if (!m_sequencer || step < 0 || step >= m_sequencer->m_patternLength)
    {
        return;
    }
    m_sequencer->m_editStep = static_cast<uint8_t>(step);
    refresh();
}

void SequencerPanelComponent::toggleStepGate(int step)
{
    if (!m_sequencer || step < 0 || step >= SequencerState::kMaxSteps)
    {
        return;
    }
    m_sequencer->m_editStep = static_cast<uint8_t>(step);
    m_sequencer->m_steps[static_cast<size_t>(step)].gate =
        !m_sequencer->m_steps[static_cast<size_t>(step)].gate;
    refresh();
}

void SequencerPanelComponent::pushResetStep(int step)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ResetSequencerStep;
    message.slot = static_cast<uint8_t>(step);
    m_core->bus().push(message);
    m_core->processBus();
    refresh();
}

void SequencerPanelComponent::pushRandSequencerStep(int step, uint8_t scope)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::RandSequencerStep;
    message.slot = static_cast<uint8_t>(step);
    message.page = scope;
    m_core->bus().push(message);
    m_core->processBus();
    if (scope != froggers_v2::kRandSeqScopeFullStep && m_bridge != nullptr)
    {
        m_bridge->syncToHost();
    }
    refresh();
}

void SequencerPanelComponent::pushDiceRand()
{
    const uint8_t scope =
        m_patternScope ? froggers_v2::kRandSeqScopePattern : froggers_v2::kRandSeqScopeStep;
    pushRandSequencerStep(m_sequencer != nullptr ? m_sequencer->m_editStep : 0, scope);
}

void SequencerPanelComponent::showStepContextMenu(int step)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Reset");
    menu.addItem(2, "Randomize");
    menu.showMenuAsync(
        juce::PopupMenu::Options(),
        [this, step](int result) {
            if (result == 1)
            {
                pushResetStep(step);
                return;
            }
            if (result == 2)
            {
                pushRandSequencerStep(step, froggers_v2::kRandSeqScopeFullStep);
            }
        });
}

void SequencerPanelComponent::refresh()
{
    if (!m_sequencer)
    {
        return;
    }
    const bool dimGates = !m_sequencer->m_playing;
    for (int i = 0; i < SequencerState::kMaxSteps; ++i)
    {
        const bool inPattern = i < m_sequencer->m_patternLength;
        m_steps[static_cast<size_t>(i)].setVisible(inPattern);
        if (!inPattern)
        {
            continue;
        }
        const bool gate = m_sequencer->m_steps[static_cast<size_t>(i)].gate;
        const bool playhead = static_cast<int>(m_sequencer->m_playhead) == i;
        const bool editStep = static_cast<int>(m_sequencer->m_editStep) == i;
        m_steps[static_cast<size_t>(i)].setColour(
            juce::TextButton::buttonColourId,
            gateFillColour(gate, dimGates));
        juce::Colour outline = juce::Colour(0xff484f58);
        if (playhead && editStep)
        {
            outline = juce::Colour(kCombinedHighlightColour);
        }
        else if (playhead)
        {
            outline = juce::Colour(kPlayheadColour);
        }
        else if (editStep)
        {
            outline = juce::Colour(kEditStepColour);
        }
        m_steps[static_cast<size_t>(i)].setColour(juce::TextButton::buttonOnColourId, outline);
        m_steps[static_cast<size_t>(i)].setToggleState(playhead || editStep, juce::dontSendNotification);
    }
    repaint();
}

void SequencerPanelComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff3d444d));
    g.drawRect(getLocalBounds(), 1);
}

void SequencerPanelComponent::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds();
    auto toolbar = area.removeFromTop(kSequencerToolbarH);
    const int arrow = kArrowButtonSize;
    const int gap = kSectionGap;
    const int btnY = toolbar.getCentreY() - arrow / 2;

    int x = toolbar.getX();
    m_prevStep.setBounds(x, btnY, arrow, arrow);
    x += arrow + gap;
    m_nextStep.setBounds(x, btnY, arrow, arrow);
    x += arrow + gap;
    m_dice.setBounds(x, btnY, arrow, arrow);

    const int scopeH = kTextButtonH;
    const int scopeY = toolbar.getCentreY() - scopeH / 2;
    m_scopePattern.setBounds(
        toolbar.getRight() - gridPx(12), scopeY, gridPx(12), scopeH);
    m_scopeStep.setBounds(
        toolbar.getRight() - gridPx(12) - gap - gridPx(9), scopeY, gridPx(9), scopeH);

    const int length = m_sequencer ? m_sequencer->m_patternLength : 16;
    constexpr int cols = 16;
    const int rows = (length + cols - 1) / cols;
    const int cellSize = kSequencerStepCellSize;
    const int gridW = cols * cellSize;
    const int gridH = rows * cellSize;
    const int gridX = area.getX() + (area.getWidth() - gridW) / 2;
    const int gridY = area.getY() + (area.getHeight() - gridH) / 2;

    for (int i = 0; i < length; ++i)
    {
        const int row = i / cols;
        const int col = i % cols;
        m_steps[static_cast<size_t>(i)].setBounds(
            gridX + col * cellSize,
            gridY + row * cellSize,
            cellSize,
            cellSize);
    }
}

juce::Drawable* SequencerPanelComponent::makeChevron(bool left)
{
    juce::Path path;
    if (left)
    {
        path.startNewSubPath(14.0f, 4.0f);
        path.lineTo(6.0f, 12.0f);
        path.lineTo(14.0f, 20.0f);
    }
    else
    {
        path.startNewSubPath(6.0f, 4.0f);
        path.lineTo(14.0f, 12.0f);
        path.lineTo(6.0f, 20.0f);
    }
    auto drawable = std::make_unique<juce::DrawablePath>();
    drawable->setPath(path);
    drawable->setStrokeType(juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawable->setStrokeFill(juce::Colours::white);
    return drawable.release();
}

juce::Drawable* SequencerPanelComponent::makeDiceFace()
{
    juce::Path path;
    path.addRoundedRectangle(3.0f, 3.0f, 14.0f, 14.0f, 2.0f);
    const float dots[] = {6.0f, 6.0f, 14.0f, 14.0f, 10.0f, 10.0f, 6.0f, 14.0f, 14.0f, 6.0f};
    for (size_t i = 0; i < 5; ++i)
    {
        path.addEllipse(dots[i * 2], dots[i * 2 + 1], 2.0f, 2.0f);
    }
    auto drawable = std::make_unique<juce::DrawablePath>();
    drawable->setPath(path);
    drawable->setFill(juce::Colours::white);
    return drawable.release();
}
