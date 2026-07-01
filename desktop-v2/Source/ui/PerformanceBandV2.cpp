#include "ui/PerformanceBandV2.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include "ModLedBrightness.hpp"

namespace
{
juce::Colour sceneLeftColour()
{
    return juce::Colour(0xff58a6ff);
}

juce::Colour sceneRightColour()
{
    return juce::Colour(0xffffa657);
}

juce::String sceneButtonLabel(uint8_t ordinal, uint8_t leftOrdinal, uint8_t rightOrdinal)
{
    juce::String label = "S" + juce::String(static_cast<int>(ordinal) + 1);
    if (ordinal == leftOrdinal)
    {
        return label + juce::String::fromUTF8("\xc2\xb7L");
    }
    if (ordinal == rightOrdinal)
    {
        return label + juce::String::fromUTF8("\xc2\xb7R");
    }
    return label;
}
} // namespace

PerformanceBandV2::PerformanceBandV2()
{
    m_sceneLabel.setText("Scene", juce::dontSendNotification);
    m_sceneLabel.setJustificationType(juce::Justification::centred);

    m_blendLabelL.setText("L", juce::dontSendNotification);
    m_blendLabelL.setJustificationType(juce::Justification::centred);
    m_blendLabelL.setColour(juce::Label::textColourId, sceneLeftColour());
    m_blendLabelR.setText("R", juce::dontSendNotification);
    m_blendLabelR.setJustificationType(juce::Justification::centred);
    m_blendLabelR.setColour(juce::Label::textColourId, sceneRightColour());

    m_bpmLabel.setText("BPM", juce::dontSendNotification);
    m_bpmLabel.setJustificationType(juce::Justification::centredRight);
    m_stepsLabel.setText("Steps", juce::dontSendNotification);
    m_stepsLabel.setJustificationType(juce::Justification::centredRight);

    m_sceneBlend.setSliderStyle(juce::Slider::LinearHorizontal);
    m_sceneBlend.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_sceneBlend.setRange(0.0, 1.0, 0.001);
    m_sceneBlend.onValueChange = [this]() {
        if (!m_core)
        {
            return;
        }
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::SceneBlend;
        message.value = static_cast<float>(m_sceneBlend.getValue());
        m_core->bus().push(message);
        m_core->processBus();
    };

    for (uint8_t lane = 0; lane < froggers_v2::kNumGestures; ++lane)
    {
        juce::ToggleButton* toggle = lane == 0 ? &m_gesture1 : &m_gesture2;
        juce::Slider* weight = lane == 0 ? &m_gestureWeight1 : &m_gestureWeight2;
        toggle->onClick = [this, lane]() { pushGesture(lane); };
        weight->setSliderStyle(juce::Slider::LinearHorizontal);
        weight->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        weight->setRange(0.0, 1.0, 0.001);
        weight->onValueChange = [this, lane, weight]() {
            pushGestureWeight(lane, static_cast<float>(weight->getValue()));
        };
    }

    m_scene1.onClick = [this]() { pushScene(0); };
    m_scene2.onClick = [this]() { pushScene(1); };
    m_scene3.onClick = [this]() { pushScene(2); };

    m_bpm.setSliderStyle(juce::Slider::LinearHorizontal);
    m_bpm.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 18);
    m_bpm.setRange(20.0, 300.0, 0.1);
    m_bpm.onValueChange = [this]() {
        if (m_sequencer)
        {
            m_sequencer->setBpm(static_cast<float>(m_bpm.getValue()));
        }
    };

    m_patternLength.setSliderStyle(juce::Slider::LinearHorizontal);
    m_patternLength.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 36, 18);
    m_patternLength.setRange(4.0, 64.0, 1.0);
    m_patternLength.onValueChange = [this]() {
        if (m_sequencer)
        {
            m_sequencer->setPatternLength(static_cast<uint8_t>(m_patternLength.getValue()));
        }
    };

    m_seqPlay.onClick = [this]() {
        if (!m_sequencer)
        {
            return;
        }
        m_sequencer->m_playing = !m_sequencer->m_playing;
        m_seqPlay.setButtonText(m_sequencer->m_playing ? "Stop Sequence" : "Start Sequence");
    };
    m_seqRecord.onClick = [this]() {
        if (m_sequencer)
        {
            m_sequencer->m_recordArm = m_seqRecord.getToggleState();
        }
    };

    for (juce::Label* label : {&m_marblesLabel1, &m_marblesLabel2})
    {
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(juce::FontOptions(9.0f)));
    }

    for (juce::Component* c :
         {static_cast<juce::Component*>(&m_sceneLabel),
          static_cast<juce::Component*>(&m_scene1),
          static_cast<juce::Component*>(&m_scene2),
          static_cast<juce::Component*>(&m_scene3),
          static_cast<juce::Component*>(&m_blendLabelL),
          static_cast<juce::Component*>(&m_sceneBlend),
          static_cast<juce::Component*>(&m_blendLabelR),
          static_cast<juce::Component*>(&m_gesture1),
          static_cast<juce::Component*>(&m_gestureWeight1),
          static_cast<juce::Component*>(&m_gesture2),
          static_cast<juce::Component*>(&m_gestureWeight2),
          static_cast<juce::Component*>(&m_seqPlay),
          static_cast<juce::Component*>(&m_seqRecord),
          static_cast<juce::Component*>(&m_bpmLabel),
          static_cast<juce::Component*>(&m_bpm),
          static_cast<juce::Component*>(&m_stepsLabel),
          static_cast<juce::Component*>(&m_patternLength),
          static_cast<juce::Component*>(&m_marblesLabel1),
          static_cast<juce::Component*>(&m_marblesLabel2)})
    {
        addAndMakeVisible(c);
    }
}

