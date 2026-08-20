// FroggersAudioRoutingTests.cpp -- the real audio path. Proves
// FroggersApp::ProcessBlock produces non-silent, finite stereo output for
// (a) a deliberately non-default patch and (b) the D16 default patch
// untouched, while the master clock's transport is running -- explicitly
// stronger than the finiteness check in FroggersHeadlessTests.cpp,
// which silence also satisfies -- and produces silence while the transport
// is stopped, including immediately after Init()/PrepareToPlay() with the
// transport never started (the ASR gate follows the transport's
// quarter-note pulse, not any note-on/off or permanently-held gate). Also
// exercises the output-safety requirement directly: the Filter
// bank's Comb pushed to its self-oscillating feedback extreme, plus the
// Reverb bank's Hold pushed to its ceiling, must still leave the output
// finite and bounded.
//
// Runs via synth_rig::SynthRig<FroggersApp> (External/Sheaf's
// tests/support/SynthRig.hpp), same JUCE-free headless harness as
// FroggersHeadlessTests.cpp and FroggersParameterModelTests.cpp;
// wired into app/Makefile's `test` target (nice make -j2 test). The rig's
// transport starts Stopped by default (synth::ClockTransportState::Stopped)
// until `rig.StartAt(...)` is called, which is exactly the state this file's
// silence tests rely on.

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers audio-routing tests must not see JUCE headers -- the app core must stay JUCE-free"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

struct TestCase {
    const char* name;
    void (*fn)();
};

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Register {
    Register(const char* name, void (*fn)()) {
        Registry().push_back({name, fn});
    }
};

#define TEST_CASE(name)     \
    void name();            \
    Register reg_##name(#name, &name); \
    void name()

#define REQUIRE_TRUE(expr)                                                 \
    do {                                                                   \
        if (!(expr)) {                                                    \
            std::ostringstream oss;                                       \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #expr; \
            throw std::runtime_error(oss.str());                          \
        }                                                                  \
    } while (false)

// Fresh scratch runtime data paths per test, mirroring
// FroggersHeadlessTests.cpp's UseScratchRuntimeDataPaths, so startup patch
// loading never observes a shared production location or another test's
// data.
synth::RuntimeDataPaths UseScratchRuntimeDataPaths(const char* testName) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-audio-routing-tests" / testName;
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;
namespace dsp = synth_froggers::dsp;

// B7.5.0: SceneCenter writes are applied through a SMOOTHED periodic Compute
// (Parameter::ProcessSamplePhase1, alpha 0.0994 every 16 samples), so a patch
// is only ~81% applied one block after it is written. ComputeAllParameters()
// passes smoothTargetCenter=false and therefore converges exactly, in one
// call. Call this after the SceneCenter writes and before the first
// RunBlocks() whenever a test asserts on a patch rather than on a ramp.
//
// Route verified against FroggersAppCore.hpp (M1): `context_` is private
// with no public accessor, so there is no `rig.Application().Context()` to
// call through. `TestParameterManager()` (FroggersAppCore.hpp, beside
// TestOutputLimiter()) is the narrow test-only accessor added for this.
inline void ApplyPatchNow(Rig& rig) {
    rig.Application().TestParameterManager().ComputeAllParameters();
}

// Peak absolute magnitude across every captured channel sample -- asserts
// some output sample exceeds a small epsilon in magnitude, which plain
// finiteness alone does not require.
float PeakAbs(const std::vector<Rig::OutputFrame>& frames) {
    float peak = 0.0f;
    for (const auto& frame : frames) {
        for (const float sample : frame.channels) {
            peak = std::max(peak, std::fabs(sample));
        }
    }
    return peak;
}

// Counts rising "gate opened" edges (channel 0's magnitude crossing above
// a small threshold from below) across captured output -- used to prove
// the ASR gate's cycle rate tracks tempo without needing to decode the
// exact envelope shape.
std::size_t CountRisingEdges(const std::vector<Rig::OutputFrame>& frames) {
    constexpr float kThreshold = 1.0e-3f;
    std::size_t count = 0;
    bool wasOpen = false;
    for (const auto& frame : frames) {
        const bool isOpen = std::fabs(frame.channels[0]) > kThreshold;
        if (isOpen && !wasOpen) {
            ++count;
        }
        wasOpen = isOpen;
    }
    return count;
}

void RequireFiniteStereo(const std::vector<Rig::OutputFrame>& frames) {
    REQUIRE_TRUE(!frames.empty());
    for (const auto& frame : frames) {
        REQUIRE_TRUE(frame.channels.size() == 2);  // Config() requests stereo out.
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::isfinite(sample));
        }
    }
}

// Settle/check-
// window silence-measurement scaffolding, extracted after it appeared a
// third near-byte-identical time (the Grit stopped-state test) -- a
// concept repeating a third time is what turns two pre-existing
// near-identical blocks into duplication worth naming and sharing. Every caller only varies HOW
// LONG to wait before measuring (settleSeconds); sample rate, block size,
// the trailing check-window length, and the silence floor are the same
// fixed values at all 3 call sites (FroggersApp::Config() sets 48 kHz/
// 256-sample blocks, FroggersAppCore.hpp:196-197; -60 dBFS: 20*log10(x) =
// -60 -> x = 1.0e-3), so only settleSeconds is a parameter here.
//
// Deliberately does NOT also fold in the RunBlocks/ClearOutput/PeakAbs/
// REQUIRE_TRUE sequence that follows each call site -- that part is each
// test's own narrative and differs materially between callers (2 windows
// here, 3 windows -- one of them a distinct "afterStop" concept -- in the
// long-release test, an intervening Freeze-button press in the Freeze-latch
// test), so folding it in would gut those per-test assertions.
// Only the declaration block that derives checkWindowBlocks/settleLeadBlocks
// is shared.
struct SilenceSettleWindow {
    std::size_t settleLeadBlocks;
    std::size_t checkWindowBlocks;
    float silenceFloorLinear;
};

SilenceSettleWindow ComputeSilenceSettleWindow(double settleSeconds) {
    constexpr double kSampleRateHz = 48000.0;
    constexpr double kBlockSizeSamples = 256.0;
    constexpr double kCheckWindowSeconds = 0.02;  // ~20 ms trailing measurement window.
    const auto checkWindowBlocks =
        static_cast<std::size_t>(std::ceil((kCheckWindowSeconds * kSampleRateHz) / kBlockSizeSamples));
    const auto settleLeadBlocks =
        static_cast<std::size_t>(std::ceil((settleSeconds * kSampleRateHz) / kBlockSizeSamples)) -
        checkWindowBlocks;
    constexpr float kSilenceFloorLinear = 1.0e-3f;  // -60 dBFS.
    return {settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear};
}

// -----------------------------------------------------------------------
// Band-limited energy check (6.2's mandatory "not RMS" assertion) -- a
// naive single-frequency Goertzel evaluated at arbitrary (non-bin-aligned)
// frequencies over a fixed-length window. Standard Goertzel recurrence
// (e.g. https://en.wikipedia.org/wiki/Goertzel_algorithm's "power" form):
//   coeff = 2*cos(2*pi*f/fs)
//   s0 = x[n] + coeff*s1 - s2 ; s2 = s1 ; s1 = s0            (per sample)
//   power = s1^2 + s2^2 - coeff*s1*s2                        (after N samples)
// This is exactly a length-N DFT correlation at frequency f, so it is valid
// for ANY f (not just fs*k/N bins) -- the "off-bin" cost is ordinary
// spectral leakage, irrelevant here since this test only needs "is there
// energy in this band", not a narrowband measurement.
double GoertzelPower(const std::vector<float>& samples, double freqHz, double sampleRateHz) {
    const double coeff = 2.0 * std::cos(2.0 * M_PI * freqHz / sampleRateHz);
    double s1 = 0.0, s2 = 0.0;
    for (const float sample : samples) {
        const double s0 = static_cast<double>(sample) + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return s1 * s1 + s2 * s2 - coeff * s1 * s2;
}

// `BandEnergy` (a summed sweep over freqStartHz..freqEndHz) used to live
// here for the pre-task-6.12 version of
// default_patch_has_audible_band_energy_above_150hz; that test now sums
// `GoertzelPower` directly at specific fundamentals instead (see its own
// comment), leaving `BandEnergy` with no caller, so it was removed.

std::vector<float> ExtractChannel(const std::vector<Rig::OutputFrame>& frames, std::size_t channel) {
    std::vector<float> samples;
    samples.reserve(frames.size());
    for (const auto& frame : frames) {
        samples.push_back(frame.channels[channel]);
    }
    return samples;
}

// -----------------------------------------------------------------------
// The default patch (D16) must produce audio: a freshly started app
// should make sound with no user input. D16 sets three VCO Shapes, six
// cross-VCO modulation depths, and Drive to 20% -- all applied once from
// FroggersApp::Init() via ApplyFroggersDefaultPatch(), with no test-side
// parameter mutation here at all.
// -----------------------------------------------------------------------
TEST_CASE(default_patch_produces_non_silent_finite_audio) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("default_patch"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // Confirm this really is D16's default patch before trusting the
    // energy assertion below to mean anything -- Drive (Drive bank, slot 0)
    // reads 20% of range, and at least one cross-VCO modulation depth is
    // materialized (ApplyFroggersDefaultPatch is called unconditionally
    // from FroggersApp::Init()).
    const synth::Parameter& driveParam = model.PageParameter(synth_froggers::FroggersBankId::Drive, 0);
    REQUIRE_TRUE(std::fabs(driveParam.SceneCenter(0) - 0.2f) < 1e-5f);
    const synth::Parameter& vco1Pitch = model.PageParameter(synth_froggers::FroggersBankId::Audio, 0);
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(synth_froggers::kModSlotVco2Audio) != nullptr);

    // The ASR gate follows the transport's quarter-note pulse, so the
    // transport must actually be running for any of this test's energy
    // assertions to mean anything.
    rig.StartAt(0);

    // Let scene-blend/fuego/modulation settle (kDefaultProcessLiteAlpha is a
    // ~1 kHz one-pole at 48 kHz, so a couple of blocks is generous margin),
    // then measure a fresh window of output.
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(4);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);

    constexpr float kEpsilon = 1.0e-4f;
    REQUIRE_TRUE(PeakAbs(output) > kEpsilon);
}

// -----------------------------------------------------------------------
// The default patch must be audible on laptop speakers,
// not merely nonzero. If All three Audio-bank pitch knobs (Audio bank slots
// 0-2) registered at the ordinary 0.0f default (FroggersParameters.hpp's own
// FroggersBankLayouts(), Audio row), then, since
// Vco::PitchToPhaseIncrement(0, sr) = ExpMapCompute(20/sr, 20000/sr, 0)
// (app/dsp/Vco.hpp), every VCO would free-run at 20 Hz --
// inaudible on a MacBook Air, and NOT caught by a plain RMS/PeakAbs
// check (a 20 Hz tone is nonzero-RMS). This test instead asserts Goertzel
// power at the EXPECTED audible fundamentals (110/220/330 Hz) dominates
// power at the inaudible ones a 0.0f default would give (20/40/60 Hz) -- a
// ratio check needing
// no calibrated magic constant
// (see the assertion's own comment below for a prior, more fragile version).
// -----------------------------------------------------------------------
TEST_CASE(default_patch_has_audible_band_energy_above_150hz) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("default_patch_band_energy"));

    // The ASR gate only opens while the transport runs, so the transport
    // must be started for this to measure anything.
    rig.StartAt(0);

    // Let scene-blend/fuego/modulation settle, then capture a fresh window
    // long enough to resolve ~150 Hz content with a non-bin-aligned
    // Goertzel sweep (4096 samples @ 48 kHz = ~85 ms, comfortably inside
    // the ASR gate's open half of a 120 BPM quarter note -- 12000 samples --
    // started at rig.StartAt(0) above).
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(16);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);

    const std::vector<float> left = ExtractChannel(output, 0);
    // FroggersApp::Config() (app/FroggersAppCore.hpp:196) sets
    // preferredSampleRate = 48000.0, and this Rig() ctor call above passes
    // no override AudioSettings, so SynthRig negotiates that same rate
    // (SynthRig.hpp:68-70).
    constexpr double kSampleRateHz = 48000.0;

    // The prior version summed Goertzel power over a
    // 150 Hz-2000 Hz sweep and compared it to a calibrated constant
    // (1.0e5, ~5x margin from measured pre-fix/post-fix runs of ~2.0e4/
    // ~5.1e5) -- fragile because a future change to the Shape/Drive
    // defaults could drift the absolute figure with only a thin margin to
    // catch it. This version instead sums Goertzel power at the three
    // EXPECTED post-fix fundamentals (110/220/330 Hz, the Audio bank's
    // three VCOs at their ordinary 0.0f pitch default) and at the three
    // BROKEN pre-fix ones (20/40/60 Hz, what `Vco::PitchToPhaseIncrement`
    // collapses to without the fix) and asserts the expected fundamentals
    // dominate by a ratio, not an absolute threshold -- true regardless of
    // exactly how much total energy either patch config carries.
    constexpr std::array<double, 3> kExpectedFundamentalsHz = {110.0, 220.0, 330.0};
    constexpr std::array<double, 3> kBrokenFundamentalsHz = {20.0, 40.0, 60.0};

    double expectedPower = 0.0;
    for (const double freqHz : kExpectedFundamentalsHz) {
        expectedPower += GoertzelPower(left, freqHz, kSampleRateHz);
    }
    double brokenPower = 0.0;
    for (const double freqHz : kBrokenFundamentalsHz) {
        brokenPower += GoertzelPower(left, freqHz, kSampleRateHz);
    }

    // Ratio, not a magic constant: the previously measured ~25x separation
    // between pre-fix and post-fix band energy (see the comment this one
    // replaced) was over a whole 150 Hz-2000 Hz sweep, which includes
    // considerable harmonic leakage from the broken 20 Hz fundamentals
    // (saw/square harmonics from Audio bank slots 4-5's Shape defaults,
    // D16); measuring directly AT the fundamentals themselves (rather than
    // a swept band that partially captures both) should separate them by
    // at least as much. 10x is picked as a threshold comfortably inside
    // that ~25x figure -- large enough to unambiguously distinguish
    // "VCOs at 110/220/330 Hz" from "VCOs at 20/40/60 Hz", small enough to
    // leave real margin against incidental drift from future Shape/Drive
    // default changes (D16-tuned parameters this test does not own).
    constexpr double kMinimumFundamentalToBrokenRatio = 10.0;
    REQUIRE_TRUE(expectedPower > brokenPower * kMinimumFundamentalToBrokenRatio);
}

