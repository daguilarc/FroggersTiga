#include "ui/PerformanceBandV2.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include "ModLedBrightness.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

namespace
{
// Lanes of the permanent modulation source catalog (single authority:
// froggers_v2::manifest::kPermanentModulationSources) that back the marbles
// indicators. Shared with the CV readback in refreshMarbles() below so the
// lane numbers are declared exactly once.
constexpr uint8_t kMarblesLane1 = 11;
constexpr uint8_t kMarblesLane2 = 12;

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

// Shared projection helper: slash-namespaced inventory labels
// (e.g. "Global/Gesture1") show the tail; names without a slash
// (e.g. "Random S&H 1") show the full operator displayName.
juce::String labelTailAfterSlash(const char* fullName)
{
    const juce::String full(fullName);
    const int slash = full.lastIndexOfChar('/');
    return slash >= 0 ? full.substring(slash + 1) : full;
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

    m_scene1.onClick = [this]() { pushScene(0); };
    m_scene2.onClick = [this]() { pushScene(1); };
    m_scene3.onClick = [this]() { pushScene(2); };

    const uint8_t marblesLanes[2] = {kMarblesLane1, kMarblesLane2};
    juce::Label* const marblesLabels[2] = {&m_marblesLabel1, &m_marblesLabel2};
    for (size_t i = 0; i < 2; ++i)
    {
        juce::Label* label = marblesLabels[i];
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(juce::FontOptions(9.0f)));
        const char* fullName = froggers_v2::manifest::kPermanentModulationSources[marblesLanes[i]].displayName;
        label->setText(labelTailAfterSlash(fullName), juce::dontSendNotification);
        label->setTooltip(juce::String(fullName));
    }

    for (juce::Component* c :
         {static_cast<juce::Component*>(&m_sceneLabel),
          static_cast<juce::Component*>(&m_scene1),
          static_cast<juce::Component*>(&m_scene2),
          static_cast<juce::Component*>(&m_scene3),
          static_cast<juce::Component*>(&m_blendLabelL),
          static_cast<juce::Component*>(&m_sceneBlend),
          static_cast<juce::Component*>(&m_blendLabelR),
          static_cast<juce::Component*>(&m_marblesLabel1),
          static_cast<juce::Component*>(&m_marblesLabel2)})
    {
        addAndMakeVisible(c);
    }
}

void PerformanceBandV2::bind(froggers_v2::FroggersV2ControlCore* core)
{
    m_core = core;
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

void PerformanceBandV2::updateSceneButtonLabels(uint8_t leftOrdinal, uint8_t rightOrdinal)
{
    m_scene1.setButtonText(sceneButtonLabel(0, leftOrdinal, rightOrdinal));
    m_scene2.setButtonText(sceneButtonLabel(1, leftOrdinal, rightOrdinal));
    m_scene3.setButtonText(sceneButtonLabel(2, leftOrdinal, rightOrdinal));

    // Scene-blend endpoint labels are tied to the live S1/S2/S3 ordinals so
    // an operator can see which scenes the blend slider interpolates
    // between, rather than the anonymous "L"/"R" endpoint markers.
    const juce::String leftLabel = "S" + juce::String(static_cast<int>(leftOrdinal) + 1);
    const juce::String rightLabel = "S" + juce::String(static_cast<int>(rightOrdinal) + 1);
    m_blendLabelL.setText(leftLabel, juce::dontSendNotification);
    m_blendLabelR.setText(rightLabel, juce::dontSendNotification);
}

void PerformanceBandV2::refreshMarbles(bool audioRunning)
{
    m_audioRunning = audioRunning;
    if (!m_host)
    {
        repaint();
        return;
    }
    m_marblesLevel[0] = m_host->GetCvOut(kMarblesLane1);
    m_marblesLevel[1] = m_host->GetCvOut(kMarblesLane2);
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
}

void PerformanceBandV2::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds().reduced(kChromePad);
    const int y = area.getY();
    const int h = area.getHeight();
    const int gap = kSectionGap;

    // Packet 17 (D14, re-derived after §13.0.1a gesture removal): fixed
    // minima for labeled controls; leftover width goes into the single
    // flexible scene-blend slider so nothing overlaps and no empty gap sits
    // past the marbles columns.
    const int sceneLabelW = kPerfSceneLabelW;
    const int sceneW = kSceneButtonMinWidth;
    const int blendEndW = kPerfBlendEndpointLabelW;
    const int marblesW = kPerfMarblesColW;
    const int blendMinW = kPerfSceneBlendW;
    constexpr int kGapCount = 6;
    const int fixedW = sceneLabelW + 3 * sceneW + 2 * blendEndW + 2 * marblesW;
    const int flexibleMinW = blendMinW;
    const int extra = juce::jmax(0, area.getWidth() - fixedW - flexibleMinW - kGapCount * gap);
    const int blendW = blendMinW + extra;

    int x = area.getX();
    m_sceneLabel.setBounds(x, y, sceneLabelW, h);
    x += sceneLabelW + gap;

    const int sceneH = kTextButtonH;
    const int sceneY = y + (h - sceneH) / 2;
    juce::TextButton* const scenes[] = {&m_scene1, &m_scene2, &m_scene3};
    for (juce::TextButton* scene : scenes)
    {
        scene->setBounds(x, sceneY, sceneW, sceneH);
        x += sceneW + gap;
    }

    m_blendLabelL.setBounds(x, y, blendEndW, h);
    x += blendEndW;
    m_sceneBlend.setBounds(x, y, blendW, h);
    x += blendW;
    m_blendLabelR.setBounds(x, y, blendEndW, h);
    x += blendEndW + gap;

    const int marblesLabelY = y + (h - kPerfMarblesLabelH) / 2;
    m_marblesLabel1.setBounds(x, marblesLabelY, marblesW, kPerfMarblesLabelH);
    x += marblesW + gap;
    m_marblesLabel2.setBounds(x, marblesLabelY, marblesW, kPerfMarblesLabelH);
}
