#pragma once

#include "PairArRotatedLabel.h"
#include "PanelBackend.hpp"
#include "PatchCableOverlay.h"
#include "WaveMorphButton.h"
#include "AudioPairArLayout.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

class SubModulePanel : public juce::Component
{
public:
    SubModulePanel(uint8_t pageIndex, juce::String title, IPanelBackend& backend);

    void refresh();
    void collectInputPorts(std::vector<PatchCableOverlay::InputPort>& ports) const;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateRowKnobDisplay(int row);
    void updateFuegKnobDisplay();
    void layoutPanel();
    void layoutPairArBand(juce::Rectangle<int> bandArea);
    void layoutParameterRow(juce::Rectangle<int> rowArea,
                            bool hasWave,
                            juce::Label& label,
                            juce::Slider& knob,
                            WaveMorphButton* waveButton,
                            juce::Rectangle<int>& jackBounds);
    juce::Rectangle<float> inputJackScreenBounds(int row) const;
    juce::Rectangle<float> pairArJackScreenBounds(int index) const;
    void updatePairArKnobDisplay(int index);

    uint8_t m_pageIndex;
    IPanelBackend& m_backend;
    juce::Label m_title;
    juce::TextButton m_randomize{"Randomize"};
    juce::TextButton m_randomizeMod{"Randmod"};
    std::array<juce::Label, 7> m_paramLabels;
    std::array<WaveMorphButton, 3> m_waveButtons;
    std::array<juce::Slider, 7> m_sliders;
    juce::Label m_fuegLabel;
    juce::Slider m_fueg;
    std::array<PairArRotatedLabel, AudioPairArLayout::kCellCount> m_pairArLabels{};
    std::array<juce::Slider, AudioPairArLayout::kCellCount> m_pairArSliders{};
    std::array<juce::Rectangle<int>, AudioPairArLayout::kCellCount> m_pairArJackBounds{};
    std::array<bool, AudioPairArLayout::kCellCount> m_pairArDragging{};
    std::array<juce::Rectangle<int>, 8> m_inputJackBounds{};
    std::array<bool, 7> m_sliderDragging{};
    bool m_fuegDragging = false;
};