// -----------------------------------------------------------------------
// A deliberately non-default patch: nonzero Drive (well past
// D16's own 20%), a nonzero VCO level (D16 leaves VCO pitch itself
// untouched, only Shape), and at least one active modulation depth. Also
// pushes the Filter/Reverb/Delay banks (which D16 never touches) away from
// their neutral defaults, exercising more of section 6a's routing surface
// than the default-patch test above.
// -----------------------------------------------------------------------
TEST_CASE(non_default_patch_produces_non_silent_finite_audio) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("non_default_patch"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    synth::Parameter& vco1Pitch = model.PageParameter(synth_froggers::FroggersBankId::Audio, 0);
    // "At least one modulation depth active" -- the six cross-VCO depths
    // D16's default patch already materializes satisfy this
    // without any drill-in machinery here; asserted explicitly rather than
    // assumed.
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(synth_froggers::kModSlotVco2Audio) != nullptr);

    vco1Pitch.SceneCenter(0) = 0.5f;  // nonzero VCO level.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;    // nonzero Drive.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 0.5f;   // Comb/Peak blend.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 0.4f;   // Wet/dry.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 1).SceneCenter(0) = 0.4f;    // Send.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 6).SceneCenter(0) = 0.4f;    // Wet mix.

    // Same as the default-patch test above -- the ASR gate only opens
    // while the transport runs.
    rig.StartAt(0);

    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(4);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);

    constexpr float kEpsilon = 1.0e-4f;
    REQUIRE_TRUE(PeakAbs(output) > kEpsilon);
}

// -----------------------------------------------------------------------
// Push the Filter bank's Comb feedback to its self-oscillating extreme
// (knob=1.0 -> dsp::Comb::GetFeedback's +0.95 branch -- item 1,
// deliberately below the frozen firmware's +1.1, Comb.hpp:66-76) with the
// Comb/Peak blend turned fully toward Comb, and the Reverb bank's
// authored Hold control pushed to its ceiling -- the exact
// self-oscillating-comb scenario reachable by design -- and confirm the
// output stays finite and bounded.
// -----------------------------------------------------------------------
TEST_CASE(self_oscillating_comb_and_near_unity_reverb_hold_stays_finite_and_bounded) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("comb_self_oscillation"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95 (item 1).
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // Wet/dry fully wet.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive.

    // The transport must be running, or the ASR gate stays closed and this
    // scenario never actually drives the self-oscillating comb with real
    // signal.
    rig.StartAt(0);

    rig.RunBlocks(64);  // run long enough for any runaway growth to show up.

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);

    // Matches FroggersApp::SanitizeOutputSample's final clamp (1.0f ==
    // float full scale; +1e-3 headroom for float rounding at the exact
    // boundary) -- the point of this test is that output stays *bounded*,
    // not that it stays quiet.
    for (const auto& frame : output) {
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::fabs(sample) <= 1.0f + 1.0e-3f);
        }
    }
}

// -----------------------------------------------------------------------
// The output clamp fix: the old ceiling
// (kMaxOutputMagnitude == 8.0f, +18 dBFS) let the device hard-clip into a
// square wave; the new ceiling (1.0f, float full scale) must never be
// exceeded even under the same deliberately overdriven patch the test above
// uses (self-oscillating Comb, Reverb Hold at its ceiling, maximum Drive),
// AND a normal-level (D16 default) patch's output -- which never approaches
// either ceiling -- must be completely unaffected by the constant change:
// asserted here as "every sample stays under 1.0", which is only possible
// if the clamp never engages for it, i.e. passthrough is unchanged.
// -----------------------------------------------------------------------
TEST_CASE(output_clamp_bounds_overdriven_patch_to_full_scale) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("output_clamp_overdriven"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95 (item 1).
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // Wet/dry fully wet.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive.

    rig.StartAt(0);
    rig.RunBlocks(64);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    for (const auto& frame : output) {
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::fabs(sample) <= 1.0f + 1.0e-3f);
        }
    }
}

TEST_CASE(output_clamp_never_engages_for_normal_level_default_patch) {
    // A normal-level signal passes through the clamp bit-identical: since
    // SanitizeOutputSample's clamp is a no-op for any |x| already <=
    // kMaxOutputMagnitude, proving every sample of the untouched D16
    // default patch stays strictly under the NEW (tighter) 1.0f ceiling
    // proves the clamp never fires for it -- so this change could not have
    // altered a single one of its samples.
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("output_clamp_normal_level"));
    rig.StartAt(0);
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(64);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    for (const auto& frame : output) {
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::fabs(sample) < 1.0f);
        }
    }
}

// -----------------------------------------------------------------------
// The acceptance test for "gain reduction, not per-sample waveshaping" --
// a signal entirely below OutputLimiter's 0.9 threshold must pass through
// Process() BIT-IDENTICAL (exact float `==`, not REQUIRE_NEAR). Drives the
// real per-app limiter instance directly via TestOutputLimiter() (this
// limiter's own test accessor), including values right at the threshold
// boundary and a long run, proving `envelope` never drifts off exactly
// 1.0f one sample at a time (see OutputLimiter::Process's own
// Sterbenz-lemma comment for why this must hold exactly, not just
// approximately).
// -----------------------------------------------------------------------
TEST_CASE(limiter_passes_below_threshold_signal_bit_identical) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("limiter_bit_identical"));
    auto& limiter = rig.Application().TestOutputLimiter();
    limiter.Reset();

    const float samples[] = {0.0f,      0.1f,       -0.1f,      0.5f,      -0.5f,     0.9f,
                              -0.9f,     0.899999f,  -0.899999f, 1.0e-6f,   -1.0e-6f};
    for (const float x : samples) {
        REQUIRE_TRUE(limiter.Process(x) == x);  // bit-for-bit, not approximate.
    }

    // A long run of below-threshold samples must never let `envelope` creep
    // off exactly 1.0f.
    for (int i = 0; i < 100000; ++i) {
        const float x = 0.8f * std::sin(static_cast<float>(i) * 0.01f);
        REQUIRE_TRUE(limiter.Process(x) == x);
    }
}

// -----------------------------------------------------------------------
// UI-rework ITEM 0 correction (2026-07-29): this test's own name says
// "reduces gain smoothly, not squared off" -- it does NOT say "never
// exceeds 1.0". That ceiling claim belongs to
// output_clamp_bounds_overdriven_patch_to_full_scale (:391 above), which
// exercises the real SanitizeOutputSample hard bound through the whole
// synth and already passes. This test instead exercises OutputLimiter
// directly via TestOutputLimiter(), bypassing that hard bound entirely, so
// it is only ever measuring the feed-forward limiter's OWN envelope
// tracking -- and a feed-forward limiter with no lookahead cannot promise
// an instantaneous ceiling: `envelope` (OutputLimiter::Process, above)
// one-pole-smooths toward each sample's targetGain rather than jumping to
// it, so on a fast-moving waveform the envelope can still be relaxing
// toward a lower target while `|x|` is already past its own local peak,
// letting `x * envelope` briefly overshoot the asymptote DesiredMagnitude
// approaches. Measured peak here is ~1.0274 for this tone -- a previous
// version of this test asserted `peak < 1.0f` directly against
// OutputLimiter output, which is what the ~1.0274 measurement contradicts;
// that assertion was wrong for what it exercises, not the limiter.
// The ceiling belongs to SanitizeOutputSample's hard bound, and gain
// reduction to this limiter -- this test now asserts exactly the
// gain-reduction claim: (1) the limited peak sits meaningfully below the
// unlimited input peak (kAmplitude, 1.5x) -- gain reduction actually
// happened; (2) consecutive near-peak output samples differ rather than
// repeating one clipped value -- the "not squared off" claim a hard clamp
// would violate; (3) a loose sanity bound of 1.05 (comfortably above the
// measured ~1.0274, comfortably below kAmplitude) to catch a gross
// regression -- e.g. a broken coefficient letting gain reduction stop
// working entirely -- without re-asserting the exact-ceiling claim this
// test does not own.
// -----------------------------------------------------------------------
TEST_CASE(limiter_reduces_gain_smoothly_not_squared_off) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("limiter_not_squared_off"));
    auto& limiter = rig.Application().TestOutputLimiter();
    limiter.Reset();

    constexpr float kSampleRate = 48000.0f;
    constexpr float kToneHz = 200.0f;
    constexpr float kAmplitude = 1.5f;  // well above 0.9 threshold, well below numeric-underflow territory.
    constexpr int kSamplesPerCycle = static_cast<int>(kSampleRate / kToneHz);
    constexpr int kWarmupCycles = 50;  // let the envelope settle into steady periodic tracking.

    auto toneAt = [&](int sampleIndex) {
        return kAmplitude * std::sin(2.0f * static_cast<float>(M_PI) * kToneHz *
                                      static_cast<float>(sampleIndex) / kSampleRate);
    };

    for (int i = 0; i < kWarmupCycles * kSamplesPerCycle; ++i) {
        limiter.Process(toneAt(i));
    }

    float peak = 0.0f;
    bool haveFirstNearPeak = false;
    float firstNearPeakValue = 0.0f;
    bool sawDistinctValuesNearPeak = false;
    for (int i = 0; i < kSamplesPerCycle; ++i) {
        const int sampleIndex = kWarmupCycles * kSamplesPerCycle + i;
        const float x = toneAt(sampleIndex);
        const float y = limiter.Process(x);
        peak = std::max(peak, std::fabs(y));
        if (std::fabs(x) > 1.4f) {  // near the sine's own peak, well above threshold.
            if (!haveFirstNearPeak) {
                firstNearPeakValue = y;
                haveFirstNearPeak = true;
            } else if (y != firstNearPeakValue) {
                sawDistinctValuesNearPeak = true;
            }
        }
    }
    REQUIRE_TRUE(haveFirstNearPeak);
    REQUIRE_TRUE(peak < kAmplitude - 0.3f);   // meaningfully below the unlimited 1.5x input peak: gain reduction happened.
    REQUIRE_TRUE(sawDistinctValuesNearPeak);  // NOT flattened to one repeated value near the peak (not squared off).
    REQUIRE_TRUE(peak < 1.05f);               // loose sanity bound (measured ~1.0274): catches gross failure, not a
                                               // hard ceiling -- see this test's header comment for why a
                                               // feed-forward limiter without lookahead cannot promise one; that
                                               // claim belongs to output_clamp_bounds_overdriven_patch_to_full_scale.
}

// -----------------------------------------------------------------------
// B7.5 Step 1 (2026-08-06 ruling): re-homed from
// limiter_engages_on_overdriven_patch_and_stays_bounded (below), which
// bundled two properties -- "the limiter itself does real gain reduction,
// not a no-op" and "this particular hostile patch runs the master hot
// enough to need it". The second is F2/B7.1's symptom (see
// overdriven_patch_stays_bounded's comment below) and was struck from the
// chain-level test. This property is independent of gain staging, so it
// moves here, driven directly via TestOutputLimiter() -- same convention as
// limiter_passes_below_threshold_signal_bit_identical (:448 above) and
// limiter_reduces_gain_smoothly_not_squared_off (:498 above).
// -----------------------------------------------------------------------
TEST_CASE(limiter_engages_and_envelope_drops_below_unity) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("limiter_engages_direct"));
    auto& limiter = rig.Application().TestOutputLimiter();
    limiter.Reset();

    // A steady tone well above the 0.9 threshold, run long enough for the
    // 1ms-attack envelope to settle away from its 1.0f rest state.
    constexpr float kSampleRate = 48000.0f;
    constexpr float kToneHz = 200.0f;
    constexpr float kAmplitude = 1.5f;
    constexpr int kSamplesPerCycle = static_cast<int>(kSampleRate / kToneHz);
    constexpr int kCycles = 20;
    for (int i = 0; i < kCycles * kSamplesPerCycle; ++i) {
        const float x = kAmplitude * std::sin(2.0f * static_cast<float>(M_PI) * kToneHz *
                                               static_cast<float>(i) / kSampleRate);
        limiter.Process(x);
    }

    REQUIRE_TRUE(limiter.envelope < 1.0f);  // the limiter genuinely engaged, not a no-op.
}

