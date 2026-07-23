// Task 7.4 (D11/D12/D14): remove the cross-coupler + self-contained PM,
// flag-gated to the V2 hosts (desktop-v2 + web), Daisy/v1 untouched.
//
// Three things are asserted here:
//  1. Flag OFF (Daisy/v1, SimHostKind::Desktop never sets the flag): the
//     legacy coupler-gated StepOscillators path is byte-for-byte unchanged.
//     Proven by pinning literal output samples captured from the engine
//     BEFORE this packet's DSP change landed (same knob/gate setup, same
//     build) -- if the refactor had altered so much as one operator in the
//     legacy branch, these would drift outside the tight epsilon below.
//  2. Flag ON (V2 hosts): the coupler is fully neutralized -- driving the
//     (still-allocated, UI-hidden) XCPL param to its extremes produces
//     IDENTICAL output, proving no cross-VCO term survives under the flag.
//  3. Flag ON: PM is NOT inert -- two different PM1A knob settings (with
//     XCPL, PM2A, PM3 held fixed) produce different output, proving the
//     self-contained sine-LFO phase modulation is real and audible.
//     (Contrast with flag OFF, where the coupler is still what gates PM: at
//     the default center XCPL knob, PM has zero effect -- unaffected by this
//     packet, since flag OFF is provably unchanged; see check 1.)
//  4. Flag ON: PM zero-off (D14, operator 2026-07-23) -- the knob's minimum
//     position (and a small floor above it) produce EXACTLY zero phase-mod
//     depth (bit-identical output across the whole sub-floor sweep, not just
//     "quiet"), and comfortably above the floor PM is clearly active again.

#include "PagedHostIO.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
constexpr size_t kBlock = 4096;
constexpr size_t kBlockV2 = 8192;

void seedAudioKnobs(PagedHostIO& io, float xcpl, float pm1, float pm2, float pm3)
{
    io.SetPageKnob(0, 0, 0.5f);   // V1VO
    io.SetPageKnob(0, 1, 0.55f);  // V2VO
    io.SetPageKnob(0, 2, 0.6f);   // V3VO
    io.SetPageKnob(0, 3, xcpl);   // XCPL (engine slot retained; UI-hidden on V2)
    io.SetPageKnob(0, 4, pm1);    // PM1A
    io.SetPageKnob(0, 5, pm2);    // PM2A
    io.SetPageKnob(0, 6, pm3);    // OLVL (legacy) / PM3 (V2 dedicated knob)
    io.SetPageKnob(0, 7, 0.5f);   // Crispy/FUEG (also legacy PM3 fallback source)
}

double rmsDiff(const std::vector<float>& a, const std::vector<float>& b)
{
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        const double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<double>(a.size()));
}

bool nearlyEqual(float a, float b, float epsilon)
{
    return std::fabs(a - b) <= epsilon;
}
} // namespace

static bool test_flag_off_legacy_path_byte_for_byte_unchanged()
{
    static constexpr float kEpsilon = 1.0e-6f;
    static constexpr float kGoldenOut0 = 0.0f;
    static constexpr float kGoldenOut1 = 0.0f;
    static constexpr float kGoldenOut7 = 0.0f;
    static constexpr float kGoldenOut100 = 0.0f;
    static constexpr float kGoldenOut1000 = 0.028242629f;
    static constexpr float kGoldenOut4095 = -0.00735413609f;
    static constexpr double kGoldenRms = 0.0217164073;

    std::vector<float> input(kBlock, 0.0f);
    std::vector<float> output(kBlock);

    PagedHostIO io;
    io.m_hostKind = SimHostKind::Desktop; // Daisy/v1 legacy: flag never set.
    io.Init();
    // Parameter pickup ("pot tracking", src/core/Parameter.hpp KnobUpdate) only
    // applies a SetPageKnob write once a param is in Tracking state; force it so
    // the seeded knob values actually take effect (mirrors DesktopHostIO::Init(),
    // which calls this for the real host -- PagedHostIO does not by default).
    io.m_pageManager.SetAllParamsTracking();
    io.SetSampleRate(44100.0f);
    seedAudioKnobs(io, 0.8f, 0.7f, 0.65f, 0.5f);
    io.ProcessBlock(input.data(), output.data(), kBlock);

    double sumSq = 0.0;
    for (float sample : output)
    {
        sumSq += static_cast<double>(sample) * sample;
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(kBlock));

    bool ok = true;
    auto check = [&](const char* label, float actual, float expected) {
        if (!nearlyEqual(actual, expected, kEpsilon))
        {
            std::printf("FAIL: legacy %s expected %.9g got %.9g\n", label, expected, actual);
            ok = false;
        }
    };
    check("out[0]", output[0], kGoldenOut0);
    check("out[1]", output[1], kGoldenOut1);
    check("out[7]", output[7], kGoldenOut7);
    check("out[100]", output[100], kGoldenOut100);
    check("out[1000]", output[1000], kGoldenOut1000);
    check("out[4095]", output[4095], kGoldenOut4095);
    if (std::fabs(rms - kGoldenRms) > 1.0e-6)
    {
        std::printf("FAIL: legacy rms expected %.9g got %.9g\n", kGoldenRms, rms);
        ok = false;
    }
    return ok;
}

