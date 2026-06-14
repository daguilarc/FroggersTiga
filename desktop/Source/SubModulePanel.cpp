#include "SubModulePanel.h"

#include "AudioPairArLayout.hpp"
#include "ParamDisplayNames.hpp"

namespace
{
constexpr int kWaveWidth = 28;
constexpr int kKnobSize = 38;
constexpr int kJackSize = 20;
constexpr int kTailGap = 2;
constexpr int kRowHeight = 36;
constexpr int kLabelRowH = 14;
constexpr juce::uint32 kModuleBorder = 0xff3d444d;

int fixedTailWidth(bool hasWave)
{
    const int knobJack = kKnobSize + kJackSize + kTailGap;
    return hasWave ? (kWaveWidth + kTailGap + knobJack) : knobJack;
}

void layoutControlCluster(juce::Rectangle<int>& area,
                          bool hasWave,
                          juce::Slider& knob,
                          WaveMorphButton* waveButton,
                          juce::Rectangle<int>& jackBounds)
{
    const int tailW = fixedTailWidth(hasWave);
    auto tail = area.removeFromRight(tailW);
    jackBounds = tail.removeFromRight(kJackSize).withSizeKeepingCentre(kJackSize, kJackSize);
    tail.removeFromRight(kTailGap);
    knob.setBounds(tail.removeFromRight(kKnobSize).withSizeKeepingCentre(kKnobSize, kKnobSize));
    if (hasWave && waveButton != nullptr)
    {
        tail.removeFromRight(kTailGap);
        waveButton->setBounds(tail.removeFromRight(kWaveWidth).reduced(1));
    }
}
} // namespace