// -----------------------------------------------------------------------
// ITEM 3 -- the same deliberately overdriven patch
// output_clamp_bounds_overdriven_patch_to_full_scale uses (self-oscillating
// Comb, Reverb Hold at ceiling, maximum Drive). B7.5 Step 1 (2026-08-06
// ruling): this test used to also assert `minEnvelopeSeen < 0.999f` --
// "the master limiter engages on this patch". That assertion IS F2/B7.1's
// symptom: it pins the master sitting in continuous gain reduction on an
// ordinary hostile patch, which is precisely what the per-stage headroom
// architecture (C = 0.80) exists to remove -- so it was destined to fail
// the moment the real fix landed. The "limiter genuinely does gain
// reduction" property it also asserted is independent and is re-homed to
// limiter_engages_and_envelope_drops_below_unity above. This test keeps
// only the boundedness claim, and its name (formerly
// limiter_engages_on_overdriven_patch_and_stays_bounded) was changed to
// match -- a test whose name claims a property it no longer checks is its
// own trap.
// -----------------------------------------------------------------------
TEST_CASE(overdriven_patch_stays_bounded) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("limiter_overdriven_engages"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95 (item 1).
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // Wet/dry fully wet.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive.

    rig.StartAt(0);
    // 256 blocks: the boundedness-stress window measured by W2.2a. No
    // longer sampled per-block -- that per-block loop existed only to track
    // `minEnvelopeSeen` for the engagement assertion removed above (B7.5
    // Step 1), so it collapses to a single call.
    rig.RunBlocks(256);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    for (const auto& frame : output) {
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::fabs(sample) <= 1.0f + 1.0e-3f);
        }
    }
}

// -----------------------------------------------------------------------
// B7.5: the master limiter is the BACKSTOP, not the gain-staging
// mechanism. With every stage bounded to C, a hostile patch must not
// engage it at all. This test is the only end-to-end proof of that; every
// other limiter test in this file measures one stage under synthetic
// input (M3).
TEST_CASE(master_limiter_stays_at_unity_across_hostile_patch) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("b7_5_hostile"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // The SAME hostile patch `overdriven_patch_stays_bounded` uses -- measured
    // to engage the master at block 101, min envelope 0.809, so it is
    // known-hostile by measurement rather than by assumption.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // fully wet
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive
    // PLUS the operator's stated repro on top: Filter bank Crispy at max
    // scrambles all 8 bits of every Filter parameter per read. NOTE the
    // accessor -- Crispy is NOT in pageParameters_ (9 wide); it lives in its
    // own `crispy_` array (FroggersParameters.hpp:413-414), and
    // PageParameter(Filter, 14) throws std::out_of_range.
    model.Crispy(synth_froggers::FroggersBankId::Filter).SceneCenter(0) = 1.0f;
    // B7.5.0: a SceneCenter write is only ~81% applied after one block
    // (Parameter::ProcessSamplePhase1's periodic smoothed Compute, alpha
    // 0.0994 every 16 samples). This test must measure the patch it declares,
    // not a ramp into it, so apply it exactly before the first block.
    ApplyPatchNow(rig);

    rig.StartAt(0);
    auto& limiter = rig.Application().TestOutputLimiter();

    float minEnvelopeSeen = 1.0f;
    for (int block = 0; block < 256; ++block) {
        rig.RunBlocks(1);
        minEnvelopeSeen = std::min(minEnvelopeSeen, limiter.envelope);
    }

    REQUIRE_TRUE(!rig.SawNaN());
    RequireFiniteStereo(rig.Output());
    // LIVENESS -- without this the assertion below passes on silence, which is
    // how the first version of this test passed while proving nothing. The
    // instrument must actually be sounding for "the master never engaged" to
    // mean anything.
    REQUIRE_TRUE(PeakAbs(rig.Output()) > 0.1f);
    // The property. Unity means the master never had to do anything.
    REQUIRE_TRUE(minEnvelopeSeen > 0.999f);
}

// -----------------------------------------------------------------------
// B7.5 Step 6 (2026-08-06): the test above proves the master stays at unity
// under STATIC knobs only. This change's own acceptance criterion is "all
// maxima, modulation live" -- section K.1 measured DriveBlendPhase's allpass
// coefficient (read fresh every sample from the Drive/Phase knob) at 50.5x
// under periodic phase/content coincidence, the largest known blowout path
// in the instrument, and no static-knob test exercises it. This is the SAME
// hostile patch as master_limiter_stays_at_unity_across_hostile_patch, PLUS
// deep audio-rate modulation on Drive slot 8 (Phase, K.1's 50x path) and
// Filter slot 5 (Comb feedback, W2.2a's trim smoother was tuned against
// rand() sweeps, never real modulation). Deliberately kept as a SEPARATE
// test rather than folded into the one above: the two discriminate
// different failures (static gain staging vs. modulation-driven
// transients).
//
// SOURCE: kModSlotVco1Audio, not kModSlotNoise. First attempt used
// kModSlotNoise and measured minEnvelopeSeen=0.985726 -- statistically
// indistinguishable from the static test's 0.985796 -- which is a genuine
// end-to-end confirmation of K.1's *free random phase* figure (1.002x, not
// the 50.5x figure), not a null result: K.1's 50.5x is specifically
// "periodic phase/content coincidence", and a per-sample-random noise
// source (NoiseModulatorProcessor::Process() -> random_.UniformOpen01(),
// DspNoise.hpp:69-71) structurally cannot produce periodic coincidence.
// kModSlotVco1Audio (vco1AudioSource_ = NormalizeBipolarToUnit(vco1Raw),
// FroggersModulation.hpp:384, registered at :535-536) IS periodic at the
// note frequency and locked to the note's period by construction -- it IS
// the note passing through DriveBlendPhase, so the coincidence K.1 measured
// is exact rather than approximate. This is the corrected source for both
// modulated parameters below; kModSlotNoise is not used anywhere in this
// test.
//
// Route verified against synth::Parameter (ParameterModulation.hpp:499):
// `Parameter* EnsureModulationDepth(std::size_t modIx)` is public, returns
// nullptr at storage capacity (checked below, not dereferenced blindly), and
// PageParameter() returns synth::Parameter& (confirmed at
// FroggersModulation.hpp:1192,1207-1208's existing call sites), so it is
// reachable as PageParameter(...).EnsureModulationDepth(...) directly, no
// intermediate pointer needed. kModSlotVco1Audio (FroggersModulation.hpp:163,
// synth_froggers namespace) is registered `connected = true` unconditionally
// in ModulatorGroup's constructor (FroggersModulation.hpp:535-536), so it
// needs no extra wiring here. Depth centers are confirmed BIPOLAR by
// ModulationDepthTargetFromKnob (ParameterModulation.hpp:115-122): knob 0.5
// maps to bipolar 0 (zero depth), knob 1.0 maps to bipolar +1 (full
// positive) -- matching kNeutralModulationDepthCenter = 0.5f
// (FroggersModulation.hpp:832). SceneCenter(0) = 1.0f below is therefore
// full-positive depth, not a mid-scale value.
TEST_CASE(master_limiter_stays_at_unity_under_live_modulation) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("b7_5_live_mod"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // Identical hostile patch to master_limiter_stays_at_unity_across_hostile_patch.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // fully wet
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive
    model.Crispy(synth_froggers::FroggersBankId::Filter).SceneCenter(0) = 1.0f;

    // PLUS deep audio-rate modulation on the two parameters the evidence
    // names -- both from kModSlotVco1Audio (periodic, note-locked -- see the
    // route note above for why this replaces kModSlotNoise), depth at full
    // positive (1.0f, bipolar-neutral is 0.5f per the route note above).
    synth::Parameter* phaseDepth =
        model.PageParameter(synth_froggers::FroggersBankId::Drive, 8)
             .EnsureModulationDepth(synth_froggers::kModSlotVco1Audio);
    REQUIRE_TRUE(phaseDepth != nullptr);
    phaseDepth->SceneCenter(0) = 1.0f;

    synth::Parameter* combFeedbackDepth =
        model.PageParameter(synth_froggers::FroggersBankId::Filter, 5)
             .EnsureModulationDepth(synth_froggers::kModSlotVco1Audio);
    REQUIRE_TRUE(combFeedbackDepth != nullptr);
    combFeedbackDepth->SceneCenter(0) = 1.0f;

    // B7.5.0: apply the patch (including the modulation-depth SceneCenter
    // writes above) before the first block, exactly as the static test does.
    ApplyPatchNow(rig);

    rig.StartAt(0);
    auto& limiter = rig.Application().TestOutputLimiter();

    float minEnvelopeSeen = 1.0f;
    for (int block = 0; block < 256; ++block) {
        rig.RunBlocks(1);
        minEnvelopeSeen = std::min(minEnvelopeSeen, limiter.envelope);
    }

    REQUIRE_TRUE(!rig.SawNaN());
    RequireFiniteStereo(rig.Output());
    // LIVENESS -- same rationale as the static test above.
    REQUIRE_TRUE(PeakAbs(rig.Output()) > 0.1f);
    // The property. Unity means the master never had to do anything.
    REQUIRE_TRUE(minEnvelopeSeen > 0.999f);
}

// -----------------------------------------------------------------------
// The randomize storm test: the predecessor's failure rate was roughly 1
// in 7 Randomize All draws (permanent silence or a full-scale-exceeding
// blowout); this must show ZERO across at least 200 draws through the
// REAL engine path --
// `rig.Application().RequestRandomizeAll()` is the exact method
// `FroggersUiSurface::HandleAction` calls for the Randomize All button
// (FroggersUiSurface.hpp), consumed on the very next audio-thread
// ProcessFrame() (FroggersAppCore.hpp's `pendingRandomizeAll_`) -- same
// call FroggersRandomizeAllRepro.cpp's proven repro uses, not a
// shadow/copy of the DSP chain.
//
// Per draw: render a full second of audio (one full quarter-note gate
// cycle or more at any ordinary tempo, since Item 4's attack/release
// ceilings and RandomizeAll's own scope never touch the master clock's
// BPM -- only the six parameter banks), then assert (a) finite throughout,
// (b) never sustained above full scale (the limiter's own bound, with the
// same rounding headroom the other output-bound tests use), and (c) still
// audibly producing sound (PeakAbs over that window exceeds a small
// epsilon) -- "not permanently silenced" is exactly the predecessor's
// failure signature.
//
// Deliberately does NOT stop at the first failure (unlike REQUIRE_TRUE
// elsewhere in this file): tallies failures across all 200+ draws so a
// nonzero rate is reported precisely rather than only "it failed once
// somewhere", per this item's own instruction to report a real finding
// rather than loosen the assertion.
// -----------------------------------------------------------------------
TEST_CASE(randomize_all_storm_test_never_blows_out_or_permanently_silences) {
    constexpr int kNumDraws = 200;
    constexpr double kSampleRateHz = 48000.0;
    constexpr double kBlockSize = 256.0;  // FroggersApp::Config().
    const std::size_t kBlocksPerSecond = static_cast<std::size_t>(std::ceil(kSampleRateHz / kBlockSize));
    constexpr float kOverScaleBound = 1.0f + 1.0e-3f;
    constexpr float kSilenceEpsilon = 1.0e-4f;

    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("randomize_all_storm"));
    rig.StartAt(0);
    rig.RunBlocks(8);  // establish a running steady state before the first draw.

    int nonFiniteFailures = 0;
    int overScaleFailures = 0;
    int permanentSilenceFailures = 0;

    for (int draw = 0; draw < kNumDraws; ++draw) {
        rig.Application().RequestRandomizeAll();
        rig.ClearOutput();
        rig.RunBlocks(kBlocksPerSecond);

        if (rig.SawNaN()) {
            ++nonFiniteFailures;
        }
        const auto& output = rig.Output();
        bool everyFinite = true;
        bool anyOverScale = false;
        float peak = 0.0f;
        for (const auto& frame : output) {
            for (const float sample : frame.channels) {
                if (!std::isfinite(sample)) {
                    everyFinite = false;
                    continue;
                }
                if (std::fabs(sample) > kOverScaleBound) {
                    anyOverScale = true;
                }
                peak = std::max(peak, std::fabs(sample));
            }
        }
        if (!everyFinite && !rig.SawNaN()) {
            ++nonFiniteFailures;  // belt-and-suspenders: SawNaN() covers the primary path, this covers any gap.
        }
        if (anyOverScale) {
            ++overScaleFailures;
        }
        if (peak <= kSilenceEpsilon) {
            ++permanentSilenceFailures;
        }
    }

    const int totalFailures = nonFiniteFailures + overScaleFailures + permanentSilenceFailures;
    if (totalFailures != 0) {
        std::ostringstream oss;
        oss << __FILE__ << ":" << __LINE__ << " randomize_all_storm_test_never_blows_out_or_permanently_silences: "
            << totalFailures << "/" << kNumDraws << " draws failed (" << nonFiniteFailures << " non-finite, "
            << overScaleFailures << " over full scale, " << permanentSilenceFailures << " permanently silent)";
        throw std::runtime_error(oss.str());
    }
}

