#pragma once

#include "control/FroggersV2ControlCore.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstddef>
#include <functional>

class EncoderRingComponent : public juce::Component
{
public:
    EncoderRingComponent();

    void setSlot(uint8_t slot);
    // Packet 15.3: unavailable detail-grid lanes stay visible but greyed;
    // turn/press/drill-in are ignored while unavailable.
    void setLaneAvailable(bool available);
    bool laneAvailable() const { return m_laneAvailable; }
    void refreshFromState(const froggers_v2::FroggersV2UIState& state);
    void refreshFromCrunchyState(const froggers_v2::FroggersV2UIState& state);

    // Display-only CV activity underlay (non-owning ring view). Does not
    // affect ParamTurn / ModDrillIn hit geometry.
    void setUnderlay(const float* samples, size_t count, size_t writeIndex, juce::Colour colour);
    void clearUnderlay();
    bool hasUnderlay() const { return m_underlaySamples != nullptr && m_underlayCount > 0; }

    std::function<void(uint8_t slot, float delta)> onTurn;
    std::function<void(uint8_t slot)> onPress;
    // D17: the fixed-center MOD/CV LED is the sole click target that opens
    // parameter-detail modulation. Fired from mouseDown when the click point
    // falls within the center hit-zone; the ring annulus never fires this.
    std::function<void(uint8_t slot)> onModDrillIn;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    float valueToAngle(float value01) const;
    void paintArc(juce::Graphics& g,
                  juce::Point<float> centre,
                  float radius,
                  float value01,
                  juce::Colour colour,
                  float strokeW) const;
    void paintBadges(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintUnderlay(juce::Graphics& g,
                       juce::Rectangle<float> bounds,
                       float availabilityAlpha) const;
    // D17: fixed-radius hit-test at the component's geometric center,
    // independent of the ring's current value/arc position.
    bool hitsCenterModZone(juce::Point<float> position) const;

    uint8_t m_slot = 0;
    float m_sceneLeft = 0.0f;
    float m_sceneRight = 0.0f;
    float m_effective = 0.0f;
    float m_arcMin = 0.0f;
    float m_arcMax = 1.0f;
    uint16_t m_modMask = 0;
    float m_dragStartY = 0.0f;
    bool m_dragging = false;
    bool m_laneAvailable = true;
    const float* m_underlaySamples = nullptr;
    size_t m_underlayCount = 0;
    size_t m_underlayWriteIndex = 0;
    juce::Colour m_underlayColour{0xff79c0ff};
};
