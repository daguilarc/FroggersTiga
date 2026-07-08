#include "ui/SequencerPanelComponent.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include <memory>

namespace
{
constexpr juce::uint32 kGateOnColour = 0xff238636;
constexpr juce::uint32 kGateOnDimColour = 0xff1a3d22;
constexpr juce::uint32 kGateOffColour = 0xff30363d;
constexpr juce::uint32 kUnwrittenColour = 0xff21262d;
constexpr juce::uint32 kPlayheadColour = 0xff58a6ff;
constexpr juce::uint32 kEditStepColour = 0xffffa657;
constexpr juce::uint32 kCombinedHighlightColour = 0xff79c0ff;
constexpr int kDirectionButtonW = DesktopV2ChromeLayout::gridPx(4);
constexpr int kSpeedButtonW = DesktopV2ChromeLayout::gridPx(5);
constexpr int kLongPressMs = 500;

juce::Colour gateFillColour(bool written, bool gate, bool dimmed)
{
    if (!written)
    {
        return juce::Colour(kUnwrittenColour);
    }
    if (!gate)
    {
        return juce::Colour(kGateOffColour);
    }
    return juce::Colour(dimmed ? kGateOnDimColour : kGateOnColour);
}
} // namespace

const char* SequencerPanelComponent::directionLabel(uint8_t direction)
{
    if (direction < froggers_v2::manifest::kDirectionChoiceCount)
    {
        return froggers_v2::manifest::kSequencerDirectionChoices[direction];
    }
    return ">";
}

const char* SequencerPanelComponent::speedLabel(uint8_t speedChoice)
{
    if (speedChoice < froggers_v2::manifest::kSpeedChoiceCount)
    {
        return froggers_v2::manifest::kSequencerSpeedChoices[speedChoice];
    }
    return "1";
}

int SequencerPanelComponent::sequencerToolbarMinWidth()
{
    using namespace DesktopV2ChromeLayout;
    const int gap = kSectionGap;
    return (kPerfBpmLabelW + gap + kPerfBpmSliderW) + gap + (kDirectionButtonW + gap + kSpeedButtonW)
        + gap + kPerfSeqTransportW + gap + kPerfSeqRecordW + gap + kArrowButtonSize + gap + kArrowButtonSize
        + gap + kSequencerRandSeqLabelW + gap + kArrowButtonSize + gap + gridPx(9) + gap
        + kSequencerScopeAllStepsW;
}

void SequencerPanelComponent::layoutClockTransport(juce::Rectangle<int> row, int gap, int& x)
{
    using namespace DesktopV2ChromeLayout;
    const int y = row.getY();
    const int h = row.getHeight();
    m_bpmLabel.setBounds(x, y, kPerfBpmLabelW, h);
    x += kPerfBpmLabelW;
    m_bpm.setBounds(x, y, kPerfBpmSliderW, h);
    x += kPerfBpmSliderW + gap;
    m_directionBtn.setBounds(x, y, kDirectionButtonW, h);
    x += kDirectionButtonW + gap;
    m_speedBtn.setBounds(x, y, kSpeedButtonW, h);
    x += kSpeedButtonW + gap;
    m_seqPlay.setBounds(x, y, kPerfSeqTransportW, h);
    x += kPerfSeqTransportW + gap;
    m_writeSeq.setBounds(x, y, kPerfSeqRecordW, h);
    x += kPerfSeqRecordW + gap;
}

void SequencerPanelComponent::layoutNavRandScope(juce::Rectangle<int> row, int gap, int x)
{
    using namespace DesktopV2ChromeLayout;
    const int arrow = kArrowButtonSize;
    const int scopeH = kTextButtonH;
    const int scopeStepW = gridPx(9);
    const int scopeAllStepsW = kSequencerScopeAllStepsW;
    const int btnY = row.getCentreY() - arrow / 2;
    const int scopeY = row.getCentreY() - scopeH / 2;

    m_prevStep.setBounds(x, btnY, arrow, arrow);
    x += arrow + gap;
    m_nextStep.setBounds(x, btnY, arrow, arrow);
    x += arrow + gap;
    m_randSeqLabel.setBounds(x, row.getY(), kSequencerRandSeqLabelW, row.getHeight());
    x += kSequencerRandSeqLabelW + gap;
    m_dice.setBounds(x, btnY, arrow, arrow);

    m_scopePattern.setBounds(row.getRight() - scopeAllStepsW, scopeY, scopeAllStepsW, scopeH);
    m_scopeStep.setBounds(
        row.getRight() - scopeAllStepsW - gap - scopeStepW, scopeY, scopeStepW, scopeH);
}