// -----------------------------------------------------------------------
// This IS the mechanism that fixes the operator's "audio never comes
// back": before RecoverPoisonedUnitState()
// existed, nothing anywhere in FroggersAppCore.hpp ever cleared a unit's
// recursive state once poisoned (see RecoverPoisonedUnitState()'s own
// header comment for the full trace), so it persisted across every future
// block regardless of what the operator changed the knobs to. Injects a
// non-finite value directly into ONE unit's state (the Filter bank's peak
// ResonantBump -- the same kind of unit the scoopNotch self-oscillation bug
// actually hit) via the task-2.2/2.3 test accessors, and asserts (a) that
// unit recovers within a handful of blocks and (b) a DIFFERENT unit's state
// is completely untouched -- proving recovery is per-unit, not global (this
// class's own design decision: a global reset would cut the reverb tail and
// delay repeats every time one unrelated unit misbehaved).
// -----------------------------------------------------------------------
TEST_CASE(finiteness_recovery_resets_only_the_poisoned_unit_and_audio_recovers) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("finiteness_recovery"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;  // nonzero VCO level.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.5f;  // nonzero Drive.
    rig.StartAt(0);

    // "Other unit untouched" marker: a sentinel written into the Comb's
    // delay line far enough from its write cursor (which starts at 0 and
    // only advances one slot per SAMPLE) that the small number of samples
    // this test runs can never reach it -- so if Reset() (which
    // std::fill()s the WHOLE array) is ever mistakenly called on the comb,
    // this slot goes from the sentinel to exactly 0.0f; if the comb is
    // truly untouched, it stays exactly the sentinel.
    dsp::Comb& comb = rig.Application().TestFilterComb();
    constexpr float kSentinel = 12345.6789f;
    constexpr std::size_t kSentinelIndex = 4000;  // << dsp::Comb::kSize (8192); far past any write cursor here.
    comb.delayLine[kSentinelIndex] = kSentinel;

    // Poison the peak ResonantBump's own recursive state directly.
    dsp::ResonantBump& peak = rig.Application().TestFilterPeak();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    peak.biquad.y1 = nan;
    peak.biquad.y2 = nan;
    REQUIRE_TRUE(!peak.StateFinite());

    // One block (256 samples @ 48kHz, FroggersAppCore::Config()) is enough
    // for RecoverPoisonedUnitState() to fire -- it runs once per block, and
    // Tier 1 fires immediately/unconditionally on the very first check that
    // observes non-finite state, with no sustained-window delay (only Tier
    // 2's magnitude check has one).
    rig.RunBlocks(1);

    REQUIRE_TRUE(peak.StateFinite());  // recovered.
    REQUIRE_TRUE(comb.delayLine[kSentinelIndex] == kSentinel);  // a DIFFERENT unit: untouched.

    // And audio genuinely recovers: run further blocks and confirm real,
    // non-silent, finite output -- not permanently masked to zero the way
    // SanitizeOutputSample alone (with no recovery underneath it) would
    // have left it.
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    REQUIRE_TRUE(PeakAbs(output) > 1.0e-4f);
}

// -----------------------------------------------------------------------
// Tasks 2.4/2.5 (Tier 2, magnitude recovery) -- "sustained" defined and
// justified: a unit's state magnitude must exceed kMaxUnitStateMagnitude
// (100.0, derived beside the constant in FroggersAppCore.hpp) for at least
// kSustainedOverCeilingSeconds (0.01s == 10ms) of continuous real time,
// checked once per block, before Tier 2 resets it -- so a single block's
// transient excursion must NOT fire it, but two-or-more consecutive
// over-ceiling blocks (which already total >= 10ms at this rig's 256-
// sample/48kHz config: 256/48000 ~= 5.33ms/block, so block 1 alone is
// 5.33ms < 10ms, but block 1 + block 2 is 10.67ms >= 10ms) must.
// -----------------------------------------------------------------------
TEST_CASE(magnitude_recovery_ignores_a_single_block_transient) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("magnitude_recovery_transient"));
    dsp::Comb& comb = rig.Application().TestFilterComb();

    // A finite, over-ceiling (> 100.0) value far from the write cursor (see
    // the finiteness-recovery test above for why 4000 is safe) -- Tier 1
    // does not fire (it IS finite), so only Tier 2's sustained-window logic
    // is under test here.
    constexpr std::size_t kIndex = 4000;
    comb.delayLine[kIndex] = 500.0f;
    REQUIRE_TRUE(comb.StateFinite());
    REQUIRE_TRUE(comb.StateMagnitude() > 100.0f);

    // One over-ceiling block (~5.33ms) is strictly less than the 10ms
    // sustained window -- must NOT have fired yet.
    rig.RunBlocks(1);
    REQUIRE_TRUE(comb.delayLine[kIndex] == 500.0f);  // untouched: not reset.

    // The transient subsides (as a real one-sample spike would, decaying
    // within its own block) -- drop back under the ceiling before the
    // sustained window would have elapsed.
    comb.delayLine[kIndex] = 50.0f;  // finite, well under the 100.0 ceiling.
    rig.RunBlocks(4);
    // Still exactly 50.0f: the counter reset to 0 the instant magnitude
    // dropped back under the ceiling, so no amount of further (now-normal)
    // time can retroactively fire Tier 2 for this now-subsided transient.
    REQUIRE_TRUE(comb.delayLine[kIndex] == 50.0f);
}

TEST_CASE(magnitude_recovery_resets_after_sustained_over_ceiling_window) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("magnitude_recovery_sustained"));
    dsp::Comb& comb = rig.Application().TestFilterComb();

    constexpr std::size_t kIndex = 4000;
    comb.delayLine[kIndex] = 500.0f;

    // Block 1: ~5.33ms elapsed, still < 10ms -- not yet reset.
    rig.RunBlocks(1);
    REQUIRE_TRUE(comb.delayLine[kIndex] == 500.0f);

    // Block 2: ~10.67ms elapsed total, >= 10ms -- Tier 2 fires. Reset()
    // std::fill()s the WHOLE delay line to 0.0f, so the sentinel slot goes
    // to exactly 0.0f (not merely "no longer 500").
    rig.RunBlocks(1);
    REQUIRE_TRUE(comb.delayLine[kIndex] == 0.0f);
    REQUIRE_TRUE(comb.StateMagnitude() <= 100.0f);
}

// -----------------------------------------------------------------------
// Full-range endpoint sweep. No existing test drives every parameter to
// its endpoints; this drives every named parameter (slots 0-8,
// kFroggersParamsPerBank) plus each bank's own Crispy (slot 14,
// kFroggersCrispySlot) in all six banks, plus the shared Crunchy (slot 15,
// kFroggersCrunchySlot -- the SAME synth::Parameter object in every bank,
// so swept once, not six times), to 0.0, to
// 1.0, and back to a neutral 0.5, through the REAL engine path
// (rig.RunBlocks -- FroggersApp::ProcessBlock/RouteAudioSample, not a
// shadow/copy), asserting output stays finite at every single endpoint AND
// that the instrument is not left permanently silent once every parameter
// is back at a neutral value.
// -----------------------------------------------------------------------
TEST_CASE(full_range_endpoint_sweep_stays_finite_and_not_permanently_silenced) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("endpoint_sweep"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    rig.StartAt(0);
    rig.RunBlocks(4);  // establish a running steady state first.

    const synth_froggers::FroggersBankId banks[] = {
        synth_froggers::FroggersBankId::Audio,  synth_froggers::FroggersBankId::Envelope,
        synth_froggers::FroggersBankId::Filter, synth_froggers::FroggersBankId::Drive,
        synth_froggers::FroggersBankId::Delay,  synth_froggers::FroggersBankId::Reverb,
    };

    const auto sweepOneParam = [&](synth::Parameter& param) {
        for (const float endpoint : {0.0f, 1.0f}) {
            param.SceneCenter(0) = endpoint;
            rig.RunBlocks(2);
            REQUIRE_TRUE(!rig.SawNaN());
            RequireFiniteStereo(rig.Output());
            rig.ClearOutput();
        }
        param.SceneCenter(0) = 0.5f;  // neutral, nonzero/non-degenerate, before the next parameter.
        rig.RunBlocks(1);
        rig.ClearOutput();
    };

    for (const auto bank : banks) {
        for (std::size_t slot = 0; slot < synth_froggers::kFroggersParamsPerBank; ++slot) {
            sweepOneParam(model.PageParameter(bank, slot));
        }
        sweepOneParam(model.Crispy(bank));  // this bank's own Crispy (slot 14) -- own accessor, not PageParameter.
    }
    sweepOneParam(model.Crunchy());  // shared Crunchy (slot 15), swept once.

    // Not permanently silenced: with every parameter now left at a neutral
    // 0.5 (not the D16 default, but nonzero/non-degenerate everywhere), the
    // instrument must still be capable of producing real, audible output --
    // proving the sweep (including whatever transient recovery it may have
    // triggered along the way) did not leave anything permanently poisoned.
    rig.RunBlocks(8);
    rig.ClearOutput();
    rig.RunBlocks(8);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    REQUIRE_TRUE(PeakAbs(output) > 1.0e-4f);
}

// -----------------------------------------------------------------------
// The app is silent while the transport is stopped, regardless of any
// parameter setting, including
// immediately after Init()/PrepareToPlay() with the transport never
// started. Uses the same non-default patch as the test above (nonzero
// Drive, nonzero VCO level, an active modulation depth) specifically to
// prove silence is coming from the gate, not merely from an unexcited
// patch -- if the gate were stuck open this patch would be loud (see
// non_default_patch_produces_non_silent_finite_audio above).
// -----------------------------------------------------------------------
TEST_CASE(silent_while_transport_is_stopped) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("transport_stopped"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    synth::Parameter& vco1Pitch = model.PageParameter(synth_froggers::FroggersBankId::Audio, 0);
    REQUIRE_TRUE(vco1Pitch.ModulationDepthParameter(synth_froggers::kModSlotVco2Audio) != nullptr);

    vco1Pitch.SceneCenter(0) = 0.5f;  // nonzero VCO level.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;  // nonzero Drive.

    // Deliberately no rig.StartAt(...) -- the rig's transport starts
    // Stopped (MasterClock::Prepare() resets transportState_ to Stopped,
    // src/MasterClock.cpp:929) and stays there absent an explicit Start.
    rig.RunBlocks(12);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    REQUIRE_TRUE(PeakAbs(output) == 0.0f);
}

// -----------------------------------------------------------------------
// A tempo-change-tracking test: gate/advance period follows a
// SetTempoBpm change. A high base tempo (6000 BPM -> quarter note = 480
// samples @ 48kHz) keeps the test fast while still giving the ASR's fast
// (ExpMap-floor) attack/release plenty of headroom to reach a clean
// silent floor every half-quarter-note. Doubling tempo halves the
// quarter-note period, so the number of gate-open cycles observed over
// the SAME sample window should roughly double.
// -----------------------------------------------------------------------
TEST_CASE(gate_period_tracks_tempo_change) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("tempo_tracks_gate"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;

    constexpr double kBaseTempoBpm = 6000.0;
    REQUIRE_TRUE(rig.Engine().Clock().SetTempoBpm(kBaseTempoBpm));
    rig.StartAt(0);
    rig.RunBlocks(64);
    const std::size_t baseTransitions = CountRisingEdges(rig.Output());

    rig.ClearOutput();
    constexpr double kDoubledTempoBpm = kBaseTempoBpm * 2.0;
    REQUIRE_TRUE(rig.Engine().Clock().SetTempoBpm(kDoubledTempoBpm));
    rig.RunBlocks(64);
    const std::size_t doubledTransitions = CountRisingEdges(rig.Output());

    REQUIRE_TRUE(baseTransitions > 0);
    // Generous tolerance (1.3x-3x, not exactly 2x) -- ASR attack/release
    // smoothing at the transition edges makes an exact doubling too strict
    // a bound for a black-box audio-domain measurement.
    REQUIRE_TRUE(static_cast<double>(doubledTransitions) > static_cast<double>(baseTransitions) * 1.3);
    REQUIRE_TRUE(static_cast<double>(doubledTransitions) < static_cast<double>(baseTransitions) * 3.0);
}

