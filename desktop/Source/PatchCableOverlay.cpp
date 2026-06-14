#include "PatchCableOverlay.h"

#include <cmath>

PatchCableOverlay::PatchCableOverlay(DesktopHostIO& host, DelayState& delay)
    : m_host(host)
    , m_delay(delay)
{
    setInterceptsMouseClicks(true, true);
}

void PatchCableOverlay::setConnectionChangedCallback(ConnectionChangedFn fn)
{
    m_onConnectionChanged = std::move(fn);
}

void PatchCableOverlay::removeCablesForModIndex(uint8_t modIndex)
{
    for (const InputPort& port : m_inputs)
    {
        if (getModSource(port.page, port.row) == modIndex)
        {
            removeCableHue(port.page, port.row);
        }
    }
}

uint8_t PatchCableOverlay::getModSource(uint8_t page, uint8_t row) const
{
    if (page >= DelayState::kDelayPageIndex)
    {
        return m_delay.getModSource(row);
    }
    return m_host.GetPageModSource(page, row);
}

void PatchCableOverlay::setModSource(uint8_t page, uint8_t row, uint8_t modIndex)
{
    if (modIndex != 255 && !m_host.IsModSourceAvailable(modIndex))
    {
        return;
    }
    if (page >= DelayState::kDelayPageIndex)
    {
        m_host.EnqueueDelaySetModSource(row, modIndex);
        return;
    }
    m_host.EnqueueSetPageModSource(page, row, modIndex);
}

void PatchCableOverlay::clearModSource(uint8_t page, uint8_t row)
{
    setModSource(page, row, 255);
    removeCableHue(page, row);
}

void PatchCableOverlay::setOutputPorts(std::vector<OutputPort> ports)
{
    m_outputs = std::move(ports);
}

void PatchCableOverlay::setInputPorts(std::vector<InputPort> ports)
{
    m_inputs = std::move(ports);
}

int PatchCableOverlay::connectionKey(uint8_t page, uint8_t row)
{
    return static_cast<int>(page) * 16 + static_cast<int>(row);
}

float PatchCableOverlay::randomHue() const
{
    return juce::Random::getSystemRandom().nextFloat();
}

juce::Colour PatchCableOverlay::colourFromHue(float hue) const
{
    return juce::Colour::fromHSV(hue, 0.88f, 0.98f, 1.0f);
}

float PatchCableOverlay::assignCableHue(uint8_t page, uint8_t row)
{
    const float hue = randomHue();
    m_cableHues[connectionKey(page, row)] = hue;
    return hue;
}

void PatchCableOverlay::removeCableHue(uint8_t page, uint8_t row)
{
    m_cableHues.erase(connectionKey(page, row));
}

juce::Colour PatchCableOverlay::colourForConnection(uint8_t page, uint8_t row) const
{
    return colourFromHue(hueForConnection(page, row));
}

float PatchCableOverlay::hueForConnection(uint8_t page, uint8_t row) const
{
    const int key = connectionKey(page, row);
    const auto it = m_cableHues.find(key);
    if (it != m_cableHues.end())
    {
        return it->second;
    }
    return std::fmod(static_cast<float>(key) * 0.171f + 0.05f, 1.0f);
}

void PatchCableOverlay::drawPortSocket(juce::Graphics& g,
                                       juce::Rectangle<float> screenBounds,
                                       juce::Colour ringColour,
                                       const bool isOutput) const
{
    const juce::Point<float> centre =
        getLocalPoint(nullptr, screenBounds.getCentre().toInt()).toFloat();
    const float outerR = isOutput ? 10.0f : 11.0f;

    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillEllipse(centre.x - outerR - 1.5f, centre.y - outerR - 1.5f, (outerR + 1.5f) * 2.0f, (outerR + 1.5f) * 2.0f);
    g.setColour(juce::Colour(0xff252a32));
    g.fillEllipse(centre.x - outerR, centre.y - outerR, outerR * 2.0f, outerR * 2.0f);
    g.setColour(ringColour);
    g.drawEllipse(centre.x - outerR, centre.y - outerR, outerR * 2.0f, outerR * 2.0f, 2.5f);
    g.setColour(juce::Colour(0xff0a0c10));
    g.fillEllipse(centre.x - 4.5f, centre.y - 4.5f, 9.0f, 9.0f);
}

void PatchCableOverlay::notifyConnectionChanged(uint8_t page)
{
    if (m_onConnectionChanged)
    {
        m_onConnectionChanged(page);
    }
}

