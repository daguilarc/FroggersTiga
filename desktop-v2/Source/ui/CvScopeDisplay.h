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
    static constexpr size_t kMaxTraces = 4;
    static constexpr float kStepThreshold = 0.02f;

    CvScopeDisplay();

    void setTraceCount(size_t count);
    void setTraceColour(size_t trace, juce::Colour colour);
    void setTraceMode(CvTraceMode mode);
    void setShowGrid(bool show);
    void setIdle(bool idle);
    void setTraceAudioRateModulated(size_t trace, bool modulated);
    void pushSample(float value01);
    void pushSample(size_t trace, float value01);
    bool hasAudioRateActivity(size_t trace, float threshold = 0.0008f) const;
    // Display-space 0..1 for paint Y (per-trace ring min/max). Raw samples unchanged.
    float displayNormalized01(size_t trace, float value01) const;
    void paint(juce::Graphics& g) override;

    static constexpr float kAutoScaleEpsilon = 1.0e-4f;

private:
    float clamp01(float value) const;
    float sampleY(float value01, float bottom, float height) const;
    void traceBufferMinMax(size_t trace, float& outMin, float& outMax) const;
    float normalizeToTraceRange(float value01, float lo, float hi) const;
    void paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintLevelFill(juce::Graphics& g, juce::Rectangle<float> bounds, size_t trace) const;
    void paintIdle(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintTrace(juce::Graphics& g, juce::Rectangle<float> bounds, size_t trace) const;
    void paintAllTraces(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    std::array<std::array<float, kBufferSize>, kMaxTraces> m_samples{};
    std::array<size_t, kMaxTraces> m_writeIndex{};
    std::array<bool, kMaxTraces> m_hasSamples{};
    std::array<float, kMaxTraces> m_lastLevel{};
    std::array<bool, kMaxTraces> m_hasLastLevel{};
    std::array<juce::Colour, kMaxTraces> m_traceColours{};
    std::array<bool, kMaxTraces> m_traceAudioRateModulated{};
    size_t m_traceCount = 1;
    CvTraceMode m_traceMode = CvTraceMode::Continuous;
    bool m_showGrid = false;
    bool m_idle = false;
};
