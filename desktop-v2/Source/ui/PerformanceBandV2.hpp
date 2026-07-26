#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DesktopHostIO.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstdint>

class PerformanceBandV2 : public juce::Component
{
public:
    PerformanceBandV2();

    void bind(froggers_v2::FroggersV2ControlCore* core);
    void bindHost(DesktopHostIO* host);
    void refresh();
    void refreshMarbles(bool audioRunning);
    void resized() override;
    void paint(juce::Graphics& g) override;

    juce::Rectangle<int> sceneLabelBoundsForTest() const { return m_sceneLabel.getBounds(); }
    juce::Rectangle<int> blendLabelLBoundsForTest() const { return m_blendLabelL.getBounds(); }
    juce::Rectangle<int> blendLabelRBoundsForTest() const { return m_blendLabelR.getBounds(); }
    juce::Rectangle<int> marblesLabel1BoundsForTest() const { return m_marblesLabel1.getBounds(); }
    juce::Rectangle<int> marblesLabel2BoundsForTest() const { return m_marblesLabel2.getBounds(); }
    juce::Rectangle<int> scene1BoundsForTest() const { return m_scene1.getBounds(); }
    juce::Rectangle<int> sceneBlendBoundsForTest() const { return m_sceneBlend.getBounds(); }

private:
    void pushScene(uint8_t ordinal);
    void paintMarblesLed(juce::Graphics& g, juce::Rectangle<int> area, float level, bool active) const;
    void updateSceneButtonLabels(uint8_t leftOrdinal, uint8_t rightOrdinal);

    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    DesktopHostIO* m_host = nullptr;
    bool m_audioRunning = false;
    std::array<float, 2> m_marblesLevel{};

    juce::Label m_sceneLabel;
    juce::TextButton m_scene1{"S1"};
    juce::TextButton m_scene2{"S2"};
    juce::TextButton m_scene3{"S3"};
    juce::Label m_blendLabelL;
    juce::Slider m_sceneBlend;
    juce::Label m_blendLabelR;
    juce::Label m_marblesLabel1;
    juce::Label m_marblesLabel2;
};