bool PatchCableOverlay::portContains(juce::Rectangle<float> bounds, juce::Point<float> screenPos) const
{
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float dx = screenPos.x - cx;
    const float dy = screenPos.y - cy;
    return dx * dx + dy * dy <= kPortHitRadius * kPortHitRadius;
}

const PatchCableOverlay::OutputPort* PatchCableOverlay::hitOutputPort(juce::Point<float> screenPos) const
{
    for (const OutputPort& port : m_outputs)
    {
        if (!port.patchEnabled)
        {
            continue;
        }
        if (portContains(port.screenBounds, screenPos))
        {
            return &port;
        }
    }
    return nullptr;
}

const PatchCableOverlay::InputPort* PatchCableOverlay::hitInputPort(juce::Point<float> screenPos) const
{
    for (const InputPort& port : m_inputs)
    {
        if (portContains(port.screenBounds, screenPos))
        {
            return &port;
        }
    }
    return nullptr;
}

void PatchCableOverlay::drawCable(juce::Graphics& g,
                                  juce::Point<float> fromScreen,
                                  juce::Point<float> toScreen,
                                  juce::Colour colour) const
{
    const juce::Point<float> fromLocal = getLocalPoint(nullptr, fromScreen.toInt()).toFloat();
    const juce::Point<float> toLocal = getLocalPoint(nullptr, toScreen.toInt()).toFloat();
    juce::Path path;
    path.startNewSubPath(fromLocal);
    const float dx = (toLocal.x - fromLocal.x) * 0.5f;
    path.cubicTo(fromLocal.x + dx,
                 fromLocal.y,
                 toLocal.x - dx,
                 toLocal.y,
                 toLocal.x,
                 toLocal.y);

    const juce::PathStrokeType outlineStroke(
        kCableOutline, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    const juce::PathStrokeType cableStroke(
        kCableStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.strokePath(path, outlineStroke);
    g.setColour(colour);
    g.strokePath(path, cableStroke);
    g.setColour(colour.brighter(0.25f).withAlpha(0.45f));
    g.strokePath(path, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PatchCableOverlay::paint(juce::Graphics& g)
{
    for (const OutputPort& port : m_outputs)
    {
        juce::Colour ring = port.patchEnabled ? juce::Colour(0xffc8d0dc) : juce::Colour(0xff6a7380);
        if (port.patchEnabled && m_highlightOutputMod && *m_highlightOutputMod == port.modIndex)
        {
            ring = juce::Colours::yellow;
        }
        drawPortSocket(g, port.screenBounds, ring, true);
    }

    for (const InputPort& port : m_inputs)
    {
        const uint8_t mod = getModSource(port.page, port.row);
        const juce::Colour ring =
            mod == 255 ? juce::Colour(0xff6a7380) : colourForConnection(port.page, port.row);
        drawPortSocket(g, port.screenBounds, ring, false);
    }

    for (const InputPort& port : m_inputs)
    {
        const uint8_t mod = getModSource(port.page, port.row);
        if (mod == 255)
        {
            continue;
        }
        for (const OutputPort& out : m_outputs)
        {
            if (out.modIndex != mod || !out.patchEnabled)
            {
                continue;
            }
            drawCable(g,
                      out.screenBounds.getCentre(),
                      port.screenBounds.getCentre(),
                      colourForConnection(port.page, port.row));
            break;
        }
    }

    if (m_drag.active && m_drag.pastThreshold)
    {
        drawCable(g,
                  m_drag.anchorScreen,
                  m_drag.cursorScreen,
                  colourFromHue(m_dragCableHue));
    }

    for (const InputPort& port : m_inputs)
    {
        const int key = connectionKey(port.page, port.row);
        if (m_highlightInputKey && *m_highlightInputKey == key)
        {
            const juce::Point<float> centre =
                getLocalPoint(nullptr, port.screenBounds.getCentre().toInt()).toFloat();
            g.setColour(juce::Colours::yellow.withAlpha(0.45f));
            g.fillEllipse(centre.x - 12.0f, centre.y - 12.0f, 24.0f, 24.0f);
        }
    }
}

bool PatchCableOverlay::hitTest(int x, int y)
{
    if (m_drag.active)
    {
        return true;
    }
    const juce::Point<float> screen = localPointToGlobal(juce::Point<int>(x, y)).toFloat();
    return hitOutputPort(screen) != nullptr || hitInputPort(screen) != nullptr;
}

void PatchCableOverlay::mouseDown(const juce::MouseEvent& e)
{
    const juce::Point<float> screen = e.getScreenPosition().toFloat();
    if (const OutputPort* out = hitOutputPort(screen))
    {
        m_drag.active = true;
        m_drag.pastThreshold = false;
        m_drag.fromOutput = true;
        m_drag.hadConnection = false;
        m_drag.modIndex = out->modIndex;
        m_drag.anchorScreen = out->screenBounds.getCentre();
        m_drag.startScreen = screen;
        m_drag.cursorScreen = screen;
        m_dragCableHue = randomHue();
        return;
    }

    if (const InputPort* in = hitInputPort(screen))
    {
        const uint8_t mod = getModSource(in->page, in->row);
        m_drag.active = true;
        m_drag.fromOutput = false;
        m_drag.page = in->page;
        m_drag.row = in->row;
        m_drag.anchorScreen = in->screenBounds.getCentre();
        m_drag.startScreen = screen;
        m_drag.cursorScreen = screen;
        if (mod == 255)
        {
            m_drag.pastThreshold = false;
            m_drag.hadConnection = false;
            m_drag.modIndex = 255;
            m_dragCableHue = randomHue();
            return;
        }
        m_drag.pastThreshold = true;
        m_drag.hadConnection = true;
        m_drag.modIndex = mod;
        const auto it = m_cableHues.find(connectionKey(in->page, in->row));
        m_dragCableHue = it != m_cableHues.end() ? it->second : randomHue();
    }
}

void PatchCableOverlay::mouseDrag(const juce::MouseEvent& e)
{
    if (!m_drag.active)
    {
        return;
    }
    m_drag.cursorScreen = e.getScreenPosition().toFloat();
    if (!m_drag.pastThreshold)
    {
        const float dx = m_drag.cursorScreen.x - m_drag.startScreen.x;
        const float dy = m_drag.cursorScreen.y - m_drag.startScreen.y;
        if (dx * dx + dy * dy >= kDragThreshold * kDragThreshold)
        {
            m_drag.pastThreshold = true;
        }
    }

    m_highlightInputKey.reset();
    m_highlightOutputMod.reset();
    if (m_drag.pastThreshold)
    {
        if (m_drag.fromOutput)
        {
            if (const InputPort* in = hitInputPort(m_drag.cursorScreen))
            {
                m_highlightInputKey = connectionKey(in->page, in->row);
            }
        }
        else if (const OutputPort* out = hitOutputPort(m_drag.cursorScreen))
        {
            m_highlightOutputMod = out->modIndex;
        }
    }
    repaint();
}

void PatchCableOverlay::finishDrag(juce::Point<float> screenPos)
{
    if (!m_drag.active || !m_drag.pastThreshold)
    {
        resetDrag();
        return;
    }

    if (const OutputPort* out = hitOutputPort(screenPos))
    {
        if (!m_drag.fromOutput)
        {
            if (m_drag.hadConnection && out->modIndex == m_drag.modIndex)
            {
                resetDrag();
                return;
            }
            if (m_drag.hadConnection)
            {
                clearModSource(m_drag.page, m_drag.row);
                notifyConnectionChanged(m_drag.page);
            }
            setModSource(m_drag.page, m_drag.row, out->modIndex);
            assignCableHue(m_drag.page, m_drag.row);
            notifyConnectionChanged(m_drag.page);
        }
        resetDrag();
        return;
    }

    if (const InputPort* in = hitInputPort(screenPos))
    {
        if (!m_drag.fromOutput && !m_drag.hadConnection)
        {
            resetDrag();
            return;
        }
        if (!m_drag.fromOutput && m_drag.hadConnection && in->page == m_drag.page && in->row == m_drag.row)
        {
            resetDrag();
            return;
        }
        if (!m_drag.fromOutput && m_drag.hadConnection)
        {
            clearModSource(m_drag.page, m_drag.row);
            notifyConnectionChanged(m_drag.page);
        }
        setModSource(in->page, in->row, m_drag.modIndex);
        assignCableHue(in->page, in->row);
        notifyConnectionChanged(in->page);
        resetDrag();
        return;
    }

    if (m_drag.hadConnection)
    {
        clearModSource(m_drag.page, m_drag.row);
        notifyConnectionChanged(m_drag.page);
    }
    resetDrag();
}

void PatchCableOverlay::resetDrag()
{
    m_drag = DragState{};
    m_highlightInputKey.reset();
    m_highlightOutputMod.reset();
    repaint();
}

void PatchCableOverlay::mouseUp(const juce::MouseEvent& e)
{
    finishDrag(e.getScreenPosition().toFloat());
}