// -----------------------------------------------------------------------
// A dedicated missing-clock-plan test: a zero-frame or rejected-commit
// callback (block.clockPlan == nullptr) does not fault and leaves the
// gate closed. Hand-builds a synth::AudioBlock with
// clockPlan == nullptr and calls FroggersApp::ProcessBlock directly
// (bypassing SynthRig's own per-block orchestration, which this one check
// does not need) -- output buffers are poisoned with a nonzero value first,
// so a silent gate is proven by an actual zero WRITE, not merely an
// untouched buffer.
// -----------------------------------------------------------------------
TEST_CASE(missing_clock_plan_does_not_fault_and_leaves_gate_closed) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("missing_clock_plan"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
    // Deliberately no rig.StartAt(...) -- same "transport never started"
    // starting point as silent_while_transport_is_stopped above.

    constexpr std::size_t kFrames = 32;
    std::vector<float> outL(kFrames, 1.0f);
    std::vector<float> outR(kFrames, 1.0f);
    float* outPtrs[2] = {outL.data(), outR.data()};

    synth::AudioBlock block;
    block.inputs = nullptr;
    block.outputs = outPtrs;
    block.numInputChannels = 0;
    block.numOutputChannels = 2;
    block.numFrames = kFrames;
    block.startSample = 12345;
    block.clockPlan = nullptr;

    rig.Application().ProcessBlock(block);  // must not throw or crash.

    for (const float sample : outL) {
        REQUIRE_TRUE(sample == 0.0f);
    }
    for (const float sample : outR) {
        REQUIRE_TRUE(sample == 0.0f);
    }
}

// -----------------------------------------------------------------------
// Operator report: "Stop doesn't work" -- pressing Stop closes the ASR gate
// (silent_while_transport_is_stopped above) but the delay and
// reverb are feedback structures that keep ringing on their own: the Delay
// bank's feedback runs up to 0.98 (dsp::StereoDelay::Process's own clamp,
// Delay.hpp:135) and the Reverb bank's Hold control pushes its internal
// feedback arbitrarily close to 1.0 (dsp::Reverb::Process, Reverb.hpp:174).
// This pushes both to that self-sustaining extreme (mirroring the pattern
// self_oscillating_comb_and_near_unity_reverb_hold_stays_finite_and_bounded
// above uses to reach a self-sustaining state), confirms it is actually
// ringing, stops the transport mid-ring, and asserts the output falls below
// -60 dBFS within ~250 ms AND stays there for a further stretch -- "decayed
// once" is not "stopped".
// -----------------------------------------------------------------------
TEST_CASE(stopping_transport_silences_self_sustaining_delay_and_reverb) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("stop_silences_delay_reverb"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    synth::Parameter& vco1Pitch = model.PageParameter(synth_froggers::FroggersBankId::Audio, 0);
    vco1Pitch.SceneCenter(0) = 0.5f;  // nonzero VCO level, so there's signal to excite the tanks.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;  // nonzero Drive.

    // Delay bank: rows per dsp::MapRowsToDelayParams's own comment (Delay.hpp
    // :1088-1119) -- 1=Send, 2=Feedback, 6=Mix. Feedback at 1.0 clamps to 0.98
    // inside StereoDelay::Process (Delay.hpp:818), the near-unity extreme
    // the defect report cites.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;  // Send.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;  // Feedback -> 0.98.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;  // Wet mix.

    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 0.08f;  // Hold -> moderate.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // Wet/dry fully wet.

    rig.StartAt(0);
    std::uint64_t timestamp = 0;  // Mirrors SynthRig's own internal timestamp counter -- see
                                   // this test's comment below on why that tracking is valid.

    // Run long enough to firmly excite and establish self-sustaining ringing
    // in both tanks.
    constexpr std::size_t kExciteBlocks = 300;
    rig.RunBlocks(kExciteBlocks);
    timestamp += kExciteBlocks;
    REQUIRE_TRUE(!rig.SawNaN());

    // Confirm it's actually ringing loudly before trusting the "and stops"
    // half of this test to mean anything.
    rig.ClearOutput();
    constexpr std::size_t kConfirmRingBlocks = 40;
    rig.RunBlocks(kConfirmRingBlocks);
    timestamp += kConfirmRingBlocks;
    const auto& ringingOutput = rig.Output();
    RequireFiniteStereo(ringingOutput);
    const float ringingPeak = PeakAbs(ringingOutput);
    constexpr float kRingingFloorLinear = 1.0e-2f;  // ~ -40 dBFS.
    REQUIRE_TRUE(ringingPeak > kRingingFloorLinear);

    // Stop the transport mid-ring. SynthRig::StopAt pushes a MessageIn::Stop
    // carrying this timestamp onto the UI bus (SynthRig.hpp:176-178);
    // Engine::ProcessBlock drains that bus using each block's own sequential
    // timestamp (Engine.hpp:396, MessageInBus::Pop's `queue_[head].timestamp
    // > timestamp` gate, ParameterModulation.cpp:4013, which is inclusive of
    // equality) BEFORE committing that block's clock plan (Engine.hpp:402),
    // so passing exactly the running tally of blocks already executed via
    // RunBlocks (SynthRig::NextTimestamp() is a private monotonically
    // increasing counter RunBlocks draws from once per block, starting at 0,
    // and StartAt(0) above never drew from it) lines this Stop message up
    // with the very next block RunBlocks executes -- the same shape
    // miniapp_system_tests.cpp's own `rig.StopAt(3); rig.RunBlockAt(3);`
    // (miniapp_system_tests.cpp:1197-1198) relies on.
    rig.StopAt(timestamp);

    // "Falls below -60 dBFS within ~250 ms" means the output has REACHED
    // that floor by the 250 ms mark -- not that it has been below the floor
    // for the entire 250 ms leading up to it. There is a brief, fast-decaying
    // transient right at the stop instant (the ASR release ramps its last
    // few samples to zero over the release knob's own time constant, still
    // feeding a small amount of real signal into the just-reset delay/
    // reverb tanks) that is expected and not itself a bug. So this runs
    // most of the settle window unmeasured, then captures only a short
    // trailing window right at the ~250 ms mark -- and does the same thing again further
    // out for the "stays there" half, rather than taking one peak over the
    // whole span (which would also catch that brief opening transient and
    // never read as silent regardless of how well the fix works).
    // (FroggersApp::Config() sets 48 kHz/256-sample blocks,
    // FroggersAppCore.hpp:196-197). -60 dBFS: 20*log10(x) = -60 -> x = 1.0e-3.
    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);

    rig.RunBlocks(settleLeadBlocks);
    timestamp += settleLeadBlocks;
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    timestamp += checkWindowBlocks;

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& settledOutput = rig.Output();
    RequireFiniteStereo(settledOutput);
    const float settledPeak = PeakAbs(settledOutput);
    REQUIRE_TRUE(settledPeak < kSilenceFloorLinear);

    // And it must STAY there -- not merely have decayed once. Run a further
    // stretch, then measure another short trailing window, confirming it's
    // still below the floor (and did not somehow build back up).
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& staysSilentOutput = rig.Output();
    RequireFiniteStereo(staysSilentOutput);
    const float staysSilentPeak = PeakAbs(staysSilentOutput);
    REQUIRE_TRUE(staysSilentPeak < kSilenceFloorLinear);
}