void PerformanceBandV2::bind(froggers_v2::FroggersV2ControlCore* core, SequencerState* sequencer)
{
    m_core = core;
    m_sequencer = sequencer;
    refresh();
}

void PerformanceBandV2::bindHost(DesktopHostIO* host)
{
    m_host = host;
}

void PerformanceBandV2::pushScene(uint8_t ordinal)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::SceneSelect;
    message.index = ordinal;
    m_core->bus().push(message);
    m_core->processBus();
}

void PerformanceBandV2::pushGesture(uint8_t lane)
{
    if (!m_core)
    {
        return;
    }
    const uint8_t current = m_core->uiState().activeGesture.load(std::memory_order_acquire);
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::GestureSelect;
    message.index = current == lane ? froggers_v2::kNoSelection : lane;
    m_core->bus().push(message);
    m_core->processBus();
}

void PerformanceBandV2::pushGestureWeight(uint8_t lane, float value)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::GestureWeight;
    message.index = lane;
    message.value = value;
    m_core->bus().push(message);
    m_core->processBus();
}

void PerformanceBandV2::updateSceneButtonLabels(uint8_t leftOrdinal, uint8_t rightOrdinal)
{
    m_scene1.setButtonText(sceneButtonLabel(0, leftOrdinal, rightOrdinal));
    m_scene2.setButtonText(sceneButtonLabel(1, leftOrdinal, rightOrdinal));
    m_scene3.setButtonText(sceneButtonLabel(2, leftOrdinal, rightOrdinal));
}

void PerformanceBandV2::refreshMarbles(bool audioRunning)
{
    m_audioRunning = audioRunning;
    if (!m_host)
    {
        repaint();
        return;
    }
    m_marblesLevel[0] = m_host->GetCvOut(13);
    m_marblesLevel[1] = m_host->GetCvOut(14);
    repaint();
}

void PerformanceBandV2::paintMarblesLed(juce::Graphics& g,
                                        juce::Rectangle<int> area,
                                        float level,
                                        bool active) const
{
    const float cx = static_cast<float>(area.getCentreX());
    const float cy = static_cast<float>(area.getCentreY());
    const float radius = static_cast<float>(DesktopV2ChromeLayout::gridPx(1));
    const float brightness = ModLedDisplayBrightness(level, active);
    g.setColour(juce::Colour(0xff21262d));
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    if (brightness > 0.0f)
    {
        g.setColour(juce::Colour(0xff3fb950).withAlpha(brightness));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }
}

void PerformanceBandV2::paint(juce::Graphics& g)
{
    const int ledSize = DesktopV2ChromeLayout::gridPx(2);
    for (int i = 0; i < 2; ++i)
    {
        auto ledArea = i == 0 ? m_marblesLabel1.getBounds() : m_marblesLabel2.getBounds();
        ledArea = ledArea.withY(ledArea.getBottom() + 1).withHeight(ledSize);
        paintMarblesLed(g, ledArea, m_marblesLevel[static_cast<size_t>(i)], m_audioRunning);
    }
}

void PerformanceBandV2::refresh()
{
    if (!m_core)
    {
        return;
    }
    const auto& state = m_core->uiState();
    m_sceneBlend.setValue(state.sceneBlend.load(std::memory_order_acquire), juce::dontSendNotification);
    const uint8_t leftOrdinal = state.leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightOrdinal = state.rightSceneOrdinal.load(std::memory_order_acquire);
    updateSceneButtonLabels(leftOrdinal, rightOrdinal);
    const uint8_t activeGesture = state.activeGesture.load(std::memory_order_acquire);
    m_gesture1.setToggleState(activeGesture == 0, juce::dontSendNotification);
    m_gesture2.setToggleState(activeGesture == 1, juce::dontSendNotification);
    m_gestureWeight1.setValue(m_core->gestureWeight(0), juce::dontSendNotification);
    m_gestureWeight2.setValue(m_core->gestureWeight(1), juce::dontSendNotification);
    if (m_sequencer)
    {
        m_bpm.setValue(m_sequencer->m_bpm, juce::dontSendNotification);
        m_patternLength.setValue(m_sequencer->m_patternLength, juce::dontSendNotification);
        m_seqRecord.setToggleState(m_sequencer->m_recordArm, juce::dontSendNotification);
        m_seqPlay.setButtonText(m_sequencer->m_playing ? "Stop Sequence" : "Start Sequence");
    }
}

