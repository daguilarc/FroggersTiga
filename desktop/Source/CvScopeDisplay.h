#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstddef>

enum class CvTraceMode
{
    Continuous,
    StepHold
};

class CvScopeDisplay : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    static constexpr size_t kBufferSize = 96;
    static constexpr float kStepThreshold = 0.02f;

    CvScopeDisplay();

    void setTraceMode(CvTraceMode mode);
    void setShowGrid(bool show);
    void setIdle(bool idle);
    void pushSample(float value01);
    void paint(juce::Graphics& g) override;

private:
    float clamp01(float value) const;
    float sampleY(float value01, float bottom, float height) const;
    void paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintLevelFill(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintIdle(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintTrace(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    std::array<float, kBufferSize> m_samples{};
    size_t m_writeIndex = 0;
    bool m_hasSamples = false;
    float m_lastLevel = 0.5f;
    bool m_hasLastLevel = false;
    CvTraceMode m_traceMode = CvTraceMode::Continuous;
    bool m_showGrid = false;
    bool m_idle = false;
};
