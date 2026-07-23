// V2EnvelopeFollowerBank_test -- packet 12 (openspec/changes/
// desktop-v2-sheaf-runtime-harmonization, tasks.md 12.1/12.4, design.md
// D13/D14).
//
// D13: the dead lfo_1/2/3 permanent-rack taps (8-10) become the VCO
// envelope followers at a slow (LFO-rate) timescale -- the same VCO inputs
// V2EnvelopeFollowerBank already feeds taps 3-7 with, just slower
// coefficients. D14: this is V2-host-scoped; the fast bank (taps 3-7) must
// stay byte-for-byte unchanged, and the whole EF-tap path (fast + new slow)
// must only ever run when DesktopHostIO's V2 mod-tap layout is enabled
// (IsV2SimHostKind), never for Daisy/v1.
//
// This test exercises the DSP directly (V2EnvelopeFollowerBank +
// V2SlowEnvelopeFollowerBank against a PermanentModTapRack), and separately
// confirms the DesktopHostIO gate: a v1 (SimHostKind::Desktop) host processes
// audio through FroggersEngine without ever touching the permanent tap rack
// for the EF taps, while a V2 (SimHostKind::DesktopV2) host populates both
// tap ranges.

#include "DesktopHostIO.hpp"
#include "PermanentModTapRack.hpp"
#include "V2EnvelopeFollowerBank.hpp"

#include <cmath>
#include <cstdio>

namespace
{

bool test_fast_bank_unchanged()
{
    // Locks down the existing fast VCO/pair EF bank's exact coefficients and
    // tap targets (taps 3-7) so the new slow pass cannot perturb it.
    V2EnvelopeFollowerBank fast;
    fast.setSampleRate(44100.0f);

    const float expectedAttack = 1.0f - std::exp(-1.0f / (0.01f * 44100.0f));
    const float expectedRelease = 1.0f - std::exp(-1.0f / (0.05f * 44100.0f));
    if (std::fabs(fast.m_attackCoeff - expectedAttack) > 1e-9f
        || std::fabs(fast.m_releaseCoeff - expectedRelease) > 1e-9f)
    {
        std::printf("FAIL: fast EF bank attack/release coefficients changed\n");
        return false;
    }

    PermanentModTapRack taps;
    for (int i = 0; i < 200; ++i)
    {
        fast.Process(0.9f, -0.6f, 0.3f, taps);
    }

    // vco_1_ef=3, vco_2_ef=4, vco_3_ef=5, vco_12_ef=6, vco_23_ef=7.
    if (taps.GetTap(3) <= 0.0f || taps.GetTap(4) <= 0.0f || taps.GetTap(5) <= 0.0f
        || taps.GetTap(6) <= 0.0f || taps.GetTap(7) <= 0.0f)
    {
        std::printf("FAIL: fast EF bank taps 3-7 did not populate\n");
        return false;
    }
    return true;
}

bool test_slow_bank_populates_lfo_ef_taps()
{
    V2SlowEnvelopeFollowerBank slow;
    slow.setSampleRate(44100.0f);

    PermanentModTapRack taps;
    for (int i = 0; i < 4000; ++i)
    {
        slow.Process(0.9f, -0.6f, 0.3f, taps);
    }

    // lfo_1=8, lfo_2=9, lfo_3=10.
    if (taps.GetTap(8) <= 0.0f || taps.GetTap(9) <= 0.0f || taps.GetTap(10) <= 0.0f)
    {
        std::printf("FAIL: slow EF bank taps 8-10 did not populate\n");
        return false;
    }
    return true;
}

bool test_slow_bank_lags_fast_bank()
{
    // Same VCO inputs into both banks; after a modest number of samples the
    // slow (LFO EF) tap must have moved less far toward the step target than
    // the fast (VCO EF) tap -- proving the slow pass is actually slower, not
    // just a relabeled copy.
    V2EnvelopeFollowerBank fast;
    V2SlowEnvelopeFollowerBank slow;
    fast.setSampleRate(44100.0f);
    slow.setSampleRate(44100.0f);

    PermanentModTapRack taps;
    constexpr int kSamples = 500;
    for (int i = 0; i < kSamples; ++i)
    {
        fast.Process(0.8f, 0.8f, 0.8f, taps);
        slow.Process(0.8f, 0.8f, 0.8f, taps);
    }

    const float fastLevel = taps.GetTap(3);
    const float slowLevel = taps.GetTap(8);
    if (!(slowLevel < fastLevel))
    {
        std::printf("FAIL: slow EF tap (%.6f) did not lag fast EF tap (%.6f)\n",
                    static_cast<double>(slowLevel),
                    static_cast<double>(fastLevel));
        return false;
    }
    if (slowLevel <= 0.0f)
    {
        std::printf("FAIL: slow EF tap should still be rising, not stuck at 0\n");
        return false;
    }
    return true;
}

bool test_v1_desktop_host_leaves_lfo_ef_taps_dead()
{
    // D14: a v1 (SimHostKind::Desktop) host must never populate the
    // permanent tap rack -- fast or slow -- since GetCvOut() for a V2 host
    // reads m_v2ModTaps, but a v1 host reads m_pageManager.m_modMgr instead
    // (PermanentModTapRack.hpp GetSimCvOut). Confirms the byte-for-byte
    // Daisy/v1 CV path is unaffected by this change.
    DesktopHostIO host;
    host.m_hostKind = SimHostKind::Desktop;
    host.Init();
    host.SetSampleRate(44100.0f);

    static constexpr size_t kBlock = 512;
    float in[kBlock]{};
    float out[kBlock]{};
    for (size_t i = 0; i < kBlock; ++i)
    {
        in[i] = std::sin(static_cast<float>(i) * 0.05f);
    }
    for (int pass = 0; pass < 20; ++pass)
    {
        host.ProcessBlock(in, out, kBlock);
    }

    if (host.m_v2ModTaps.GetTap(3) != 0.0f || host.m_v2ModTaps.GetTap(8) != 0.0f)
    {
        std::printf("FAIL: v1 Desktop host must never write the V2 permanent tap rack\n");
        return false;
    }
    return true;
}

bool test_v2_desktop_host_populates_both_ef_ranges()
{
    DesktopHostIO host;
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);

    static constexpr size_t kBlock = 512;
    float in[kBlock]{};
    float out[kBlock]{};
    for (size_t i = 0; i < kBlock; ++i)
    {
        in[i] = std::sin(static_cast<float>(i) * 0.05f);
    }
    for (int pass = 0; pass < 20; ++pass)
    {
        host.ProcessBlock(in, out, kBlock);
    }

    if (host.GetCvOut(3) <= 0.0f)
    {
        std::printf("FAIL: DesktopV2 host did not populate fast VCO EF tap 3\n");
        return false;
    }
    if (host.GetCvOut(8) <= 0.0f || host.GetCvOut(9) <= 0.0f || host.GetCvOut(10) <= 0.0f)
    {
        std::printf("FAIL: DesktopV2 host did not populate slow LFO EF taps 8-10\n");
        return false;
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_fast_bank_unchanged() && ok;
    ok = test_slow_bank_populates_lfo_ef_taps() && ok;
    ok = test_slow_bank_lags_fast_bank() && ok;
    ok = test_v1_desktop_host_leaves_lfo_ef_taps_dead() && ok;
    ok = test_v2_desktop_host_populates_both_ef_ranges() && ok;

    if (!ok)
    {
        std::printf("FAIL: V2EnvelopeFollowerBank_test\n");
        return 1;
    }
    std::printf("PASS: V2EnvelopeFollowerBank_test\n");
    return 0;
}
