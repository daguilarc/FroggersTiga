// Regression coverage for task 4.4's locked preset silent-drop.
//
// Desktop-v2 deleted the Random S&H module page, but the C2 seam keeps the shared
// engine Marbles page at PM index 1 to drive the surviving Random S&H 1/2 mod lanes.
// The shared raw SimPresetSnapshot blob restores every PM page verbatim, so a
// pre-deletion preset would write stale Random bag values (Step chance / Deja vu /
// Bag size / Slew) plus mod routes straight into that still-live page -- which
// Marbles::UpdateParams reads every block. The desktop-v2 load path funnels every
// restore through AudioEngine::notifyStateRestored() -> dropDeletedRandomPageAxes()
// -> Marbles::ResetPageToDefaults(). This test drives the real SimPresetSnapshot read
// and asserts that exact reset drops the deleted axes back to Sheaf-style defaults.
//
// It also proves the shared SimPresetSnapshot itself is unchanged (v1 still restores
// PM page 1 verbatim): the "precondition" block below shows the raw read DID land the
// stale values on the engine -- the drop is a desktop-v2 step layered on top, not a
// change to the shared reader.

#include "DesktopHostIO.hpp"
#include "DelayState.hpp"
#include "SimPresetSnapshot.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
bool nearlyEqual(float a, float b, float eps = 1.0e-5f)
{
    return std::fabs(a - b) <= eps;
}

constexpr uint8_t kMarblesPmPage = 1; // Audio 0, Marbles 1, Reverb 2, Filter 3, Drive 4, ADSR 5.
constexpr uint8_t kBagRows = 7;
} // namespace

int main()
{
    DesktopHostIO host;
    DelayState delay;
    host.m_hostKind = SimHostKind::DesktopV2;
    host.setDelayState(&delay);
    host.Init();
    delay.init(44100.0f);

    PageManager& pm = host.m_pageManager;
    const float kDefaults[kBagRows] = {1.0f, 0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f};

    // Simulate a pre-packet-4 preset: stale bag values + a live mod route on the
    // Marbles page (rows 0-6, the ones Marbles::UpdateParams reads).
    for (uint8_t row = 0; row < kBagRows; ++row)
    {
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_knobValue = 0.777f;
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_modIndex = 3;
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_modAmount = 0.6f;
    }

    // Serialize -> the shared raw blob now carries the deleted Random-page axes.
    std::vector<uint8_t> blob(SimPresetSnapshot::serializedSize(), 0);
    if (!SimPresetSnapshot::write(host, delay, blob.data(), blob.size()))
    {
        std::printf("FAIL: SimPresetSnapshot::write failed\n");
        return 1;
    }

    // Wipe the live page to a sentinel, then read the blob back: proves the restored
    // values come from the blob (the load path), not leftover engine state.
    for (uint8_t row = 0; row < kBagRows; ++row)
    {
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_knobValue = 0.123f;
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_modIndex = 255;
        pm.m_pages[kMarblesPmPage].m_parameters[row].m_modAmount = 0.0f;
    }
    if (!SimPresetSnapshot::read(host, delay, blob.data(), blob.size()))
    {
        std::printf("FAIL: SimPresetSnapshot::read failed\n");
        return 1;
    }

    // Precondition (and shared-reader-unchanged proof): the raw snapshot read landed
    // the stale bag values + mod route straight on the live engine page. Without the
    // desktop-v2 drop this is what the DSP would run on -- the bug task 4.4 fixes.
    if (!nearlyEqual(pm.m_pages[kMarblesPmPage].m_parameters[0].m_knobValue, 0.777f)
        || pm.m_pages[kMarblesPmPage].m_parameters[0].m_modIndex != 3)
    {
        std::printf("FAIL: precondition -- raw SimPresetSnapshot::read did not restore the Marbles page\n");
        return 1;
    }

    // The exact reset AudioEngine::dropDeletedRandomPageAxes() performs after every read.
    const uint8_t dropped = host.m_engine.m_marbles.ResetPageToDefaults();
    if (dropped != kBagRows)
    {
        std::printf("FAIL: expected %u dropped Marbles rows, got %u\n", kBagRows, dropped);
        return 1;
    }

    // The deleted Random-page axes are gone: knobs back at Sheaf-style defaults, mods cleared.
    for (uint8_t row = 0; row < kBagRows; ++row)
    {
        const Parameter& p = pm.m_pages[kMarblesPmPage].m_parameters[row];
        if (!nearlyEqual(p.m_knobValue, kDefaults[row]))
        {
            std::printf("FAIL: Marbles row %u knob expected %f got %f (stale preset axis not dropped)\n",
                        row, kDefaults[row], p.m_knobValue);
            return 1;
        }
        if (p.m_modIndex != 255 || !nearlyEqual(p.m_modAmount, 0.0f))
        {
            std::printf("FAIL: Marbles row %u mod route not cleared (idx=%u amt=%f)\n",
                        row, static_cast<unsigned>(p.m_modIndex), p.m_modAmount);
            return 1;
        }
    }

    // And the live Random S&H DSP now reads the Sheaf-style defaults.
    Marbles& marbles = host.m_engine.m_marbles;
    marbles.UpdateParams();
    if (!nearlyEqual(marbles.m_probability, 1.0f)
        || marbles.m_size[0] != Marbles::x_numMarbles || marbles.m_size[1] != Marbles::x_numMarbles
        || !nearlyEqual(marbles.m_dejaVuKnob[0], 0.5f) || !nearlyEqual(marbles.m_dejaVuKnob[1], 0.5f))
    {
        std::printf("FAIL: Marbles DSP did not fall back to defaults after drop "
                    "(prob=%f size=%u/%u dejavu=%f/%f)\n",
                    marbles.m_probability,
                    static_cast<unsigned>(marbles.m_size[0]),
                    static_cast<unsigned>(marbles.m_size[1]),
                    marbles.m_dejaVuKnob[0],
                    marbles.m_dejaVuKnob[1]);
        return 1;
    }

    std::printf("PresetRandomPageDrop_test OK\n");
    return 0;
}