SubModulePanel::SubModulePanel(uint8_t pageIndex, juce::String title, IPanelBackend& backend)
    : m_pageIndex(pageIndex)
    , m_backend(backend)
{
    m_title.setText(title, juce::dontSendNotification);
    m_title.setJustificationType(juce::Justification::centred);
    m_title.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    addAndMakeVisible(m_title);

    m_randomize.onClick = [this]() { m_backend.randomizeKnobs(); };
    addAndMakeVisible(m_randomize);

    m_randomizeMod.onClick = [this]() { m_backend.randomizeMod(); };
    addAndMakeVisible(m_randomizeMod);

    for (int i = 0; i < 7; i++)
    {
        m_paramLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centredLeft);
        m_paramLabels[static_cast<size_t>(i)].setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(m_paramLabels[static_cast<size_t>(i)]);

        const int row = i;
        m_sliders[static_cast<size_t>(i)].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        m_sliders[static_cast<size_t>(i)].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_sliders[static_cast<size_t>(i)].setRange(0.0, 1.0, 0.001);
        m_sliders[static_cast<size_t>(i)].setValue(0.5);
        m_sliders[static_cast<size_t>(i)].onValueChange = [this, row]() {
            const uint8_t mod = m_backend.getModSource(static_cast<uint8_t>(row));
            const float v = static_cast<float>(m_sliders[static_cast<size_t>(row)].getValue());
            if (mod == 255)
            {
                m_backend.setKnob(static_cast<uint8_t>(row), v);
            }
            else
            {
                m_backend.setModDepth(static_cast<uint8_t>(row), v);
            }
        };
        m_sliders[static_cast<size_t>(i)].onDragStart = [this, row]() {
            m_sliderDragging[static_cast<size_t>(row)] = true;
            const uint8_t mod = m_backend.getModSource(static_cast<uint8_t>(row));
            if (mod != 255)
            {
                m_sliders[static_cast<size_t>(row)].setValue(
                    m_backend.getModDepth(static_cast<uint8_t>(row)), juce::dontSendNotification);
            }
        };
        m_sliders[static_cast<size_t>(i)].onDragEnd = [this, row]() {
            m_sliderDragging[static_cast<size_t>(row)] = false;
            updateRowKnobDisplay(row);
        };
        addAndMakeVisible(m_sliders[static_cast<size_t>(i)]);
    }

    if (m_backend.hasVcoWaveButtons())
    {
        for (int i = 0; i < 3; i++)
        {
            const int vco = i;
            m_waveButtons[static_cast<size_t>(i)].setMorph(
                m_backend.getVcoDisplayMorph(static_cast<size_t>(i)));
            m_waveButtons[static_cast<size_t>(i)].setOnClick([this, vco]() {
                m_backend.cycleVcoMorph(static_cast<size_t>(vco));
                m_waveButtons[static_cast<size_t>(vco)].setMorph(
                    m_backend.getVcoDisplayMorph(static_cast<size_t>(vco)));
            });
            addAndMakeVisible(m_waveButtons[static_cast<size_t>(i)]);
        }
    }

    m_fuegLabel.setText(m_backend.getRowName(7), juce::dontSendNotification);
    m_fuegLabel.setJustificationType(juce::Justification::centredLeft);
    m_fuegLabel.setTooltip("Crispy — scramble knobs 1–7 (Field OLED: FUEG; see Manual)");
    addAndMakeVisible(m_fuegLabel);

    m_fueg.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    m_fueg.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_fueg.setRange(0.0, 1.0, 0.001);
    m_fueg.setValue(0.0);
    m_fueg.onValueChange = [this]() {
        const uint8_t mod = m_backend.getModSource(7);
        const float v = static_cast<float>(m_fueg.getValue());
        if (mod == 255)
        {
            m_backend.setKnob(7, v);
        }
        else
        {
            m_backend.setModDepth(7, v);
        }
    };
    m_fueg.onDragStart = [this]() {
        m_fuegDragging = true;
        const uint8_t mod = m_backend.getModSource(7);
        if (mod != 255)
        {
            m_fueg.setValue(m_backend.getModDepth(7), juce::dontSendNotification);
        }
    };
    m_fueg.onDragEnd = [this]() {
        m_fuegDragging = false;
        updateFuegKnobDisplay();
    };
    addAndMakeVisible(m_fueg);

    if (m_backend.hasPairArBand())
    {
        for (int i = 0; i < AudioPairArLayout::kCellCount; i++)
        {
            const int index = i;
            addAndMakeVisible(m_pairArLabels[static_cast<size_t>(i)]);

            m_pairArSliders[static_cast<size_t>(i)].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            m_pairArSliders[static_cast<size_t>(i)].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            m_pairArSliders[static_cast<size_t>(i)].setRange(0.0, 1.0, 0.001);
            m_pairArSliders[static_cast<size_t>(i)].setValue(0.5);
            m_pairArSliders[static_cast<size_t>(i)].onValueChange = [this, index]() {
                const uint8_t mod = m_backend.getPairArModSource(static_cast<uint8_t>(index));
                const float v = static_cast<float>(m_pairArSliders[static_cast<size_t>(index)].getValue());
                if (mod == 255)
                {
                    m_backend.setPairArKnob(static_cast<uint8_t>(index), v);
                }
                else
                {
                    m_backend.setPairArModDepth(static_cast<uint8_t>(index), v);
                }
            };
            m_pairArSliders[static_cast<size_t>(i)].onDragStart = [this, index]() {
                m_pairArDragging[static_cast<size_t>(index)] = true;
                const uint8_t mod = m_backend.getPairArModSource(static_cast<uint8_t>(index));
                if (mod != 255)
                {
                    m_pairArSliders[static_cast<size_t>(index)].setValue(
                        m_backend.getPairArModDepth(static_cast<uint8_t>(index)),
                        juce::dontSendNotification);
                }
            };
            m_pairArSliders[static_cast<size_t>(i)].onDragEnd = [this, index]() {
                m_pairArDragging[static_cast<size_t>(index)] = false;
                updatePairArKnobDisplay(index);
            };
            addAndMakeVisible(m_pairArSliders[static_cast<size_t>(i)]);
        }
        setSize(300, 480 + AudioPairArLayout::kBandTopPad + AudioPairArLayout::kBandHeight);
    }
    else
    {
        setSize(300, 480);
    }
}

void SubModulePanel::layoutParameterRow(juce::Rectangle<int> rowArea,
                                        bool hasWave,
                                        juce::Label& label,
                                        juce::Slider& knob,
                                        WaveMorphButton* waveButton,
                                        juce::Rectangle<int>& jackBounds)
{
    auto inner = rowArea;
    label.setBounds(inner.removeFromTop(kLabelRowH));
    layoutControlCluster(inner, hasWave, knob, waveButton, jackBounds);
}

void SubModulePanel::updateRowKnobDisplay(int row)
{
    const uint8_t mod = m_backend.getModSource(static_cast<uint8_t>(row));
    if (mod == 255)
    {
        m_sliders[static_cast<size_t>(row)].setValue(
            m_backend.getKnob(static_cast<uint8_t>(row)), juce::dontSendNotification);
        return;
    }
    m_sliders[static_cast<size_t>(row)].setValue(
        m_backend.getEffectiveKnob(static_cast<uint8_t>(row)), juce::dontSendNotification);
}

