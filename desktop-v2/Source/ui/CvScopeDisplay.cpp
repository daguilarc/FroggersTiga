#include "CvScopeDisplay.h"

#include <cmath>

CvScopeDisplay::CvScopeDisplay()
{
    m_traceColours.fill(juce::Colour(0xff7ec8ff));
    setTooltip("Mod CV trace (not audio)");
}

void CvScopeDisplay::setTraceCount(size_t count)
{
    m_traceCount = std::min(count, kMaxTraces);
    repaint();
}

void CvScopeDisplay::setTraceColour(size_t trace, juce::Colour colour)
{
    if (trace < kMaxTraces)
    {
        m_traceColours[trace] = colour;
        repaint();
    }
}

void CvScopeDisplay::setTraceMode(CvTraceMode mode)
{
    m_traceMode = mode;
    repaint();
}

void CvScopeDisplay::setShowGrid(bool show)
{
    m_showGrid = show;
    repaint();
}

void CvScopeDisplay::setIdle(bool idle)
{
    m_idle = idle;
    repaint();
}

void CvScopeDisplay::setTraceAudioRateModulated(size_t trace, bool modulated)
{
    if (trace >= kMaxTraces || m_traceAudioRateModulated[trace] == modulated)
    {
        return;
    }
    m_traceAudioRateModulated[trace] = modulated;
    repaint();
}

bool CvScopeDisplay::hasAudioRateActivity(size_t trace, float threshold) const
{
    if (trace >= kMaxTraces || !m_hasSamples[trace])
    {
        return false;
    }

    float deltaEnergy = 0.0f;
    for (size_t step = 1; step < 8; ++step)
    {
        const size_t index = (m_writeIndex[trace] + kBufferSize - step) % kBufferSize;
        const size_t prevIndex = (index + kBufferSize - 1) % kBufferSize;
        const float delta = m_samples[trace][index] - m_samples[trace][prevIndex];
        deltaEnergy += delta * delta;
    }
    return deltaEnergy > threshold;
}

float CvScopeDisplay::clamp01(float value) const
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

float CvScopeDisplay::sampleY(float value01, float bottom, float height) const
{
    return bottom - clamp01(value01) * height;
}

void CvScopeDisplay::traceBufferMinMax(size_t trace, float& outMin, float& outMax) const
{
    float lo = m_samples[trace][0];
    float hi = m_samples[trace][0];
    for (size_t i = 1; i < kBufferSize; ++i)
    {
        const float v = m_samples[trace][i];
        if (v < lo)
        {
            lo = v;
        }
        if (v > hi)
        {
            hi = v;
        }
    }
    outMin = lo;
    outMax = hi;
}

float CvScopeDisplay::normalizeToTraceRange(float value01, float lo, float hi) const
{
    const float range = hi - lo;
    if (range <= kAutoScaleEpsilon)
    {
        return 0.5f;
    }
    return clamp01((value01 - lo) / range);
}

float CvScopeDisplay::displayNormalized01(size_t trace, float value01) const
{
    if (trace >= kMaxTraces || !m_hasSamples[trace])
    {
        return clamp01(value01);
    }
    float lo = 0.0f;
    float hi = 1.0f;
    traceBufferMinMax(trace, lo, hi);
    return normalizeToTraceRange(value01, lo, hi);
}

void CvScopeDisplay::pushSample(float value01)
{
    pushSample(0, value01);
}

void CvScopeDisplay::pushSample(size_t trace, float value01)
{
    if (trace >= kMaxTraces)
    {
        return;
    }
    const float clamped = clamp01(value01);
    m_samples[trace][m_writeIndex[trace]] = clamped;
    m_writeIndex[trace] = (m_writeIndex[trace] + 1) % kBufferSize;
    m_hasSamples[trace] = true;
    m_lastLevel[trace] = clamped;
    m_hasLastLevel[trace] = true;
    repaint();
}

void CvScopeDisplay::paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(juce::Colour(0xff3d4450).withAlpha(0.85f));
    for (const float level : {0.0f, 0.5f, 1.0f})
    {
        const float y = sampleY(level, bounds.getBottom(), bounds.getHeight());
        g.drawHorizontalLine(static_cast<int>(std::lround(y)), bounds.getX(), bounds.getRight());
    }
}

