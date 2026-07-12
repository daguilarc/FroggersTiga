#include "ui/EncoderRingComponent.hpp"

#include "ui/DesktopV2ChromeLayout.hpp"

#include <cmath>

namespace
{
constexpr float kPi = 3.14159265f;
constexpr float kStartAngle = kPi * 1.25f;
constexpr float kSweepAngle = kPi * 1.5f;

// D17: fixed-center MOD/CV LED hit-zone and visual radii (px). The hit-zone
// is intentionally larger than the drawn LED for a comfortable click target.
constexpr float kCenterModHitRadius = 9.0f;
constexpr float kCenterModLedRadius = 4.0f;

juce::Colour sceneLeftColour()
{
    return juce::Colour(0xff58a6ff);
}

juce::Colour sceneRightColour()
{
    return juce::Colour(0xffffa657);
}
} // namespace

EncoderRingComponent::EncoderRingComponent()
{
    setSize(DesktopV2ChromeLayout::kEncoderRingSize, DesktopV2ChromeLayout::kEncoderRowH);
}

void EncoderRingComponent::setSlot(uint8_t slot)
{
    m_slot = slot;
}

void EncoderRingComponent::refreshFromState(const froggers_v2::FroggersV2UIState& state)
{
    m_sceneLeft = state.sceneLeft[m_slot].load(std::memory_order_acquire);
    m_sceneRight = state.sceneRight[m_slot].load(std::memory_order_acquire);
    m_effective = state.effective[m_slot].load(std::memory_order_acquire);
    m_arcMin = state.arcMin[m_slot].load(std::memory_order_acquire);
    m_arcMax = state.arcMax[m_slot].load(std::memory_order_acquire);
    m_modMask = state.modulatorsMask[m_slot].load(std::memory_order_acquire);
    m_gestureMask = state.gesturesMask[m_slot].load(std::memory_order_acquire);
    repaint();
}

float EncoderRingComponent::valueToAngle(float value01) const
{
    return kStartAngle + kSweepAngle * juce::jlimit(0.0f, 1.0f, value01);
}

void EncoderRingComponent::paintArc(juce::Graphics& g,
                                    juce::Point<float> centre,
                                    float radius,
                                    float value01,
                                    juce::Colour colour,
                                    float strokeW) const
{
    juce::Path arc;
    arc.addCentredArc(centre.x,
                      centre.y,
                      radius,
                      radius,
                      0.0f,
                      kStartAngle,
                      valueToAngle(value01),
                      true);
    g.setColour(colour);
    g.strokePath(arc, juce::PathStrokeType(strokeW));
}

void EncoderRingComponent::paintBadges(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float badgeR = 3.5f;
    float x = bounds.getX() + 4.0f;
    const float modY = bounds.getY() + 3.0f;
    for (uint8_t i = 0; i < froggers_v2::kNumModSources; ++i)
    {
        if ((m_modMask & static_cast<uint16_t>(1u << i)) == 0)
        {
            continue;
        }
        g.setColour(juce::Colour(0xff79c0ff));
        g.fillEllipse(x, modY, badgeR * 2.0f, badgeR * 2.0f);
        x += badgeR * 2.0f + 2.0f;
    }

    x = bounds.getX() + 4.0f;
    const float gestureY = bounds.getBottom() - badgeR * 2.0f - 3.0f;
    for (uint8_t lane = 0; lane < froggers_v2::kNumGestures; ++lane)
    {
        if ((m_gestureMask & static_cast<uint8_t>(1u << lane)) == 0)
        {
            continue;
        }
        g.setColour(lane == 0 ? juce::Colour(0xffd2a8ff) : juce::Colour(0xfff778ba));
        g.fillEllipse(x, gestureY, badgeR * 2.0f, badgeR * 2.0f);
        x += badgeR * 2.0f + 2.0f;
    }
}

