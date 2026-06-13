#pragma once

#include "PanelBackend.hpp"
#include "PatchCableOverlay.h"
#include "WaveMorphButton.h"

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
    juce::Rectangle<float> inputJackScreenBounds(int row) const;

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
    std::array<juce::Rectangle<int>, 8> m_inputJackBounds{};
    std::array<bool, 7> m_sliderDragging{};
    bool m_fuegDragging = false;
};