// ITEM 1 (post-just-landed-fix defect): the test above
// (stopping_transport_silences_self_sustaining_delay_and_reverb) sets no
// Envelope-bank parameters, so every VcoAdsrState voice's Release sits at
// its ~0 default (FroggersParameters.hpp's Envelope layout gives Sustain an
// explicit 1.0f default but leaves Attack/Release at ParameterConfig's own
// 0.0f default) -- closing the gate on Stop snaps each voice to Idle almost
// immediately, so the one-shot delay_.ClearBuffers()/reverb_.Reset() at the
// running->stopped edge (FroggersAppCore.hpp's `runStopTeardown()`, the
// immediate AllIdle() branch) is never re-armed by anything and the test
// above is silent on arrival.
//
// This test instead pushes Envelope Release (bank rows 2/5/8, "Release
// VCO1-3" -- FroggersParameters.hpp:160-162) to 1.0, mapping through
// VcoAdsrState::mapRelease (VoiceEnvelope.hpp:94-98) to ~kMaxReleaseSeconds
// (2.5f, VoiceEnvelope.hpp:84) -- so closing the gate on Stop moves every
// voice to Stage::Release (VoiceEnvelope.hpp:139) and keeps it producing a
// slowly-decaying signal for ~2.5 real seconds, feeding delay_/reverb_ well
// after the one-shot reset already fired.
//
// Delay Time (row 0) is additionally pinned to 0.6 (baseSeconds =
// ExpMapCompute(0.001, 2.0, 0.6) ~= 0.0957s per delay cycle -- Delay.hpp:111)
// rather than left at its own 0.0f default (~0.001s/cycle, too short to
// demonstrate the bug within a boundable test window): with feedback
// clamped to 0.98 (Delay.hpp:135), decaying 60 dB unaided takes
// ln(1e-3)/ln(0.98) ~= 342 delay cycles, ~= 342 * 0.0957s ~= 32.7s here --
// squarely the "tens of seconds" the operator's Randomize All report and
// this task's brief describe -- so a settle window shortly after the ~2.5s
// release completes sits deep inside that unaided decay tail and cleanly
// discriminates the one-shot fix (still ringing there) from the
// keep-clearing-until-Idle fix (already silent there).
TEST_CASE(stopping_transport_silences_self_sustaining_delay_and_reverb_with_long_release) {
    Rig rig(/*patchPumpBudgetBlocks=*/64,
            UseScratchRuntimeDataPaths("stop_silences_delay_reverb_long_release"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    synth::Parameter& vco1Pitch = model.PageParameter(synth_froggers::FroggersBankId::Audio, 0);
    vco1Pitch.SceneCenter(0) = 0.5f;  // nonzero VCO level, so there's signal to excite the tanks.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;  // nonzero Drive.

    // Envelope bank: interleaved ADSR, slot = 4*vco + {0:Attack, 1:Decay,
    // 2:Sustain, 3:Release} (FroggersParameters.hpp:162-164), so rows 3/7/11
    // are Release VCO1-3. Sustain (rows 2/6/10) already defaults to 1.0f;
    // Attack (rows 0/4/8) stays at its fast ~0 default.
    model.PageParameter(synth_froggers::FroggersBankId::Envelope, 3).SceneCenter(0) = 1.0f;  // Release VCO1 -> ~2.5s.
    model.PageParameter(synth_froggers::FroggersBankId::Envelope, 7).SceneCenter(0) = 1.0f;  // Release VCO2 -> ~2.5s.
    model.PageParameter(synth_froggers::FroggersBankId::Envelope, 11).SceneCenter(0) = 1.0f;  // Release VCO3 -> ~2.5s.

    // Delay bank: rows per dsp::MapRowsToDelayParams's own comment
    // (Delay.hpp:1088-1119) -- 0=Time, 1=Send, 2=Feedback, 6=Mix.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 0).SceneCenter(0) = 0.6f;  // Time -> ~96ms/cycle.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;  // Send.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;  // Feedback -> 0.98.
    model.PageParameter(synth_froggers::FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;  // Wet mix.

    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 0.08f;  // Hold -> moderate.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // Wet/dry fully wet.

    rig.StartAt(0);
    std::uint64_t timestamp = 0;

    // Run long enough to firmly excite and establish self-sustaining ringing
    // in both tanks (same shape as the sibling test above).
    constexpr std::size_t kExciteBlocks = 300;
    rig.RunBlocks(kExciteBlocks);
    timestamp += kExciteBlocks;
    REQUIRE_TRUE(!rig.SawNaN());

    rig.ClearOutput();
    constexpr std::size_t kConfirmRingBlocks = 40;
    rig.RunBlocks(kConfirmRingBlocks);
    timestamp += kConfirmRingBlocks;
    const auto& ringingOutput = rig.Output();
    RequireFiniteStereo(ringingOutput);
    const float ringingPeak = PeakAbs(ringingOutput);
    constexpr float kRingingFloorLinear = 1.0e-2f;  // ~ -40 dBFS.
    REQUIRE_TRUE(ringingPeak > kRingingFloorLinear);

    rig.StopAt(timestamp);

    // Settle window: 10.5s from the Stop instant -- generous enough that the
    // ~2.5s Release (VcoAdsrState::kMaxReleaseSeconds) has fully finished on
    // every voice (VcoAdsrState::AllIdle()) with roughly 8s to spare for block
    // granularity and the fix's final flush, but far short of the ~32.7s
    // unaided decay tail (see header comment above) the pre-fix one-shot
    // reset leaves behind -- so this window only passes once the delay/
    // reverb tanks are actively being kept clear through the whole release,
    // not merely once they've had "long enough" to ring out on their own.
    // kSampleRateHz/kBlockSizeSamples are kept local (not folded into
    // ComputeSilenceSettleWindow) because this test alone also reuses them
    // below for its own separate afterStopSeconds -> afterStopBlocks
    // conversion (a single-occurrence computation, not part of the 3x
    // duplicated settle-window concept F2 flagged).
    constexpr double kSampleRateHz = 48000.0;
    constexpr double kBlockSizeSamples = 256.0;
    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/10.5);

    // ITEM 1 (clear-once-at-AllIdle, not clear-every-block): this is the
    // assertion that actually distinguishes the two policies. Both the
    // old "wipe delay_/reverb_ every block for as long as any voice is
    // releasing" policy and the new "wipe once, only when AllIdle() first
    // turns true" policy leave the tanks (and thus the output, both banks
    // set fully wet above) silent by the 10.5s settle window checked below
    // -- that check alone cannot tell them apart. What it cannot see is
    // that the ~2.5s Release is supposed to keep ringing wet through
    // delay_/reverb_ the whole time it's live, same as it would with the
    // transport still running (RouteAudioSample() feeds them every sample
    // regardless of transport, ProcessBlock's own comment). Under the old
    // policy delay_/reverb_ would already have been wiped within the very
    // first post-stop block and stay wiped every block after, so the
    // output here would already be at/near the settled silence floor; the
    // new policy leaves the tanks alone until AllIdle(), so the release's
    // wet tail must still be clearly audible this soon after Stop, deep
    // inside the 2.5s Release and nowhere near AllIdle. 0.3s post-stop is
    // comfortably past the release's own fast opening transient (see the
    // sibling short-release test's comment on that) while still far short
    // of the ~2.5s it takes every voice to reach Stage::Idle.
    // SUPERSEDED 2026-07-29 — this assertion was inverted, deliberately.
    //
    // It used to require that the release was STILL AUDIBLY RINGING 0.3s
    // after Stop, as proof that the delay/reverb tanks were not being wiped
    // every block during the release. That protection is now moot and the
    // behaviour it pinned is the bug the operator reported: "the stop button
    // doesnt actually stop all audio in all circumstances."
    //
    // A transport Stop now forces a ~50ms fade independent of the patch's
    // release knob (FroggersAppCore::RouteAudioSample's `releaseKnob` lambda),
    // so by 0.3s every voice has long since reached Stage::Idle, the
    // clear-at-AllIdle has fired, and the tanks are empty. The old
    // "don't wipe every block" concern only ever applied while a long release
    // was still running after Stop — which can no longer happen.
    constexpr double kAfterStopSeconds = 0.3;
    const auto afterStopBlocks =
        static_cast<std::size_t>(std::ceil((kAfterStopSeconds * kSampleRateHz) / kBlockSizeSamples));
    rig.RunBlocks(afterStopBlocks);
    timestamp += afterStopBlocks;
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    timestamp += checkWindowBlocks;

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& afterStopOutput = rig.Output();
    RequireFiniteStereo(afterStopOutput);
    const float afterStopPeak = PeakAbs(afterStopOutput);
    REQUIRE_TRUE(afterStopPeak < kSilenceFloorLinear);

    rig.RunBlocks(settleLeadBlocks);
    timestamp += settleLeadBlocks;
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    timestamp += checkWindowBlocks;

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& settledOutput = rig.Output();
    RequireFiniteStereo(settledOutput);
    const float settledPeak = PeakAbs(settledOutput);
    REQUIRE_TRUE(settledPeak < kSilenceFloorLinear);

    // And it must STAY there through the (short, near-unity-feedback-decay)
    // remainder -- not merely have decayed once right at the release's tail.
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);

    REQUIRE_TRUE(!rig.SawNaN());
    const auto& staysSilentLongReleaseOutput = rig.Output();
    RequireFiniteStereo(staysSilentLongReleaseOutput);
    const float staysSilentLongReleasePeak = PeakAbs(staysSilentLongReleaseOutput);
    REQUIRE_TRUE(staysSilentLongReleasePeak < kSilenceFloorLinear);
}

// =========================================================================
// T2.3(a) pins T2.2 directly -- while the transport is stopped, the
// three drive pre-gains (Delay slot 9 "Feedback drive", Reverb slot 10
// "Tank drive", Filter slot 12 "Comb drive") and Freeze (Delay slot 4)
// resolve to their unity/zero effective values regardless of the commanded
// knob, WITHOUT writing to the parameter model, and resuming play restores
// the commanded mapping bit-exactly. Commanded values are deliberately
// pinned to each control's MAX (1.0f) -- the opposite extreme from the
// override's unity/zero target -- so a passing run cannot be a coincidence
// of the override happening to match a default. Comb drive/Feedback drive
// read directly off dsp::Comb::combDrive/dsp::StereoDelay::fbDrive
// (TestFilterComb()/TestDelay(), both already public members storing the
// POST-ExpMapCompute value); Reverb Tank drive and the Freeze knob have no
// such member (Reverb::Process computes tankDrive as a Process()-local, and
// DelayParams::dfrz lives on a RouteAudioSample()-local DelayParams), so
// T2.2 added TestLastReverbTankDriveKnobEffective()/
// TestLastDelayFreezeKnobEffective() (FroggersAppCore.hpp) for this test.
//
// T2.5 extends this SAME test rather than adding a new one: Grit (Reverb
// slot 11) joins the override, resolving to 0.0f (its exact bit-identical
// bypass by construction -- dsp::DigitalReorganizer at default, Reverb.hpp)
// -- same "no member to read back" situation as Tank drive/Freeze, so
// TestLastReverbGritKnobEffective() was added the same way.
// =========================================================================
TEST_CASE(stopped_transport_overrides_drive_and_freeze_to_unity_zero_and_resumes_bit_exact) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("stop_overrides_drive_freeze"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    using synth_froggers::FroggersBankId;

    model.PageParameter(FroggersBankId::Filter, 12).SceneCenter(0) = 1.0f;  // Comb drive commanded MAX.
    model.PageParameter(FroggersBankId::Delay, 9).SceneCenter(0) = 1.0f;    // Feedback drive commanded MAX.
    model.PageParameter(FroggersBankId::Reverb, 10).SceneCenter(0) = 1.0f;  // Tank drive commanded MAX.
    model.PageParameter(FroggersBankId::Delay, 4).SceneCenter(0) = 1.0f;    // Freeze commanded MAX.
    // T2.5: Grit (Reverb slot 11) commanded MAX -- joins the same
    // stopped-state override this test already pins for the other four.
    model.PageParameter(FroggersBankId::Reverb, 11).SceneCenter(0) = 1.0f;  // Grit commanded MAX.
    ApplyPatchNow(rig);

    // Deliberately no rig.StartAt(...) -- transport starts Stopped by
    // default (this file's own header comment, and silent_while_transport_
    // is_stopped above relies on the same starting point). A few blocks so
    // RouteAudioSample() actually executes the commanded-max patch while
    // stopped.
    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.SawNaN());

    // Effective values: unity/zero, not the commanded max. dsp::Reverb::
    // TankDriveFromKnob is the SAME public static map Reverb::Process
    // itself calls -- reused here rather than re-derived, so this
    // assertion cannot silently drift from the real mapping if that range
    // is ever retuned (a test that retypes a production formula is a
    // second definition site of it).
    REQUIRE_TRUE(rig.Application().TestFilterComb().combDrive == 1.0f);
    REQUIRE_TRUE(rig.Application().TestDelay().fbDrive == 1.0f);
    REQUIRE_TRUE(rig.Application().TestLastReverbTankDriveKnobEffective() == 0.5f);
    REQUIRE_TRUE(dsp::Reverb::TankDriveFromKnob(rig.Application().TestLastReverbTankDriveKnobEffective()) == 1.0f);
    REQUIRE_TRUE(rig.Application().TestLastDelayFreezeKnobEffective() == 0.0f);
    // T2.5: Grit's effective value reads 0.0f (its own exact bit-identical
    // bypass by construction, dsp/Reverb.hpp:526-527), not the commanded
    // MAX -- same override, joining the four above.
    REQUIRE_TRUE(rig.Application().TestLastReverbGritKnobEffective() == 0.0f);

    // WITHOUT writing to the parameter model: the commanded values set
    // above are exactly what was written, not silently rewritten to the
    // override's unity/zero.
    REQUIRE_TRUE(model.PageParameter(FroggersBankId::Filter, 12).CachedKnobValue(0) == 1.0f);
    REQUIRE_TRUE(model.PageParameter(FroggersBankId::Delay, 9).CachedKnobValue(0) == 1.0f);
    REQUIRE_TRUE(model.PageParameter(FroggersBankId::Reverb, 10).CachedKnobValue(0) == 1.0f);
    REQUIRE_TRUE(model.PageParameter(FroggersBankId::Delay, 4).CachedKnobValue(0) == 1.0f);
    REQUIRE_TRUE(model.PageParameter(FroggersBankId::Reverb, 11).CachedKnobValue(0) == 1.0f);

    // Resume play: the override stops applying and the never-touched
    // commanded MAX values reach the DSP units bit-exactly -- checked
    // against the real mapping formula's own value at knob==1.0, not
    // merely "no longer 1.0f/0.0f".
    rig.StartAt(4);
    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.SawNaN());

    const float kExpectedMaxDrive = dsp::ExpMapCompute(0.25f, 4.0f, 1.0f);  // same [0.25,4.0] map all three drive pre-gains share.
    REQUIRE_TRUE(rig.Application().TestFilterComb().combDrive == kExpectedMaxDrive);
    REQUIRE_TRUE(rig.Application().TestDelay().fbDrive == kExpectedMaxDrive);
    REQUIRE_TRUE(rig.Application().TestLastReverbTankDriveKnobEffective() == 1.0f);
    REQUIRE_TRUE(dsp::Reverb::TankDriveFromKnob(rig.Application().TestLastReverbTankDriveKnobEffective()) ==
                 kExpectedMaxDrive);
    REQUIRE_TRUE(rig.Application().TestLastDelayFreezeKnobEffective() == 1.0f);
    // T2.5: Grit's commanded MAX (1.0f) reaches the DSP unit bit-exactly on
    // resume too -- the override was never a write to the parameter model,
    // so `stoppedKnob` just returns `knob(bank, slot)` unchanged the instant
    // `wasTransportRunning_` is true again, same as the other four.
    REQUIRE_TRUE(rig.Application().TestLastReverbGritKnobEffective() == 1.0f);
}

// =========================================================================
// T2.3(b) pins T2.1's forced-release addition directly, isolated from
// T1.1's ramp bound -- Curve (Envelope
// slot 12) stays at its default 0.0f (the untouched linear ComputeRampStep
// path, no ramp-progress-floor arithmetic even runs), so a pass here can
// only be T2.1's doing. Attack VCO1 (slot 0) is pinned near its own
// ceiling (kMaxAttackSeconds, 0.5f post-W5) and the run is stopped well
// inside that 0.5s window, so the voice is still genuinely mid-Attack (not
// yet Hold) at the moment Stop lands. Grace (slot 13) is pinned to its own
// ceiling (kMaxGraceSeconds, 1.0f): WITHOUT T2.1, a pending release
// deliberately defers through Attack/Decay to Hold and only THEN starts
// this 1.0s countdown (VoiceEnvelope.hpp's own Grace comment) -- stage
// completion (>=0.4s remaining Attack + Decay) plus this 1.0s grace puts
// pre-T2.1 AllIdle comfortably past 2s, matching this task's own "NOT
// stage-completion + grace (~2s+)" framing. T2.1 forces Release
// immediately at the edge instead, bounded only by the existing ~50ms
// kStopFadeReleaseKnob fade -- AllIdle within the fade time (~0.1s).
// =========================================================================
TEST_CASE(stop_forces_release_from_mid_attack_bypassing_grace_and_stage_completion) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("stop_forces_release_mid_attack"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    using synth_froggers::FroggersBankId;

    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;  // VCO1 pitch.
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;  // Drive gain.
    model.PageParameter(FroggersBankId::Envelope, 0).SceneCenter(0) = 1.0f;   // Attack VCO1: ceiling (~0.5s).
    model.PageParameter(FroggersBankId::Envelope, 13).SceneCenter(0) = 1.0f;  // Grace: ceiling (~1.0s).
    // Curve (slot 12) left at its 0.0f default -- explicit per this test's
    // own "isolated from T1.1" framing, not merely relying on the registered
    // default silently.
    model.PageParameter(FroggersBankId::Envelope, 12).SceneCenter(0) = 0.0f;
    ApplyPatchNow(rig);

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;
    constexpr double kSampleRateHz = 48000.0;
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
    };

    // 0.1s: well inside VCO1's 0.5s Attack ceiling, and well inside the
    // 120 BPM default tempo's first (open) gate half (0.25s -- MasterClock::
    // kDefaultTempoBpm), so the gate never closes/reopens under this voice
    // before Stop lands.
    rig.RunBlocks(secondsToBlocks(0.1));
    timestamp += secondsToBlocks(0.1);
    REQUIRE_TRUE(!rig.SawNaN());
    // Precondition this test needs: still genuinely non-idle (mid-Attack),
    // not already settled -- otherwise the AllIdle() check below would pass
    // vacuously regardless of T2.1.
    REQUIRE_TRUE(!rig.Application().TestAudioAdsr().AllIdle());

    rig.StopAt(timestamp);

    // T2.1's bound: AllIdle within the fade time, generously budgeted to
    // 0.2s (comfortably above the ~50ms+wet-tail-irrelevant fade, since
    // AllIdle() only measures the ADSR stage, not delay/reverb) -- NOT the
    // pre-fix ~2s+ (stage completion + grace).
    rig.RunBlocks(secondsToBlocks(0.2));
    REQUIRE_TRUE(rig.Application().TestAudioAdsr().AllIdle());
}

