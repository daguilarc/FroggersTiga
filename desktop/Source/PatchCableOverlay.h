#pragma once

#include "DesktopHostIO.hpp"
#include "DelayState.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

class PatchCableOverlay : public juce::Component
{
public:
    static constexpr float kPortHitRadius = 22.0f;
    static constexpr float kDragThreshold = 4.0f;
    static constexpr float kCableStroke = 7.0f;
    static constexpr float kCableOutline = 10.0f;

    using ConnectionChangedFn = std::function<void(uint8_t page)>;

    struct OutputPort
    {
        uint8_t modIndex = 0;
        bool patchEnabled = true;
        juce::Rectangle<float> screenBounds;
    };

    struct InputPort
    {
        uint8_t page = 0;
        uint8_t row = 0;
        juce::Rectangle<float> screenBounds;
    };

    PatchCableOverlay(DesktopHostIO& host, DelayState& delay);

    void setOutputPorts(std::vector<OutputPort> ports);
    void setInputPorts(std::vector<InputPort> ports);
    void setConnectionChangedCallback(ConnectionChangedFn fn);

    void removeCablesForModIndex(uint8_t modIndex);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool hitTest(int x, int y) override;

private:
    struct DragState
    {
        bool active = false;
        bool pastThreshold = false;
        bool fromOutput = false;
        bool hadConnection = false;
        uint8_t modIndex = 0;
        uint8_t page = 0;
        uint8_t row = 0;
        juce::Point<float> startScreen;
        juce::Point<float> anchorScreen;
        juce::Point<float> cursorScreen;
    };

    uint8_t getModSource(uint8_t page, uint8_t row) const;
    void setModSource(uint8_t page, uint8_t row, uint8_t modIndex);
    void clearModSource(uint8_t page, uint8_t row);
    const OutputPort* hitOutputPort(juce::Point<float> screenPos) const;
    const InputPort* hitInputPort(juce::Point<float> screenPos) const;
    bool portContains(juce::Rectangle<float> bounds, juce::Point<float> screenPos) const;
    void drawCable(juce::Graphics& g,
                   juce::Point<float> fromScreen,
                   juce::Point<float> toScreen,
                   juce::Colour colour) const;
    void drawPortSocket(juce::Graphics& g,
                        juce::Rectangle<float> screenBounds,
                        juce::Colour ringColour,
                        bool isOutput) const;
    float hueForConnection(uint8_t page, uint8_t row) const;
    float assignCableHue(uint8_t page, uint8_t row);
    void removeCableHue(uint8_t page, uint8_t row);
    juce::Colour colourForConnection(uint8_t page, uint8_t row) const;
    juce::Colour colourFromHue(float hue) const;
    float randomHue() const;
    static int connectionKey(uint8_t page, uint8_t row);
    void notifyConnectionChanged(uint8_t page);
    void finishDrag(juce::Point<float> screenPos);
    void resetDrag();

    DesktopHostIO& m_host;
    DelayState& m_delay;
    std::vector<OutputPort> m_outputs;
    std::vector<InputPort> m_inputs;
    std::unordered_map<int, float> m_cableHues;
    DragState m_drag;
    std::optional<int> m_highlightInputKey;
    std::optional<uint8_t> m_highlightOutputMod;
    float m_dragCableHue = 0.0f;
    ConnectionChangedFn m_onConnectionChanged;
};