void EncoderRingComponent::refreshFromCrunchyState(const froggers_v2::FroggersV2UIState& state)
{
    m_sceneLeft = state.crunchySceneLeft.load(std::memory_order_acquire);
    m_sceneRight = state.crunchySceneRight.load(std::memory_order_acquire);
    m_effective = state.crunchyEffective.load(std::memory_order_acquire);
    m_arcMin = state.crunchyArcMin.load(std::memory_order_acquire);
    m_arcMax = state.crunchyArcMax.load(std::memory_order_acquire);
    m_modMask = 0;
    m_gestureMask = 0;
    repaint();
}

void EncoderRingComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float side = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const juce::Point<float> centre(bounds.getCentreX(), bounds.getCentreY());
    const float outerR = side * 0.4f;
    const float innerR = outerR * 0.72f;

    g.setColour(juce::Colour(0xff30363d));
    g.drawEllipse(centre.x - outerR, centre.y - outerR, outerR * 2.0f, outerR * 2.0f, 1.5f);

    if (m_arcMax > m_arcMin)
    {
        paintArc(g, centre, outerR + 2.0f, m_arcMax, juce::Colour(0xff8b949e).withAlpha(0.45f), 2.0f);
        paintArc(g, centre, outerR + 2.0f, m_arcMin, juce::Colour(0xff8b949e).withAlpha(0.45f), 2.0f);
    }

    paintArc(g, centre, outerR, m_sceneLeft, sceneLeftColour(), 3.0f);
    paintArc(g, centre, innerR, m_sceneRight, sceneRightColour(), 2.5f);

    const float dotR = 4.0f;
    const float dotAngle = valueToAngle(m_effective);
    const float dotX = centre.x + std::cos(dotAngle) * (outerR + innerR) * 0.5f;
    const float dotY = centre.y + std::sin(dotAngle) * (outerR + innerR) * 0.5f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);

    // D17: fixed-center MOD/CV LED, distinct from the bipolar effective-dot
    // above (which moves with m_effective/arc angle). Placeholder styling —
    // color/intensity encoding signed modulation bias is a manual-QA pass.
    g.setColour(juce::Colour(0xff2ea043).withAlpha(0.55f));
    g.fillEllipse(centre.x - kCenterModLedRadius,
                  centre.y - kCenterModLedRadius,
                  kCenterModLedRadius * 2.0f,
                  kCenterModLedRadius * 2.0f);

    paintBadges(g, bounds.reduced(2.0f));
}

bool EncoderRingComponent::hitsCenterModZone(juce::Point<float> position) const
{
    const auto bounds = getLocalBounds().toFloat();
    const juce::Point<float> centre(bounds.getCentreX(), bounds.getCentreY());
    return centre.getDistanceFrom(position) <= kCenterModHitRadius;
}

void EncoderRingComponent::mouseDown(const juce::MouseEvent& event)
{
    if (hitsCenterModZone(event.position))
    {
        // Center MOD/CV LED: the sole drill-in target (D17). Does not start
        // a drag session.
        if (onModDrillIn)
        {
            onModDrillIn(m_slot);
        }
        return;
    }

    // Ring annulus: click+drag turns the parameter. onPress still fires here
    // (unchanged from before this packet) because it is not itself a
    // mod-detail-opening action — only ModDrillIn opens mod detail (D17).
    // ParamPress remains the detail-grid Target(Back)/lane-clear-press
    // action (15-C1's onParamPress), and is a no-op when the detail grid is
    // closed, so reusing the same onPress signal for both the closed
    // module-row state and the open detail-grid state is correct here.
    m_dragging = true;
    m_dragStartY = static_cast<float>(event.position.y);
    if (onPress)
    {
        onPress(m_slot);
    }
}

void EncoderRingComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!m_dragging || !onTurn)
    {
        return;
    }
    const float deltaY = m_dragStartY - static_cast<float>(event.position.y);
    m_dragStartY = static_cast<float>(event.position.y);
    onTurn(m_slot, deltaY * 0.01f);
}

void EncoderRingComponent::mouseUp(const juce::MouseEvent&)
{
    m_dragging = false;
}