void PerformanceBandV2::resized()
{
    auto area = getLocalBounds().reduced(DesktopV2ChromeLayout::kChromePad);
    const int y = area.getY();
    const int h = area.getHeight();
    int x = area.getX();
    const int gap = DesktopV2ChromeLayout::kSectionGap;

    m_sceneLabel.setBounds(x, y, DesktopV2ChromeLayout::kPerfSceneLabelW, h);
    x += DesktopV2ChromeLayout::kPerfSceneLabelW + gap;

    const int sceneSize = DesktopV2ChromeLayout::kPerfSceneButtonSize;
    const int sceneY = y + (h - sceneSize) / 2;
    m_scene1.setBounds(x, sceneY, sceneSize, sceneSize);
    x += sceneSize + gap;
    m_scene2.setBounds(x, sceneY, sceneSize, sceneSize);
    x += sceneSize + gap;
    m_scene3.setBounds(x, sceneY, sceneSize, sceneSize);
    x += sceneSize + gap;

    m_blendLabelL.setBounds(x, y, DesktopV2ChromeLayout::kPerfBlendEndpointLabelW, h);
    x += DesktopV2ChromeLayout::kPerfBlendEndpointLabelW;
    m_sceneBlend.setBounds(x, y, DesktopV2ChromeLayout::kPerfSceneBlendW, h);
    x += DesktopV2ChromeLayout::kPerfSceneBlendW;
    m_blendLabelR.setBounds(x, y, DesktopV2ChromeLayout::kPerfBlendEndpointLabelW, h);
    x += DesktopV2ChromeLayout::kPerfBlendEndpointLabelW + gap;

    m_gesture1.setBounds(x, y, DesktopV2ChromeLayout::kPerfGestureToggleW, h);
    x += DesktopV2ChromeLayout::kPerfGestureToggleW + gap;
    m_gestureWeight1.setBounds(x, y, DesktopV2ChromeLayout::kPerfGestureWeightW, h);
    x += DesktopV2ChromeLayout::kPerfGestureWeightW + gap;
    m_gesture2.setBounds(x, y, DesktopV2ChromeLayout::kPerfGestureToggleW, h);
    x += DesktopV2ChromeLayout::kPerfGestureToggleW + gap;
    m_gestureWeight2.setBounds(x, y, DesktopV2ChromeLayout::kPerfGestureWeightW, h);
    x += DesktopV2ChromeLayout::kPerfGestureWeightW + gap;

    m_seqPlay.setBounds(x, y, DesktopV2ChromeLayout::kPerfSeqTransportW, h);
    x += DesktopV2ChromeLayout::kPerfSeqTransportW + gap;
    m_seqRecord.setBounds(x, y, DesktopV2ChromeLayout::kPerfSeqRecordW, h);
    x += DesktopV2ChromeLayout::kPerfSeqRecordW + gap;

    m_bpmLabel.setBounds(x, y, DesktopV2ChromeLayout::kPerfBpmLabelW, h);
    x += DesktopV2ChromeLayout::kPerfBpmLabelW;
    m_bpm.setBounds(x, y, DesktopV2ChromeLayout::kPerfBpmSliderW, h);
    x += DesktopV2ChromeLayout::kPerfBpmSliderW + gap;

    m_stepsLabel.setBounds(x, y, DesktopV2ChromeLayout::kPerfStepsLabelW, h);
    x += DesktopV2ChromeLayout::kPerfStepsLabelW;
    m_patternLength.setBounds(x, y, DesktopV2ChromeLayout::kPerfStepsSliderW, h);
    x += DesktopV2ChromeLayout::kPerfStepsSliderW + gap;

    m_marblesLabel1.setBounds(
        x, y, DesktopV2ChromeLayout::kPerfMarblesColW, DesktopV2ChromeLayout::kPerfMarblesLabelH);
    m_marblesLabel2.setBounds(x + DesktopV2ChromeLayout::kPerfMarblesColW + gap,
                              y,
                              DesktopV2ChromeLayout::kPerfMarblesColW,
                              DesktopV2ChromeLayout::kPerfMarblesLabelH);
}