// =========================================================================
// T6: T2.4's claim above -- "latching the
// Freeze button while stopped is a no-op on the audio" -- is SUPERSEDED and
// exactly backwards: the sustained-drive drone the button exists to
// reproduce only ever existed with the transport stopped, and
// T2.1/T2.2/T2.4 together made it unreachable. The old test above
// (deleted) proved the latch was a no-op by engaging it AFTER Stop, onto
// an already-silenced instrument --
// which is still true (nothing to hold once already torn down) but was
// never the operator's scenario and does not exercise T6.1/T6.2 at all.
// spec.md's own scenario order is latch-THEN-Stop; the three tests below
// (T6.4 a/b/c) follow that order instead.
//
// T7: T6 reached the drone only via Freeze+Stop, and left the latch armed
// through Stop -- so a button labelled Stop could conditionally sustain
// instead of silence. Freeze is now SELF-CONTAINED: engaging it stops the
// transport itself (FroggersUiSurface.hpp's kFreeze branch pushes
// MessageIn::Stop and calls SetDesiredTransportRunning(false) on engage, the
// same as kStop), so BuildLatchedRingHeldAcrossStop below drives the drone
// through a single Freeze press -- no separate Stop press reaches it
// anymore -- and Stop's own branch now also disarms the latch
// unconditionally. Both handler branches are driven through the real
// DispatchAction -> HandleAction path below (PressFreeze/PressStop), never
// SetFreezeLatched() directly: a direct flag write bypasses the exact
// handler logic T7.1/T7.2 add.
// =========================================================================

// Drives the Freeze/Stop transport BUTTONS through the real UI action path
// (DispatchAction -> FroggersUiSurface::HandleAction) rather than the app
// flags directly -- see this section's own T7 comment above for why that
// distinction matters for these particular two buttons.
void PressFreeze(Rig& rig) {
    rig.Application().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kFreeze));
}

void PressStop(Rig& rig) {
    rig.Application().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
}

// T7.7: Play, through the same real action path, for the one branch T7's
// own tests left unpinned.
void PressPlay(Rig& rig) {
    rig.Application().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
}

// Shared setup for T6.4(a)/(b)/(c): the SAME self-sustaining-ring recipe as
// stopping_transport_silences_self_sustaining_delay_and_reverb above
// (feedback/hold pushed to their near-unity extremes, so there is real
// recirculating energy for the latch to hold), but with the Freeze BUTTON
// pressed WHILE RUNNING -- spec.md's "The Freeze button alone reaches the
// sustained drone" scenario (T7). Leaves the rig stopped-and-latched, with
// the ring already confirmed audible immediately beforehand (the
// ringingPeak check below), so every caller starts from a genuinely live
// drone, not an assumed one.
// Same value the deleted T2.4 test and stopping_transport_silences_self_
// sustaining_delay_and_reverb both used for "actually ringing" (~-40 dBFS)
// -- named distinctly from those tests' own LOCAL kRingingFloorLinear so
// this one file-scope constant can be shared by BuildLatchedRingHeldAcrossStop
// and every T6.4 test below without colliding with those unrelated locals.
constexpr float kFrozenRingFloorLinear = 1.0e-2f;

std::uint64_t BuildLatchedRingHeldAcrossStop(Rig& rig) {
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    using synth_froggers::FroggersBankId;

    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
    model.PageParameter(FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;  // Send.
    model.PageParameter(FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;  // Feedback -> 0.98.
    model.PageParameter(FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;  // Wet mix.
    model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0) = 0.08f;  // Hold -> moderate.
    model.PageParameter(FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;   // Wet/dry fully wet.

    rig.StartAt(0);
    std::uint64_t timestamp = 0;

    constexpr std::size_t kExciteBlocks = 300;
    rig.RunBlocks(kExciteBlocks);
    timestamp += kExciteBlocks;
    REQUIRE_TRUE(!rig.SawNaN());

    rig.ClearOutput();
    constexpr std::size_t kConfirmRingBlocks = 40;
    rig.RunBlocks(kConfirmRingBlocks);
    timestamp += kConfirmRingBlocks;
    const float ringingPeak = PeakAbs(rig.Output());
    REQUIRE_TRUE(ringingPeak > kFrozenRingFloorLinear);  // actually ringing before trusting anything below.

    // T7.1: a single Freeze press now both engages the latch AND stops the
    // transport (FroggersUiSurface.hpp's kFreeze branch) -- no separate Stop
    // press needed or wanted here anymore (T6's version of this helper
    // pushed SetFreezeLatched(true) directly, then a separate rig.StopAt(...);
    // both are wrong under T7).
    PressFreeze(rig);

    return timestamp;
}

// T7.3(a): Freeze pressed while playing, with NO Stop press -- the
// transport reads
// stopped (TransportRunning() false) AND output stays above an audible
// floor PAST the bound an unlatched Stop must meet (stopping_transport_
// silences_self_sustaining_delay_and_reverb's own 0.25s settle window), the
// inverse of that test's own silence assertion. Checked twice (settle mark,
// then a further stretch) to prove the drone HOLDS rather than merely
// decaying slower. Was named ..._before_stop_...; BuildLatchedRingHeldAcrossStop
// no longer presses Stop at all (T7.1), so the old name is now false --
// renamed rather than left describing a step this test no longer takes.
TEST_CASE(freeze_alone_holds_the_ring_above_an_audible_floor_and_stops_the_transport) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("freeze_alone_holds_ring_stops_transport"));
    BuildLatchedRingHeldAcrossStop(rig);

    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);
    (void)kSilenceFloorLinear;  // Not the assertion here -- this test asserts the INVERSE.

    rig.RunBlocks(settleLeadBlocks);
    // T7.1: the Freeze press itself (inside BuildLatchedRingHeldAcrossStop,
    // no separate Stop press) must have stopped the transport -- this is
    // the "transport reads stopped" half of T7.3(a)'s two-part claim.
    REQUIRE_TRUE(!rig.Application().TransportRunning());
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& heldOutput = rig.Output();
    RequireFiniteStereo(heldOutput);
    REQUIRE_TRUE(PeakAbs(heldOutput) > kFrozenRingFloorLinear);  // still audible, past the bound an unlatched Stop must meet.

    // And it must STAY held, not merely still be mid-decay at the first
    // checkpoint -- same "stays there" shape the silence tests use, proving
    // the opposite property.
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& stillHeldOutput = rig.Output();
    RequireFiniteStereo(stillHeldOutput);
    REQUIRE_TRUE(PeakAbs(stillHeldOutput) > kFrozenRingFloorLinear);
    REQUIRE_TRUE(!rig.Application().TransportRunning());  // still stopped -- nothing restarted it.
}

// T6.4(b) is still a live scenario under T7: Freeze pressed again tears
// down and silences, with the transport staying stopped. A second Freeze
// press, releasing the latch while stopped, is the escape hatch out of the
// drone -- it must silence within the SAME bound an unlatched Stop
// guarantees. Now driven
// through PressFreeze() (DispatchAction -> HandleAction) rather than a
// direct SetFreezeLatched(false) call, per this task's rule that these
// tests exercise the real handler, not the flag.
TEST_CASE(freeze_latch_release_while_stopped_silences_within_the_bound) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("freeze_latch_release_while_stopped_silences"));
    BuildLatchedRingHeldAcrossStop(rig);

    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);

    // Confirm the drone is genuinely still held immediately before
    // releasing the latch -- the positive control this test's silence
    // claim below needs: a "silences" result means nothing if there was
    // nothing sounding to silence.
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    REQUIRE_TRUE(PeakAbs(rig.Output()) > kFrozenRingFloorLinear);

    // Release the latch with a second Freeze press -- still stopped, no
    // Play in between. T7.1: RELEASE does not start the transport.
    PressFreeze(rig);

    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& silencedOutput = rig.Output();
    RequireFiniteStereo(silencedOutput);
    REQUIRE_TRUE(PeakAbs(silencedOutput) < kSilenceFloorLinear);
    REQUIRE_TRUE(!rig.Application().TransportRunning());  // T7.1: release never starts the transport.
}

// T7.3(b): the NEW behaviour T6 got backwards -- Stop, pressed while
// Freeze is engaged and the drone is sustaining, must disarm the latch AND
// silence within the bound, exactly as any other Stop (spec.md's "Stop
// always means stop" scenario). Under T6 this used to SUSTAIN instead;
// this is the direct regression test for that bug, distinct from T6.4(b)
// above (which exits the drone via a second Freeze press, not Stop).
TEST_CASE(stop_disarms_the_latch_and_silences_the_held_drone_within_the_bound) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("stop_disarms_latch_silences_drone"));
    BuildLatchedRingHeldAcrossStop(rig);

    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);

    // Positive control: the drone is genuinely live immediately before
    // Stop.
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    REQUIRE_TRUE(PeakAbs(rig.Output()) > kFrozenRingFloorLinear);
    REQUIRE_TRUE(rig.Application().FreezeLatched());  // still latched right before Stop.

    PressStop(rig);

    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& silencedOutput = rig.Output();
    RequireFiniteStereo(silencedOutput);
    REQUIRE_TRUE(PeakAbs(silencedOutput) < kSilenceFloorLinear);
    REQUIRE_TRUE(!rig.Application().FreezeLatched());  // T7.2: Stop disarms the latch.
    REQUIRE_TRUE(!rig.Application().TransportRunning());
}

// T7.3(c): no sequence of Freeze/Stop presses may leave the
// instrument sounding after a Stop -- exercises the three sequences the
// task names at minimum, each on a fresh rig with the same self-sustaining
// recipe BuildLatchedRingHeldAcrossStop's ring uses (inlined here rather
// than reusing that helper, since two of the three sequences below need a
// DIFFERENT press order than the helper's own "Freeze once" shape).
void RequireSilentAfter(Rig& rig, const char* label) {
    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);
    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& output = rig.Output();
    RequireFiniteStereo(output);
    const float peak = PeakAbs(output);
    std::cout << "T7.3(c) " << label << ": peak after final Stop=" << peak << "\n";
    REQUIRE_TRUE(peak < kSilenceFloorLinear);
    REQUIRE_TRUE(!rig.Application().FreezeLatched());
    REQUIRE_TRUE(!rig.Application().TransportRunning());
}

TEST_CASE(no_freeze_stop_press_sequence_leaves_the_instrument_sounding_after_stop) {
    // Sequence 1: Freeze -> Stop (engage the drone, then Stop over it).
    {
        Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("freeze_stop_sequence_1"));
        BuildLatchedRingHeldAcrossStop(rig);  // presses Freeze once, confirms the ring first.
        PressStop(rig);
        RequireSilentAfter(rig, "Freeze->Stop");
    }
    // Sequence 2: Freeze -> Freeze -> Stop (engage, release via a second
    // Freeze press, then Stop on an already-unlatched-and-silent instrument
    // -- Stop must still be a no-op-safe unconditional silence, not assume
    // something is left to tear down).
    {
        Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("freeze_stop_sequence_2"));
        BuildLatchedRingHeldAcrossStop(rig);
        PressFreeze(rig);  // release.
        PressStop(rig);
        RequireSilentAfter(rig, "Freeze->Freeze->Stop");
    }
    // Sequence 3: Stop -> Freeze -> Stop (Stop while already unlatched and
    // playing, then Freeze re-engages and re-stops the transport, holding a
    // fresh drone, then a second Stop must disarm and silence it again).
    {
        Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("freeze_stop_sequence_3"));
        synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
        using synth_froggers::FroggersBankId;
        model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
        model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
        model.PageParameter(FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;
        model.PageParameter(FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;
        model.PageParameter(FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;
        model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0) = 0.08f;
        model.PageParameter(FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;

        rig.StartAt(0);
        rig.RunBlocks(300);
        REQUIRE_TRUE(!rig.SawNaN());

        PressStop(rig);  // Stop while unlatched and playing: must silence, same as any other Stop.
        RequireSilentAfter(rig, "Stop (first, unlatched)");

        // Play again, re-build the ring, then Freeze re-engages and
        // re-stops the transport, holding a fresh drone.
        rig.StartAt(0);
        rig.RunBlocks(300);
        REQUIRE_TRUE(!rig.SawNaN());
        rig.ClearOutput();
        rig.RunBlocks(40);
        REQUIRE_TRUE(PeakAbs(rig.Output()) > kFrozenRingFloorLinear);  // positive control: really ringing again.
        PressFreeze(rig);
        REQUIRE_TRUE(rig.Application().FreezeLatched());

        PressStop(rig);
        RequireSilentAfter(rig, "Stop->Freeze->Stop (final)");
    }
}

// T7.3(d): releasing Freeze must NOT restart the transport --
// the operator resumes with Play, not by releasing the latch.
// T7.7 (operator 2026-08-17, found in the built app: "why does clicking play
// not de-select freeze"). Play disarms the latch for the same reason Stop
// does -- and more urgently, because a latched Freeze holds the voice gate
// open unconditionally (FroggersAppCore's `setGate(gateOpen ||
// FreezeLatched())`). Starting the transport with the latch still engaged
// would run the sequencer with every voice pinned sustaining and the delay
// still at its latch overdrive, so Play would not actually return the
// instrument to playing.
//
// This was the single behaviour T7's own tests did not pin: the
// Play branch was written last, by hand, and every other Freeze/Stop
// sequence had a test while this one did not. Without this case, deleting
// `SetFreezeLatched(false)` from the kPlay handler leaves the whole suite
// green.
TEST_CASE(play_disarms_the_freeze_latch_and_returns_the_voice_gate_to_the_transport) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("play_disarms_freeze_latch"));
    BuildLatchedRingHeldAcrossStop(rig);

    rig.RunBlocks(4);
    // Positive control: the preconditions this test needs must actually
    // hold before Play is pressed, or "the latch cleared" would be
    // provable by an instrument that was never latched in the first place.
    REQUIRE_TRUE(rig.Application().FreezeLatched());
    REQUIRE_TRUE(!rig.Application().TransportRunning());

    PressPlay(rig);
    rig.RunBlocks(4);

    REQUIRE_TRUE(!rig.Application().FreezeLatched());   // the fix under test.
    REQUIRE_TRUE(rig.Application().TransportRunning());  // Play still starts the transport.
    REQUIRE_TRUE(!rig.SawNaN());
}