static bool test_flag_on_coupler_fully_neutralized()
{
    std::vector<float> input(kBlockV2, 0.0f);
    std::vector<float> outXcplLow(kBlockV2);
    std::vector<float> outXcplHigh(kBlockV2);

    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.0f, 0.7f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outXcplLow.data(), kBlockV2);
    }
    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 1.0f, 0.7f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outXcplHigh.data(), kBlockV2);
    }

    double sumSq = 0.0;
    for (float sample : outXcplLow)
    {
        sumSq += static_cast<double>(sample) * sample;
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(kBlockV2));
    if (rms < 1.0e-4)
    {
        std::printf("FAIL: flag-on output is unexpectedly near-silent (rms=%.9g); test is not exercising audio\n", rms);
        return false;
    }

    const double diff = rmsDiff(outXcplLow, outXcplHigh);
    if (diff > 1.0e-7)
    {
        std::printf("FAIL: flag-on XCPL extremes should be fully neutralized, got diffRms=%.9g\n", diff);
        return false;
    }
    return true;
}

// D14 zero-off (operator 2026-07-23): the PM knob's minimum position must be
// PM fully OFF. Below/at a small floor (mirrors FroggersEngine.hpp's
// x_pmLfoFloor -- not exposed publicly, so hardcoded here; keep in sync) the
// phase-mod depth must be EXACTLY zero, regardless of the differing LFO
// frequency each sub-floor knob value maps to. Proof: sweep PM1A across
// several distinct sub-floor values (0.0, 0.01, the floor itself) with PM2/PM3
// held at an audibly-active setting; if depth were merely small instead of
// exactly zero, differing frequencies would still produce differing output.
// Identical output across the whole sub-floor sweep is the only way to show
// depth == 0, not just "quiet."
static bool test_flag_on_pm_inert_at_knob_minimum()
{
    static constexpr float kFloor = 0.02f; // mirrors x_pmLfoFloor

    std::vector<float> input(kBlockV2, 0.0f);
    std::vector<float> outAtZero(kBlockV2);
    std::vector<float> outAtNearZero(kBlockV2);
    std::vector<float> outAtFloor(kBlockV2);

    auto render = [&](float pm1, std::vector<float>& out) {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.5f, pm1, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), out.data(), kBlockV2);
    };

    render(0.0f, outAtZero);
    render(0.01f, outAtNearZero);
    render(kFloor, outAtFloor);

    bool ok = true;
    const double diffNearZero = rmsDiff(outAtZero, outAtNearZero);
    if (diffNearZero > 1.0e-7)
    {
        std::printf(
            "FAIL: PM knob at 0.0 vs 0.01 (both sub-floor) should be bit-identical (depth==0), got diffRms=%.9g\n",
            diffNearZero);
        ok = false;
    }
    const double diffAtFloor = rmsDiff(outAtZero, outAtFloor);
    if (diffAtFloor > 1.0e-7)
    {
        std::printf(
            "FAIL: PM knob at 0.0 vs floor (%.3g) should be bit-identical (depth==0), got diffRms=%.9g\n",
            kFloor,
            diffAtFloor);
        ok = false;
    }
    return ok;
}

// Complements test_flag_on_pm_inert_at_knob_minimum: comfortably above the
// floor, PM must be clearly active again (not stuck at zero forever).
static bool test_flag_on_pm_active_above_floor()
{
    std::vector<float> input(kBlockV2, 0.0f);
    std::vector<float> outAtFloor(kBlockV2);
    std::vector<float> outAboveFloor(kBlockV2);

    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.5f, 0.02f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outAtFloor.data(), kBlockV2);
    }
    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.5f, 0.5f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outAboveFloor.data(), kBlockV2);
    }

    const double diff = rmsDiff(outAtFloor, outAboveFloor);
    if (diff < 1.0e-4)
    {
        std::printf("FAIL: PM well above the floor should be clearly active vs at-floor, got diffRms=%.9g\n", diff);
        return false;
    }
    return true;
}

static bool test_flag_on_pm_is_not_inert()
{
    std::vector<float> input(kBlockV2, 0.0f);
    std::vector<float> outPmLow(kBlockV2);
    std::vector<float> outPmHigh(kBlockV2);

    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.5f, 0.05f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outPmLow.data(), kBlockV2);
    }
    {
        PagedHostIO io;
        io.m_hostKind = SimHostKind::DesktopV2;
        io.Init();
        io.m_pageManager.SetAllParamsTracking(); // see note in test 1 above
        io.SetSampleRate(44100.0f);
        io.SetGate(true);
        seedAudioKnobs(io, 0.5f, 0.95f, 0.65f, 0.5f);
        io.ProcessBlock(input.data(), outPmHigh.data(), kBlockV2);
    }

    const double diff = rmsDiff(outPmLow, outPmHigh);
    if (diff < 1.0e-4)
    {
        std::printf("FAIL: PM1A should audibly change output under flag-on, got diffRms=%.9g\n", diff);
        return false;
    }
    return true;
}

int main()
{
    int fails = 0;
    if (!test_flag_off_legacy_path_byte_for_byte_unchanged())
    {
        ++fails;
    }
    if (!test_flag_on_coupler_fully_neutralized())
    {
        ++fails;
    }
    if (!test_flag_on_pm_is_not_inert())
    {
        ++fails;
    }
    if (!test_flag_on_pm_inert_at_knob_minimum())
    {
        ++fails;
    }
    if (!test_flag_on_pm_active_above_floor())
    {
        ++fails;
    }

    if (fails != 0)
    {
        std::printf("V2IndependentPm_test FAIL (%d)\n", fails);
        return 1;
    }
    std::printf("V2IndependentPm_test OK\n");
    return 0;
}