void SubModulePanel::updateFuegKnobDisplay()
{
    const uint8_t fuegMod = m_backend.getModSource(7);
    if (fuegMod == 255)
    {
        m_fueg.setValue(m_backend.getKnob(7), juce::dontSendNotification);
        return;
    }
    m_fueg.setValue(m_backend.getEffectiveKnob(7), juce::dontSendNotification);
}

juce::Rectangle<float> SubModulePanel::inputJackScreenBounds(int row) const
{
    return localAreaToGlobal(m_inputJackBounds[static_cast<size_t>(row)]).toFloat();
}

juce::Rectangle<float> SubModulePanel::pairArJackScreenBounds(int index) const
{
    return localAreaToGlobal(m_pairArJackBounds[static_cast<size_t>(index)]).toFloat();
}

void SubModulePanel::updatePairArKnobDisplay(int index)
{
    const uint8_t mod = m_backend.getPairArModSource(static_cast<uint8_t>(index));
    if (mod == 255)
    {
        m_pairArSliders[static_cast<size_t>(index)].setValue(
            m_backend.getPairArKnob(static_cast<uint8_t>(index)), juce::dontSendNotification);
        return;
    }
    m_pairArSliders[static_cast<size_t>(index)].setValue(
        m_backend.getPairArEffectiveKnob(static_cast<uint8_t>(index)), juce::dontSendNotification);
}

void SubModulePanel::collectInputPorts(std::vector<PatchCableOverlay::InputPort>& ports) const
{
    for (int row = 0; row < 8; row++)
    {
        PatchCableOverlay::InputPort port;
        port.page = m_pageIndex;
        port.row = static_cast<uint8_t>(row);
        port.screenBounds = inputJackScreenBounds(row);
        ports.push_back(port);
    }

    if (!m_backend.hasPairArBand())
    {
        return;
    }

    for (int i = 0; i < AudioPairArLayout::kCellCount; i++)
    {
        PatchCableOverlay::InputPort port;
        port.page = m_pageIndex;
        port.row = static_cast<uint8_t>(AudioPairArLayout::kModRowBase + i);
        port.screenBounds = pairArJackScreenBounds(i);
        ports.push_back(port);
    }
}

void SubModulePanel::refresh()
{
    for (int i = 0; i < 7; i++)
    {
        m_paramLabels[static_cast<size_t>(i)].setText(
            m_backend.getRowName(static_cast<uint8_t>(i)), juce::dontSendNotification);

        if (!m_sliderDragging[static_cast<size_t>(i)])
        {
            updateRowKnobDisplay(i);
        }

        if (m_backend.hasVcoWaveButtons() && i < 3)
        {
            m_waveButtons[static_cast<size_t>(i)].setMorph(
                m_backend.getVcoDisplayMorph(static_cast<size_t>(i)));
        }
    }

    m_fuegLabel.setText(m_backend.getRowName(7), juce::dontSendNotification);

    if (!m_fuegDragging)
    {
        updateFuegKnobDisplay();
    }

    if (m_backend.hasPairArBand())
    {
        for (int i = 0; i < AudioPairArLayout::kCellCount; i++)
        {
            m_pairArLabels[static_cast<size_t>(i)].setText(
                m_backend.getPairArName(static_cast<uint8_t>(i)));
            if (!m_pairArDragging[static_cast<size_t>(i)])
            {
                updatePairArKnobDisplay(i);
            }
        }
    }
}

void SubModulePanel::resized()
{
    layoutPanel();
}