void CvScopeDisplay::paintLevelFill(juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    size_t trace) const
{
    if (trace >= kMaxTraces || !m_hasLastLevel[trace])
    {
        return;
    }
    // Idle keeps shared-axis last-level Y; active paint uses per-trace auto-scale.
    float displayValue = m_lastLevel[trace];
    if (!m_idle && m_hasSamples[trace])
    {
        float lo = 0.0f;
        float hi = 1.0f;
        traceBufferMinMax(trace, lo, hi);
        displayValue = normalizeToTraceRange(m_lastLevel[trace], lo, hi);
    }
    const float y = sampleY(displayValue, bounds.getBottom(), bounds.getHeight());
    const auto fill = juce::Rectangle<float>(
        bounds.getX(),
        y,
        bounds.getWidth(),
        bounds.getBottom() - y);
    g.setColour(m_traceColours[trace].withAlpha(0.25f));
    g.fillRect(fill);
}

void CvScopeDisplay::paintIdle(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    if (m_showGrid)
    {
        paintGrid(g, bounds);
    }
    for (size_t trace = 0; trace < m_traceCount; ++trace)
    {
        if (!m_hasLastLevel[trace])
        {
            continue;
        }
        const float y = sampleY(m_lastLevel[trace], bounds.getBottom(), bounds.getHeight());
        g.setColour(m_traceColours[trace].withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(std::lround(y)), bounds.getX(), bounds.getRight());
        paintLevelFill(g, bounds, trace);
    }
    if (m_traceCount == 1 && !m_hasLastLevel[0])
    {
        const float yMid = sampleY(0.5f, bounds.getBottom(), bounds.getHeight());
        g.setColour(juce::Colour(0xff5a6270).withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(std::lround(yMid)), bounds.getX(), bounds.getRight());
    }
}

void CvScopeDisplay::paintTrace(juce::Graphics& g, juce::Rectangle<float> bounds, size_t trace) const
{
    if (trace >= kMaxTraces || !m_hasSamples[trace])
    {
        return;
    }

    juce::Path path;
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    float prevY = bounds.getBottom();
    bool hasPoint = false;
    float lo = 0.0f;
    float hi = 1.0f;
    traceBufferMinMax(trace, lo, hi);

    for (size_t i = 0; i < kBufferSize; ++i)
    {
        const size_t index = (m_writeIndex[trace] + i) % kBufferSize;
        const size_t prevIndex = (index + kBufferSize - 1) % kBufferSize;
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(kBufferSize - 1)) * width;
        const float display01 = normalizeToTraceRange(m_samples[trace][index], lo, hi);
        const float y = sampleY(display01, bounds.getBottom(), height);
        if (!hasPoint)
        {
            path.startNewSubPath(x, y);
            prevY = y;
            hasPoint = true;
            continue;
        }

        if (m_traceMode == CvTraceMode::StepHold)
        {
            const float delta = std::fabs(m_samples[trace][index] - m_samples[trace][prevIndex]);
            if (delta < kStepThreshold)
            {
                path.lineTo(x, y);
            }
            else
            {
                path.lineTo(x, prevY);
                path.lineTo(x, y);
            }
        }
        else
        {
            path.lineTo(x, y);
        }
        prevY = y;
    }

    g.setColour(m_traceColours[trace]);
    const bool modulated = m_traceAudioRateModulated[trace];
    if (modulated)
    {
        g.setColour(m_traceColours[trace].withAlpha(0.35f));
        g.strokePath(path, juce::PathStrokeType(3.5f));
        g.setColour(m_traceColours[trace]);
    }
    g.strokePath(path, juce::PathStrokeType(modulated ? 2.5f : 1.5f));
}

void CvScopeDisplay::paintAllTraces(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    for (size_t trace = 0; trace < m_traceCount; ++trace)
    {
        if (m_traceMode == CvTraceMode::StepHold)
        {
            paintLevelFill(g, bounds, trace);
        }
        paintTrace(g, bounds, trace);
    }
}

void CvScopeDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff1a1d22));
    g.fillRoundedRectangle(bounds, 2.0f);
    g.setColour(juce::Colour(0xff3d4450));
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);

    if (m_idle)
    {
        paintIdle(g, bounds);
        return;
    }

    if (m_showGrid)
    {
        paintGrid(g, bounds);
    }

    bool anySamples = false;
    for (size_t trace = 0; trace < m_traceCount; ++trace)
    {
        if (m_hasSamples[trace])
        {
            anySamples = true;
            break;
        }
    }
    if (!anySamples)
    {
        return;
    }

    paintAllTraces(g, bounds);
}