SequencerPanelComponent::SequencerPanelComponent()
{
    m_bpmLabel.setText("BPM", juce::dontSendNotification);
    m_bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(m_bpmLabel);

    m_bpm.setSliderStyle(juce::Slider::LinearHorizontal);
    m_bpm.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 18);
    m_bpm.setRange(20.0, 300.0, 0.1);
    m_bpm.onValueChange = [this]() {
        if (m_sequencer)
        {
            m_sequencer->setBpm(static_cast<float>(m_bpm.getValue()));
        }
    };
    addAndMakeVisible(m_bpm);

    m_directionBtn.onClick = [this]() { cycleDirection(); };
    addAndMakeVisible(m_directionBtn);

    m_speedBtn.onClick = [this]() { cycleSpeed(); };
    addAndMakeVisible(m_speedBtn);

    m_seqPlay.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        const bool wasPlaying = m_sequencer->m_playing;
        m_sequencer->m_playing = !wasPlaying;
        if (!wasPlaying && m_sequencer->m_writeSeqArm && m_bridge != nullptr)
        {
            m_bridge->captureLiveToSequencerStep(m_sequencer->m_playhead);
            m_sequencer->m_writeSeqJustStarted = true;
        }
        if (m_sequencer->m_writeSeqArm)
        {
            m_sequencer->m_editStep = m_sequencer->m_playhead;
        }
        m_seqPlay.setButtonText(m_sequencer->m_playing ? "Stop Sequence" : "Start Sequence");
    };
    addAndMakeVisible(m_seqPlay);

    m_writeSeq.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        const bool arming = m_writeSeq.getToggleState();
        if (!arming && m_sequencer->m_writeSeqArm && !m_sequencer->m_playing && m_bridge != nullptr)
        {
            m_bridge->captureLiveToSequencerStep(m_sequencer->m_editStep);
        }
        m_sequencer->m_writeSeqArm = arming;
    };
    addAndMakeVisible(m_writeSeq);

    m_prevStep.setImages(makeChevron(true));
    m_prevStep.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        setEditStep(static_cast<int>(m_sequencer->wrappedEditStep(-1)));
    };
    addAndMakeVisible(m_prevStep);

    m_nextStep.setImages(makeChevron(false));
    m_nextStep.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        setEditStep(static_cast<int>(m_sequencer->wrappedEditStep(1)));
    };
    addAndMakeVisible(m_nextStep);

    m_randSeqLabel.setText("Rand-seq", juce::dontSendNotification);
    m_randSeqLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_randSeqLabel);

    m_dice.setImages(makeDiceFace());
    m_dice.setTooltip("Randomize sequencer steps (scene slots)");
    m_dice.onClick = [this]() { pushDiceRand(); };
    addAndMakeVisible(m_dice);

    m_scopeStep.setRadioGroupId(9001);
    m_scopeStep.setClickingTogglesState(false);
    m_scopeStep.setToggleState(true, juce::dontSendNotification);
    m_scopeStep.onClick = [this]() {
        m_randAllSteps = false;
        m_scopeStep.setToggleState(true, juce::dontSendNotification);
        m_scopePattern.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(m_scopeStep);

    m_scopePattern.setRadioGroupId(9001);
    m_scopePattern.setClickingTogglesState(false);
    m_scopePattern.onClick = [this]() {
        m_randAllSteps = true;
        m_scopePattern.setToggleState(true, juce::dontSendNotification);
        m_scopeStep.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(m_scopePattern);

    for (int i = 0; i < SequencerState::kSlotCount; ++i)
    {
        StepCell& cell = m_steps[static_cast<size_t>(i)];
        cell.stepIndex = i;
        cell.setButtonText(juce::String(i + 1));
        addAndMakeVisible(cell);

        cell.onStepClick = [this](int step) {
            if (m_longPressFired)
            {
                m_longPressFired = false;
                return;
            }
            setEditStep(step);
        };
        cell.onStepDoubleClick = [this](int step) { toggleStepGate(step); };
        cell.onStepMouseDown = [this](int step, const juce::MouseEvent& event) {
            if (event.mods.isPopupMenu())
            {
                setEditStep(step);
                showStepContextMenu(step);
                return;
            }
            beginLongPress(step);
        };
    }
}

void SequencerPanelComponent::timerCallback()
{
    stopTimer();
    m_longPressFired = true;
    clearStep(m_longPressStep);
}

void SequencerPanelComponent::beginLongPress(int step)
{
    m_longPressStep = step;
    m_longPressFired = false;
    startTimer(kLongPressMs);
}

void SequencerPanelComponent::cycleDirection()
{
    if (!m_sequencer)
    {
        return;
    }
    const uint8_t next =
        static_cast<uint8_t>((m_sequencer->m_direction + 1u) % SequencerState::kDirectionChoiceCount);
    m_sequencer->setDirection(next);
    refresh();
}

void SequencerPanelComponent::cycleSpeed()
{
    if (!m_sequencer)
    {
        return;
    }
    const uint8_t next =
        static_cast<uint8_t>((m_sequencer->m_speedChoice + 1u) % SequencerState::kSpeedChoiceCount);
    m_sequencer->setSpeedChoice(next);
    refresh();
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
    if (m_bridge != nullptr)
    {
        m_bridge->setOnStepCaptured([this](uint8_t step) { triggerCaptureFlash(static_cast<int>(step)); });
    }
    refresh();
}

void SequencerPanelComponent::setEditStep(int step)
{
    if (!m_sequencer || step < 0 || step >= SequencerState::kSlotCount)
    {
        return;
    }
    if (static_cast<int>(m_sequencer->m_editStep) == step)
    {
        return;
    }
    const uint8_t oldEdit = m_sequencer->m_editStep;
    if (!m_sequencer->m_playing && m_sequencer->m_writeSeqArm && m_bridge != nullptr)
    {
        m_bridge->captureLiveToSequencerStep(oldEdit);
    }
    m_sequencer->m_editStep = static_cast<uint8_t>(step);
    if (!m_sequencer->m_playing && m_bridge != nullptr)
    {
        m_bridge->recallSequencerStep(static_cast<uint8_t>(step));
    }
    refresh();
}

void SequencerPanelComponent::toggleStepGate(int step)
{
    if (!m_sequencer || step < 0 || step >= SequencerState::kSlotCount)
    {
        return;
    }
    m_sequencer->m_editStep = static_cast<uint8_t>(step);
    auto& slot = m_sequencer->m_slots[static_cast<size_t>(step)];
    if (!slot.written)
    {
        slot.written = true;
    }
    slot.payload.gate = !slot.payload.gate;
    refresh();
}

void SequencerPanelComponent::clearStep(int step)
{
    if (!m_sequencer || step < 0 || step >= SequencerState::kSlotCount)
    {
        return;
    }
    m_sequencer->clearStep(static_cast<uint8_t>(step));
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

void SequencerPanelComponent::triggerCaptureFlash(int step)
{
    m_captureFlashStep = step;
    m_captureFlashMs = juce::Time::getMillisecondCounter();
}

uint8_t SequencerPanelComponent::getRandSeqScope() const
{
    return m_randAllSteps ? froggers_v2::kRandSeqScopePattern : froggers_v2::kRandSeqScopeStep;
}

juce::Rectangle<int> SequencerPanelComponent::getStepBounds(int stepIndex) const
{
    if (stepIndex < 0 || stepIndex >= SequencerState::kSlotCount)
    {
        return {};
    }
    return m_steps[static_cast<size_t>(stepIndex)].getBounds();
}

juce::Rectangle<int> SequencerPanelComponent::directionButtonBounds() const
{
    return m_directionBtn.getBounds();
}

juce::Rectangle<int> SequencerPanelComponent::speedButtonBounds() const
{
    return m_speedBtn.getBounds();
}

void SequencerPanelComponent::pushDiceRand()
{
    const uint8_t scope =
        m_randAllSteps ? froggers_v2::kRandSeqScopePattern : froggers_v2::kRandSeqScopeStep;
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
    m_bpm.setValue(m_sequencer->m_bpm, juce::dontSendNotification);
    m_directionBtn.setButtonText(directionLabel(m_sequencer->m_direction));
    m_speedBtn.setButtonText(speedLabel(m_sequencer->m_speedChoice));
    m_writeSeq.setToggleState(m_sequencer->m_writeSeqArm, juce::dontSendNotification);
    m_seqPlay.setButtonText(m_sequencer->m_playing ? "Stop Sequence" : "Start Sequence");

    const bool dimGates = !m_sequencer->m_playing;
    const juce::uint32 now = juce::Time::getMillisecondCounter();
    const bool flashVisible =
        m_captureFlashStep >= 0 && (now - m_captureFlashMs) < 400u;
    for (int i = 0; i < SequencerState::kSlotCount; ++i)
    {
        const auto& slot = m_sequencer->m_slots[static_cast<size_t>(i)];
        m_steps[static_cast<size_t>(i)].setCaptureFlash(flashVisible && i == m_captureFlashStep);
        const bool gate = slot.payload.gate;
        const bool playhead = static_cast<int>(m_sequencer->m_playhead) == i;
        const bool editStep = static_cast<int>(m_sequencer->m_editStep) == i;
        m_steps[static_cast<size_t>(i)].setColour(
            juce::TextButton::buttonColourId,
            gateFillColour(slot.written, gate, dimGates));
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
    const int gap = kSectionGap;
    const bool twoRows = sequencerToolbarMinWidth() > area.getWidth();
    const int toolbarTotalH = twoRows ? kSequencerToolbarH * 2 : kSequencerToolbarH;
    auto toolbar = area.removeFromTop(toolbarTotalH);

    if (twoRows)
    {
        auto row1 = toolbar.removeFromTop(kSequencerToolbarH);
        int x1 = row1.getX();
        layoutClockTransport(row1, gap, x1);
        layoutNavRandScope(toolbar, gap, toolbar.getX());
    }
    else
    {
        int x = toolbar.getX();
        layoutClockTransport(toolbar, gap, x);
        layoutNavRandScope(toolbar, gap, x);
    }

    constexpr int cols = SequencerState::kSlotCount;
    const int rows = 1;
    const int cellSize = kSequencerStepCellSize;
    const int gridW = cols * cellSize;
    const int gridH = rows * cellSize;
    const int gridX = area.getX() + (area.getWidth() - gridW) / 2;
    const int gridY = area.getY() + DesktopV2ChromeLayout::kChromePad;

    for (int i = 0; i < SequencerState::kSlotCount; ++i)
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