void SubModulePanel::layoutPanel()
{
    auto area = getLocalBounds().reduced(6);
    m_title.setBounds(area.removeFromTop(22));
    auto btnRow = area.removeFromTop(26);
    constexpr int kBtnPad = 4;
    const juce::Font btnFont =
        m_randomize.getLookAndFeel().getTextButtonFont(m_randomize, btnRow.getHeight());
    const int randW = btnFont.getStringWidth("Randomize") + kBtnPad;
    const int modW = btnFont.getStringWidth("Randmod") + kBtnPad;
    m_randomize.setBounds(btnRow.removeFromLeft(randW));
    btnRow.removeFromLeft(4);
    m_randomizeMod.setBounds(btnRow.removeFromLeft(modW));
    area.removeFromTop(4);

    for (int i = 0; i < 7; i++)
    {
        auto rowArea = area.removeFromTop(kRowHeight);
        const bool hasWave = m_backend.hasVcoWaveButtons() && i < 3;
        WaveMorphButton* wave =
            hasWave ? &m_waveButtons[static_cast<size_t>(i)] : nullptr;
        layoutParameterRow(rowArea,
                           hasWave,
                           m_paramLabels[static_cast<size_t>(i)],
                           m_sliders[static_cast<size_t>(i)],
                           wave,
                           m_inputJackBounds[static_cast<size_t>(i)]);
    }

    layoutParameterRow(area.removeFromTop(kRowHeight),
                       false,
                       m_fuegLabel,
                       m_fueg,
                       nullptr,
                       m_inputJackBounds[7]);

    if (m_backend.hasPairArBand())
    {
        area.removeFromTop(AudioPairArLayout::kBandTopPad);
        layoutPairArBand(area.removeFromTop(AudioPairArLayout::kBandHeight));
    }
}

void SubModulePanel::layoutPairArBand(juce::Rectangle<int> bandArea)
{
    const int colCount = AudioPairArLayout::kCellCount;
    const int totalGap = (colCount - 1) * AudioPairArLayout::kColumnPad;
    const int colW = (bandArea.getWidth() - totalGap) / colCount;
    int x = bandArea.getX();
    const int y = bandArea.getY();
    const int h = bandArea.getHeight();

    for (int i = 0; i < colCount; i++)
    {
        juce::Rectangle<int> col(x, y, colW, h);
        x += colW + AudioPairArLayout::kColumnPad;

        m_pairArJackBounds[static_cast<size_t>(i)] =
            col.removeFromTop(AudioPairArLayout::kJackSize)
                .withSizeKeepingCentre(AudioPairArLayout::kJackSize, AudioPairArLayout::kJackSize);
        col.removeFromTop(AudioPairArLayout::kStackGap);
        m_pairArSliders[static_cast<size_t>(i)].setBounds(
            col.removeFromTop(AudioPairArLayout::kKnobSize)
                .withSizeKeepingCentre(AudioPairArLayout::kKnobSize, AudioPairArLayout::kKnobSize));
        col.removeFromTop(AudioPairArLayout::kStackGap);
        m_pairArLabels[static_cast<size_t>(i)].setBounds(
            col.removeFromTop(AudioPairArLayout::kPairArLabelZoneH));
    }
}

void SubModulePanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(kModuleBorder));
    g.drawRect(getLocalBounds(), 1.0f);

    for (int row = 0; row < 8; row++)
    {
        const uint8_t mod = m_backend.getModSource(static_cast<uint8_t>(row));
        if (mod == 255)
        {
            continue;
        }
        const auto knobBounds = row < 7 ? m_sliders[static_cast<size_t>(row)].getBounds()
                                        : m_fueg.getBounds();
        g.setColour(juce::Colour(0xff5a9fd4).withAlpha(0.55f));
        g.drawEllipse(knobBounds.toFloat().reduced(1.0f), 2.0f);
    }

    g.setColour(juce::Colour(0xffc8d0dc));
    for (int row = 0; row < 8; row++)
    {
        g.fillEllipse(m_inputJackBounds[static_cast<size_t>(row)].toFloat());
        g.drawEllipse(m_inputJackBounds[static_cast<size_t>(row)].toFloat(), 1.0f);
    }

    if (!m_backend.hasPairArBand())
    {
        return;
    }

    for (int i = 0; i < AudioPairArLayout::kCellCount; i++)
    {
        const uint8_t mod = m_backend.getPairArModSource(static_cast<uint8_t>(i));
        if (mod != 255)
        {
            g.setColour(juce::Colour(0xff5a9fd4).withAlpha(0.55f));
            g.drawEllipse(m_pairArSliders[static_cast<size_t>(i)].getBounds().toFloat().reduced(1.0f), 2.0f);
        }
    }

    for (int i = 0; i < AudioPairArLayout::kCellCount; i++)
    {
        g.setColour(juce::Colour(0xffc8d0dc));
        g.fillEllipse(m_pairArJackBounds[static_cast<size_t>(i)].toFloat());
        g.drawEllipse(m_pairArJackBounds[static_cast<size_t>(i)].toFloat(), 1.0f);
    }
}