TEST_CASE(releasing_freeze_does_not_restart_the_transport) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("releasing_freeze_does_not_restart_transport"));
    BuildLatchedRingHeldAcrossStop(rig);

    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.Application().TransportRunning());  // Freeze engage stopped it (T7.1).

    PressFreeze(rig);  // release.
    rig.RunBlocks(4);
    REQUIRE_TRUE(!rig.Application().FreezeLatched());
    REQUIRE_TRUE(!rig.Application().TransportRunning());  // still stopped -- release did not restart it.
}

// T6.4(c): parameter edits stay live while frozen -- an encoder edit made
// while the drone is held must change the output measurably, with a
// positive control proving the drone was live immediately before the edit
// (a null result from an already-dead instrument would be void). Edits
// Delay Mix (bank Delay slot
// 6, "6=Mix" per MapRowsToDelayParams's own comment) from fully wet to
// fully dry -- a post-gain crossfade applied every sample to the delay's
// own (already self-sustaining) output, so its effect on an ALREADY-
// ringing signal is immediate, not dependent on new input reaching the
// tank.
TEST_CASE(encoder_edit_while_frozen_changes_the_output_measurably) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("encoder_edit_while_frozen_changes_output"));
    BuildLatchedRingHeldAcrossStop(rig);
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    using synth_froggers::FroggersBankId;

    const auto [settleLeadBlocks, checkWindowBlocks, kSilenceFloorLinear] =
        ComputeSilenceSettleWindow(/*settleSeconds=*/0.25);
    (void)kSilenceFloorLinear;

    rig.RunBlocks(settleLeadBlocks);
    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const float peakBeforeEdit = PeakAbs(rig.Output());
    // Positive control: the drone must be genuinely audible right here,
    // immediately before the edit below, or a measured change (or lack of
    // one) proves nothing.
    REQUIRE_TRUE(peakBeforeEdit > kFrozenRingFloorLinear);

    // The "encoder edit" -- still latched, still stopped, no Play in
    // between. Deliberately NOT using ApplyPatchNow/ComputeAllParameters()
    // here (same reasoning as patch_change_still_reaches_dsp_while_
    // transport_stopped's own comment, above in this file): that helper
    // writes CachedKnobValue() directly, bypassing parameters_.
    // ProcessSample()'s smoothed per-sample Compute entirely -- so it would
    // still pass even if THIS task's actual claim (ProcessSample() itself
    // stays ungated by transport state) were broken. A plain SceneCenter
    // write exercises the real path an operator's encoder turn uses.
    model.PageParameter(FroggersBankId::Delay, 6).SceneCenter(0) = 0.0f;  // Mix: fully wet -> fully dry.

    // B7.5.0's periodic smoothed Compute (alpha 0.0994 every 16 samples,
    // this file's own ApplyPatchNow comment) converges geometrically;
    // patch_change_still_reaches_dsp_while_transport_stopped's own comment
    // works the math for 50 blocks x 256 samples -- comfortably converged
    // well under float precision. Run that many BEFORE measuring so the
    // "after" window reads the settled value, not a mid-crossfade one.
    constexpr std::size_t kConvergeBlocks = 50;
    rig.RunBlocks(kConvergeBlocks);

    rig.ClearOutput();
    rig.RunBlocks(checkWindowBlocks);
    REQUIRE_TRUE(!rig.SawNaN());
    const auto& afterEditOutput = rig.Output();
    RequireFiniteStereo(afterEditOutput);
    const float peakAfterEdit = PeakAbs(afterEditOutput);

    // Measurable, not merely different in the noise: a self-sustaining ring
    // is not a pure tone, so consecutive non-overlapping windows differ by
    // a small amount even with NO edit at all -- measured directly (with
    // parameters_.ProcessSample() deliberately gated behind
    // `transportRunningNow`, so the edit above could not reach the DSP):
    // diff == 0.00627667 over this same 50-block gap. The real edit (Mix
    // 1.0 -> 0.0 collapsing delayOut toward the near-silent dry signal,
    // dsp/Delay.hpp's ToReverbMono) measures diff == 0.537887 -- ~86x that
    // noise floor. kMeasurableChangeLinear sits an order of magnitude above
    // the measured noise floor and comfortably below the measured true
    // effect, so this cannot pass on drift alone.
    const float peakDiff = std::fabs(peakBeforeEdit - peakAfterEdit);
    std::cout << "T6.4(c) encoder-edit-while-frozen: peakBeforeEdit=" << peakBeforeEdit
              << " peakAfterEdit=" << peakAfterEdit << " diff=" << peakDiff << "\n";
    constexpr float kMeasurableChangeLinear = 0.05f;  // noise floor 0.0063, true effect 0.538 (both measured, comment above).
    REQUIRE_TRUE(peakDiff > kMeasurableChangeLinear);
}

// =========================================================================
// S1a.2: operator-ordered gate on modulation_.Step() --
// NOT the F3 fix (see FroggersAppCore.hpp's own comment at the call site for
// why a static DC seed could never have been removed by freezing
// modulation; that is S1.3). This proves the actual behaviour change: with
// the transport stopped, a genuinely free-running modulation source holds
// its last value instead of continuing to update. Uses kModSlotNoise, one
// of the 8 slots the gate actually stops (kModSlotRandomSh6, kModSlotVco1/
// 2/3Audio, kModSlotVco1/2/3Ef, kModSlotNoise) -- the simplest of the 8, a
// fresh synth::NoiseModulatorProcessor::Process() draw (random_.
// UniformOpen01(), FroggersModulation.hpp) every Step() call, needing no
// patch/knob setup to prove liveness.
//
// Deliberately lives here, not in FroggersModulationTests.cpp: that file's
// own Fixture calls FroggersModulationSlate::Step() directly (its own
// header comment: "no Engine/SynthRig needed"), bypassing
// FroggersAppCore::ProcessBlock entirely -- exactly where this gate lives --
// so it structurally cannot observe this change. This file already drives
// ProcessBlock through the real synth_rig::SynthRig<FroggersApp> path (see
// e.g. the stopping_transport_silences_self_sustaining_delay_and_reverb
// tests above), so it is the correct home, following the same TEST_CASE/
// REQUIRE_TRUE convention FroggersModulationTests.cpp also uses (each test
// file in this directory defines its own copy; there is no shared header).
//
// A negative result (holds while stopped) requires a positive control
// (the SAME rig, SAME source, proven to move while running) in the SAME
// test, or the held-value read is meaningless. Both numbers are printed,
// not just asserted.
TEST_CASE(free_running_modulation_source_holds_while_stopped_with_positive_control) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("s1a2_source_holds_while_stopped"));
    const auto noiseValue = [&] {
        return rig.Application().Modulation().SourceValue(synth_froggers::kModSlotNoise);
    };

    // --- Positive control: transport RUNNING, sampled at every block
    // boundary (RunBlocks(1) chunking is bit-exact with an un-chunked call --
    // the same fact FroggersStopFlushRepro.cpp's runBlocksTrackingLiveness
    // and this file's own master_limiter_stays_at_unity_under_live_modulation
    // test above both already rely on). kModSlotNoise is a fresh open-(0,1)
    // draw every Step() call, so any nonzero spread over 40 independent
    // draws (short of an astronomically unlikely all-equal run) proves
    // liveness.
    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    constexpr std::size_t kLiveBlocks = 40;
    float liveMin = std::numeric_limits<float>::infinity();
    float liveMax = -std::numeric_limits<float>::infinity();
    for (std::size_t i = 0; i < kLiveBlocks; ++i) {
        rig.RunBlocks(1);
        ++timestamp;
        const float v = noiseValue();
        liveMin = std::min(liveMin, v);
        liveMax = std::max(liveMax, v);
    }
    std::cout << "S1a.2 positive control -- transport RUNNING, kModSlotNoise, " << kLiveBlocks
              << " block-boundary samples: min=" << liveMin << " max=" << liveMax
              << " range=" << (liveMax - liveMin) << "\n";
    // Same void-liveness role as FroggersStopFlushRepro.cpp's own
    // kVoidLivenessThreshold (1e-3f): comfortably above float noise, and a
    // uniform-(0,1) draw over 40 samples spans on the order of the full
    // [0,1) range in practice, so this margin is not close.
    constexpr float kLivenessThreshold = 1.0e-3f;
    REQUIRE_TRUE((liveMax - liveMin) > kLivenessThreshold);

    // --- Stop the transport; the source must now hold. ---
    rig.StopAt(timestamp);
    rig.RunBlocks(1);  // One block past the Stop message's own drain boundary.
    ++timestamp;

    const float heldValue = noiseValue();
    constexpr std::size_t kHeldBlocks = 40;
    for (std::size_t i = 0; i < kHeldBlocks; ++i) {
        rig.RunBlocks(1);
        REQUIRE_TRUE(noiseValue() == heldValue);
    }
    std::cout << "S1a.2 held value -- transport STOPPED, kModSlotNoise, constant at " << heldValue
              << " across " << kHeldBlocks << " further block-boundary samples\n";
}

// S1a.2 regression check: `parameters_.ProcessSample()` stays UNGATED by
// design (FroggersAppCore.hpp's own comment on the Step() call site) --
// gating it would freeze knob edits and Randomize All until Play, a worse
// bug than the one S1a.2 fixes. This proves that stays true: a raw
// SceneCenter write (the "commanded value" convention FroggersModulation
// Tests.cpp/FroggersParameterModelTests.cpp also use) still reaches
// CachedKnobValue() -- the value RouteAudioSample's knob()/vcoDrive actually
// read -- through the ordinary ProcessBlock path, with the transport never
// started at all (this rig's default state, per this file's own header
// comment: "The rig's transport starts Stopped by default").
TEST_CASE(patch_change_still_reaches_dsp_while_transport_stopped) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("s1a2_patch_reaches_dsp_while_stopped"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    synth::Parameter& driveGain = model.PageParameter(synth_froggers::FroggersBankId::Drive, 0);

    const float before = driveGain.CachedKnobValue(0);
    constexpr float kTarget = 0.8f;
    driveGain.SceneCenter(0) = kTarget;

    // B7.5.0's periodic smoothed Compute (Parameter::ProcessSamplePhase1,
    // alpha 0.0994 every 16 samples -- this file's own ApplyPatchNow comment
    // above) converges geometrically: residual after k updates is
    // (1-0.0994)^k. 50 blocks x 256 samples / 16 samples-per-update = 800
    // updates; (0.9006)^800 underflows float precision many times over.
    // Deliberately NOT using ApplyPatchNow/ComputeAllParameters() here --
    // the point of this test is to exercise the real per-sample path
    // (parameters_.ProcessSample(), called every sample regardless of
    // transport state) rather than the direct one-shot convergence helper.
    constexpr std::size_t kBlocks = 50;
    rig.RunBlocks(kBlocks);

    const float after = driveGain.CachedKnobValue(0);
    std::cout << "S1a.2 patch-while-stopped -- Drive gain CachedKnobValue(0): before=" << before
              << " target=" << kTarget << " after=" << after << " (transport never started)\n";
    REQUIRE_TRUE(!rig.SawNaN());
    // The write started meaningfully far from target (catches a vacuous
    // "already there" pass) and converged onto it (catches the regression
    // this test exists to rule out: parameters_.ProcessSample() silently
    // gated alongside modulation_.Step()).
    REQUIRE_TRUE(std::fabs(before - kTarget) > 0.1f);
    REQUIRE_TRUE(std::fabs(after - kTarget) < 1.0e-4f);
}

}  // namespace

int main() {
    int failed = 0;
    for (const auto& test : Registry()) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << "\n";
        } catch (const std::exception& ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << "\n";
        }
    }
    return failed == 0 ? 0 : 1;
}
