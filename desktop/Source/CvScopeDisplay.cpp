#include "CvScopeDisplay.h"

#include <cmath>

CvScopeDisplay::CvScopeDisplay()
{
    setTooltip("Mod CV trace (not audio)");
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

void CvScopeDisplay::pushSample(float value01)
{
    const float clamped = clamp01(value01);
    m_samples[m_writeIndex] = clamped;
    m_writeIndex = (m_writeIndex + 1) % kBufferSize;
    m_hasSamples = true;
    m_lastLevel = clamped;
    m_hasLastLevel = true;
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

void CvScopeDisplay::paintLevelFill(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    if (!m_hasLastLevel)
    {
        return;
    }
    const float y = sampleY(m_lastLevel, bounds.getBottom(), bounds.getHeight());
    const auto fill = juce::Rectangle<float>(
        bounds.getX(),
        y,
        bounds.getWidth(),
        bounds.getBottom() - y);
    g.setColour(juce::Colour(0xff7ec8ff).withAlpha(0.25f));
    g.fillRect(fill);
}

void CvScopeDisplay::paintIdle(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    if (m_showGrid)
    {
        paintGrid(g, bounds);
    }
    if (m_hasLastLevel)
    {
        const float y = sampleY(m_lastLevel, bounds.getBottom(), bounds.getHeight());
        g.setColour(juce::Colour(0xff7ec8ff).withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(std::lround(y)), bounds.getX(), bounds.getRight());
        paintLevelFill(g, bounds);
        return;
    }
    const float yMid = sampleY(0.5f, bounds.getBottom(), bounds.getHeight());
    g.setColour(juce::Colour(0xff5a6270).withAlpha(0.55f));
    g.drawHorizontalLine(static_cast<int>(std::lround(yMid)), bounds.getX(), bounds.getRight());
}

void CvScopeDisplay::paintTrace(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    juce::Path trace;
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    float prevY = bounds.getBottom();
    bool hasPoint = false;

    for (size_t i = 0; i < kBufferSize; ++i)
    {
        const size_t index = (m_writeIndex + i) % kBufferSize;
        const size_t prevIndex = (index + kBufferSize - 1) % kBufferSize;
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(kBufferSize - 1)) * width;
        const float y = sampleY(m_samples[index], bounds.getBottom(), height);
        if (!hasPoint)
        {
            trace.startNewSubPath(x, y);
            prevY = y;
            hasPoint = true;
            continue;
        }

        if (m_traceMode == CvTraceMode::StepHold)
        {
            const float delta = std::fabs(m_samples[index] - m_samples[prevIndex]);
            if (delta < kStepThreshold)
            {
                trace.lineTo(x, y);
            }
            else
            {
                trace.lineTo(x, prevY);
                trace.lineTo(x, y);
            }
        }
        else
        {
            trace.lineTo(x, y);
        }
        prevY = y;
    }

    g.setColour(juce::Colour(0xff7ec8ff));
    g.strokePath(trace, juce::PathStrokeType(1.5f));
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

    if (m_traceMode == CvTraceMode::StepHold)
    {
        paintLevelFill(g, bounds);
    }

    if (!m_hasSamples)
    {
        return;
    }

    paintTrace(g, bounds);
}
