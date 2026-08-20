// FroggersStopFlushRepro.cpp -- headless measurement
// harness for a reported "Stop doesn't stop" bug ("it has now been
// over a minute since i stopped audio, and it's still coming out ... the
// oscilloscope isn't moving. something is stuck in an infinite loop").
//
// NOT part of the regular suite (not wired into app/Makefile's `test`
// target, per the *Repro.cpp convention) -- a one-off investigation
// binary, built the same way FroggersCrunchyBlowupRepro.cpp documents:
//
//   cd /path/to/FroggersTiga && nice clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersStopFlushRepro.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_stop_flush_repro
//
// Purpose: measure, not fix -- no fix before the recorded root cause.
// Instruments every unit
// `RecoverPoisonedUnitState()` (FroggersAppCore.hpp:1377) walks -- via the
// existing TestXxx() "Test/inspection access" accessors it already exposes,
// plus TestDelay()/TestReverb() (read-only,
// mirroring the same convention), and two StateMagnitude() diagnostics
// on dsp::StereoDelay/dsp::Reverb themselves (they previously had only
// StateFinite() -- Tier 1 only, deliberately, per those methods' own
// comments -- these new methods are NOT wired into RecoverPoisonedUnitState,
// purely a read-only measurement addition) -- and prints a magnitude table
// at t+5s, t+30s and t+60s after Stop, on a patch with comb feedback at max
// and reverb Hold at max (the traced candidate's stated repro patch).
//
// Patch choice: ONLY comb feedback (Filter slot 5) and reverb Hold (Reverb
// slot 8) are pushed to their extremes --
// plus the minimum excitation (VCO pitch, Drive gain) the two existing
// sibling tests in FroggersAudioRoutingTests.cpp
// (stopping_transport_silences_self_sustaining_delay_and_reverb[_with_long_
// release]) already establish is needed for there to be any signal at all.
// No Delay-bank parameters are touched, so `delay_` is expected to read
// near-zero throughout (Send defaults to 0, dsp/Delay.hpp's own early-return
// guard) -- itself a useful confirming data point, not an oversight.
//
// ---------------------------------------------------------------------
// EXTENSION: re-run of the above, after commit 1c37657 landed the
// all-14-unit Stop flush, with ONE deliberate delta -- an audio-rate
// modulation depth from kModSlotVco1Audio on Filter slot 5 (comb
// feedback), so `fb` genuinely sweeps while the transport is stopped and
// the comb has just been zeroed. RunPass() below runs MODULATED then
// CONTROL through the identical code path (the `withModulation` flag is
// the only difference), isolating the modulation depth as the one
// variable under test.
//
// This measurement's FIRST run (commit 572e486) was INVALID
// and was retracted: it left Filter slot 5's base `SceneCenter(0)` at
// the original patch's 1.0f (comb feedback already at maximum) and added a
// full-positive depth on top. `Parameter::GetRaw` is
// `ClampToRange(currentCenter_*scale + offset + ApplyActive(...), range)`
// (External/Sheaf/.../ParameterModulation.cpp:1211-1215) and
// `Modulators::ApplyActive` is `sum(values_[source]*depth)` with NO
// re-centering (:532-550); `kModSlotVco1Audio` is
// `NormalizeBipolarToUnit(vco1Raw)`, i.e. [0,1], non-negative. Base at the
// range ceiling plus a non-negative contribution clamps to the ceiling on
// EVERY sample, so `fb` was pinned, never swept -- the "modulated" run was
// physically the same system as the control, which is why it read
// bit-identical. The hypothesis was never tested. Two corrections fix
// that, both in RunPass() below:
//
//   1. Base moved to 0.5f (mid-range) so a non-negative contribution has
//      somewhere to travel.
//   2. Modulation liveness is now asserted and printed, not assumed:
//      `combKnobMin`/`combKnobMax` track `Parameter::CachedKnobValue(0)`
//      -- the exact value `ProcessLitePhase1()` recomputes fresh from
//      `GetRaw()` every sample, unconditionally, regardless of transport
//      state (External/Sheaf/.../ParameterModulation.cpp:1459-1461,
//      reached via `Parameter::ProcessSample` <-
//      `FroggersParameterModel::ProcessSample` <-
//      `modulation_.Step(...)`/`parameters_.ProcessSample(...)` in
//      FroggersAppCore.hpp's per-sample loop, both called unconditionally
//      every sample -- transport state is passed INTO Step as data, it
//      does not gate the call) -- sampled at every block boundary across
//      the WHOLE post-Stop window via `RunBlocksTrackingLiveness()`.
//      Chunking `RunBlocks(n)` down to n=1 calls is bit-exact with the
//      original single-call form (`SynthRig::RunBlocks` is just a loop of
//      `RunOneBlockAt(NextTimestamp())`, support/SynthRig.hpp:96-100), so
//      this changes nothing about the audio itself, only how often the
//      harness peeks at the knob. If `max - min` does not clear
//      `kVoidLivenessThreshold`, the pass prints VOID and the process
//      exit code goes non-zero: no refutation/confirmation may be drawn
//      from it.
//
// Route/bipolar-depth convention (EnsureModulationDepth returns nullptr
// at storage capacity, checked below; depth SceneCenter is bipolar -- 0.5
// knob == zero depth, 1.0 knob == full positive) verified against the
// existing master_limiter_stays_at_unity_under_live_modulation test,
// FroggersAudioRoutingTests.cpp:733-759.
// ---------------------------------------------------------------------
//
// STATUS UPDATE: VOID -- PREMISE ELIMINATED.
// Two things landed in this tree since the extension above was written:
//   1. The real cause was found and fixed, and it is NOT audio-rate
//      modulation: DigitalReorganizer::Process(0.0f) was nonzero for any
//      flip != 0 (exactly -1.0 at flip==128), a static DC seed re-emitted
//      identically every sample regardless of modulation. app/dsp/Drive.hpp
//      now returns `Mangle(input) - Mangle(0.0f)`, removing it at the
//      source. See the silent-chain gate case below for the gate that proves it.
//   2. FroggersAppCore.hpp's per-sample loop now calls
//      `modulation_.Step()` only `if (transportRunningNow)`.
//      Modulation sources HOLD their last value rather than reset when
//      stopped, so a downstream parameter's knob value can no longer SWEEP
//      post-Stop -- it can only sit at whatever constant it was already at.
// Together these mean the condition delayMod/combMod exist to test -- a
// coefficient sweeping at audio rate AFTER Stop, pumping a zeroed loop back
// up -- cannot occur under current code, by construction. Every RunPass()
// call below with a non-empty target will therefore read a flat post-Stop
// knob (max-min at or near 0.0f, under kVoidLivenessThreshold) and print
// "VOID -- premise eliminated" for exactly that reason: it is not an
// inconclusive sample that might read differently on a re-run, it is the
// transport gate working as designed. A run whose controlling
// quantity did not move is VOID, not a refutation, so this is NOT written up
// as "parametric modulation confirmed harmless" -- and PassResult::ok no
// longer clears on this branch (see its own comment), so it cannot fail
// this binary's exit code either. RunPass() and both passes are KEPT
// running (not deleted) so the historical measurement -- the actual
// knob-range and snapshot numbers -- stays in the record and reproducible
// on demand, rather than destroyed.
// ---------------------------------------------------------------------

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers stop-flush repro must not see JUCE headers"
#endif

#include <algorithm>
#include <cmath>
#include <span>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;
namespace dsp = synth_froggers::dsp;

namespace {

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-stop-flush-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

struct Snapshot {
    double tSeconds = 0.0;
    float vco1 = 0.0f, vco2 = 0.0f, vco3 = 0.0f;
    float driveBlendPhase = 0.0f;
    float oversampler = 0.0f, srr1 = 0.0f, srr2 = 0.0f;
    float peak = 0.0f, scoopNotch = 0.0f, comb = 0.0f;
    float delay = 0.0f, reverb = 0.0f;
    float outputLimiterEnv = 0.0f, peakLimiterEnv = 0.0f;
    float outputWindowPeak = 0.0f;  // trailing ~20ms window, the "can the operator still hear it" proxy.
};

Snapshot TakeSnapshot(Rig& rig, double tSeconds, float outputWindowPeak) {
    synth_froggers::FroggersApp& app = rig.Application();
    Snapshot s;
    s.tSeconds = tSeconds;
    s.vco1 = app.TestAudioVco(0).StateMagnitude();
    s.vco2 = app.TestAudioVco(1).StateMagnitude();
    s.vco3 = app.TestAudioVco(2).StateMagnitude();
    s.driveBlendPhase = app.TestDriveBlendPhase().StateMagnitude();
    s.oversampler = app.TestDriveOversampler().StateMagnitude();
    s.srr1 = app.TestSampleRateReducer(0).StateMagnitude();
    s.srr2 = app.TestSampleRateReducer(1).StateMagnitude();
    s.peak = app.TestFilterPeak().StateMagnitude();
    s.scoopNotch = app.TestFilterScoopNotch().StateMagnitude();
    s.comb = app.TestFilterComb().StateMagnitude();
    s.delay = app.TestDelay().StateMagnitude();
    s.reverb = app.TestReverb().StateMagnitude();
    s.outputLimiterEnv = app.TestOutputLimiter().envelope;
    s.peakLimiterEnv = app.TestFilterPeakLimiter().envelope;
    s.outputWindowPeak = outputWindowPeak;
    return s;
}

void PrintRow(const char* label, float value) {
    std::printf("  %-16s %s%.6g\n", label, value > 0.0f ? "" : " ", static_cast<double>(value));
}

void PrintSnapshot(const Snapshot& s) {
    std::printf("--- t+%.3fs post-Stop ---\n", s.tSeconds);
    PrintRow("audioVco1_", s.vco1);
    PrintRow("audioVco2_", s.vco2);
    PrintRow("audioVco3_", s.vco3);
    PrintRow("driveBlendPhase_", s.driveBlendPhase);
    PrintRow("drive.oversampler", s.oversampler);
    PrintRow("drive.srr1", s.srr1);
    PrintRow("drive.srr2", s.srr2);
    PrintRow("filterChain.peak", s.peak);
    PrintRow("filterChain.scoopNotch", s.scoopNotch);
    PrintRow("filterChain.comb", s.comb);
    PrintRow("delay_ (line memory)", s.delay);
    PrintRow("reverb_ (tank memory)", s.reverb);
    PrintRow("outputLimiter_.envelope", s.outputLimiterEnv);
    PrintRow("peakLimiter.envelope", s.peakLimiterEnv);
    PrintRow("OUTPUT (windowed peak)", s.outputWindowPeak);
}

// The void
// threshold separating "genuinely swept" from "pinned/clamped". A full-
// depth sweep is expected to span roughly the upper half of the knob's
// [0,1] range (order 0.1-0.5); a pinned/clamped
// knob -- the invalid first run's actual failure mode -- reads EXACTLY
// flat (max-min == 0.0f) or differs only by the currentCenter_ slew's
// already-settled residual (many orders of magnitude smaller, since the
// 2s pre-Stop excitation gives it far longer than its convergence time to
// reach target). 1e-3 sits three-plus orders of magnitude below a real
// sweep and comfortably above float noise, with wide margin either way.
constexpr float kVoidLivenessThreshold = 1e-3f;

struct PassResult {
    // False only on a hard setup failure (EnsureModulationDepth == nullptr). VOID liveness --
    // expected for any non-empty target now that modulation is transport-gated (see the
    // header comment) -- is reported via stdout only, in
    // RunPass() below, and deliberately does NOT clear this flag: a VOID run is
    // neither pass nor fail, so it must not be able to fail this binary's exit code either.
    bool ok = true;
    float prestopPeak = 0.0f;
    float combKnobMin = std::numeric_limits<float>::infinity();
    float combKnobMax = -std::numeric_limits<float>::infinity();
    std::vector<Snapshot> snapshots;
};

// Runs the patch/schedule described above exactly, with one flag-gated delta
// (see the header comment): when `withModulation` is
// true, Filter slot 5 (comb feedback) additionally gets a full-positive
// audio-rate modulation depth from kModSlotVco1Audio. `scratchLabel` gives
// each pass its own RuntimeDataPaths so the two Rig instances never share
// scratch state.
// The modulated LOOP is a parameter, not a
// hardcoded one: testing ONLY Filter slot 5 (comb feedback) alone
// finds no sustain there, and risks over-generalizing "parametric oscillation is dead as
// the root cause" from one stage to the whole mechanism, which a single-stage
// measurement cannot support. The parametric
// pumping shows up as the FILTER blowout, while the infinite decay is
// expressed in the DELAY, whose loop additionally has a modulated SEND feeding
// it. Same mechanism, different stage, different symptom -- so the target has
// to be a variable.
struct ModTarget {
    synth_froggers::FroggersBankId bank;
    std::size_t slot;
    const char* name;
};

PassResult RunPass(std::span<const ModTarget> targets, const char* scratchLabel, const char* passLabel) {
    PassResult result;
    constexpr double kSampleRateHz = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(scratchLabel));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    std::printf("################ %s ################\n", passLabel);

    // Minimum excitation so there is genuine signal to begin with (matches
    // the recipe FroggersAudioRoutingTests.cpp's sibling Stop tests use).
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;   // VCO1 pitch.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;   // Drive gain.
    // Base MID-RANGE, not 1.0f (see the header comment) --
    // the invalid first run pinned this at the ceiling, which is what
    // pinned `fb` regardless of modulation.
    synth::Parameter& combFeedback = model.PageParameter(synth_froggers::FroggersBankId::Filter, 5);
    combFeedback.SceneCenter(0) = 0.5f;
    // Liveness is asserted on the FIRST modulated target, whichever loop this
    // pass is exercising -- not on a hardcoded comb reference, which would
    // report a flat knob (and therefore VOID) for the delay pass while the
    // delay knob swept perfectly well. Control passes have no target and fall
    // back to comb, which correctly reads flat.
    const synth::Parameter& livenessParam =
        targets.empty() ? combFeedback
                        : model.PageParameter(targets[0].bank, targets[0].slot);
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> max.

    // Every modulated target's base sits MID-RANGE so a non-negative
    // contribution has somewhere to travel -- pinning a
    // base at its ceiling is what silently voided the first run.
    for (const ModTarget& target : targets) {
        synth::Parameter& param = model.PageParameter(target.bank, target.slot);
        param.SceneCenter(0) = 0.5f;
        synth::Parameter* depth = param.EnsureModulationDepth(synth_froggers::kModSlotVco1Audio);
        if (depth == nullptr) {
            std::printf("%s: EnsureModulationDepth returned nullptr for %s -- aborting this pass.\n",
                        passLabel, target.name);
            result.ok = false;
            return result;
        }
        depth->SceneCenter(0) = 1.0f;  // Bipolar: 0.5 == zero depth, 1.0 == full positive.
        std::printf("%s: modulating %s at audio rate (kModSlotVco1Audio, depth=+1.0)\n", passLabel, target.name);
    }

    rig.Application().TestParameterManager().ComputeAllParameters();  // Converge exactly before RunBlocks.

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;  // FroggersApp::Config(), matches FroggersAudioRoutingTests.cpp's comment.

    // Excite for 2s, confirm it's actually loud before trusting anything
    // measured after Stop (systematic-debugging: liveness before silence).
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
    };
    const std::size_t exciteBlocks = secondsToBlocks(2.0);
    rig.RunBlocks(exciteBlocks);
    timestamp += exciteBlocks;

    rig.ClearOutput();
    const std::size_t confirmBlocks = secondsToBlocks(0.1);
    rig.RunBlocks(confirmBlocks);
    timestamp += confirmBlocks;
    for (const auto& frame : rig.Output()) {
        for (float sample : frame.channels) result.prestopPeak = std::max(result.prestopPeak, std::fabs(sample));
    }
    std::printf("Pre-Stop peak (100ms window after 2s excitation): %.6g  (sawNaN=%d)\n",
                static_cast<double>(result.prestopPeak), rig.SawNaN());
    if (result.prestopPeak <= 0.1f) {
        std::printf("WARNING: instrument was not genuinely loud before Stop -- everything below is meaningless.\n");
    }

    // Stop the transport (mirrors FroggersAudioRoutingTests.cpp's
    // established StopAt(timestamp) idiom: timestamp is the exact running
    // tally of blocks already executed via RunBlocks, so the Stop message
    // lines up with the very next block).
    rig.StopAt(timestamp);
    std::printf("\nStop issued at timestamp=%llu (t=0 for everything below)\n\n",
                static_cast<unsigned long long>(timestamp));

    // Tracks Filter slot 5's
    // CachedKnobValue(0) at every block boundary across the WHOLE
    // post-Stop window below (both the "lead" and 20ms "window"
    // sub-phases of every checkpoint gap) by chunking every RunBlocks
    // call down to n=1 -- bit-exact with the un-chunked form (see the header
    // comment). This is the modulation-liveness assertion the invalid
    // first run omitted.
    const auto runBlocksTrackingLiveness = [&](std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            rig.RunBlocks(1);
            const float knob = livenessParam.CachedKnobValue(0);
            result.combKnobMin = std::min(result.combKnobMin, knob);
            result.combKnobMax = std::max(result.combKnobMax, knob);
        }
    };

    // Checkpoints: a few early ones for context, plus the three checkpoints
    // asked for explicitly (5s/30s/60s).
    const double checkpoints[] = {0.1, 1.0, 5.0, 30.0, 60.0};
    double elapsedSeconds = 0.0;
    for (double checkpoint : checkpoints) {
        const double remaining = checkpoint - elapsedSeconds;
        const std::size_t leadBlocks = secondsToBlocks(remaining) > secondsToBlocks(0.02)
            ? secondsToBlocks(remaining) - secondsToBlocks(0.02)
            : 0;
        runBlocksTrackingLiveness(leadBlocks);
        timestamp += leadBlocks;
        elapsedSeconds += static_cast<double>(leadBlocks) * static_cast<double>(blockSize) / kSampleRateHz;

        rig.ClearOutput();
        const std::size_t windowBlocks = secondsToBlocks(0.02);
        runBlocksTrackingLiveness(windowBlocks);
        timestamp += windowBlocks;
        elapsedSeconds += static_cast<double>(windowBlocks) * static_cast<double>(blockSize) / kSampleRateHz;

        float windowPeak = 0.0f;
        for (const auto& frame : rig.Output()) {
            for (float sample : frame.channels) windowPeak = std::max(windowPeak, std::fabs(sample));
        }
        const Snapshot snap = TakeSnapshot(rig, elapsedSeconds, windowPeak);
        PrintSnapshot(snap);
        std::printf("  sawNaN=%d\n\n", rig.SawNaN());
        result.snapshots.push_back(snap);
    }

    const float combKnobRange = result.combKnobMax - result.combKnobMin;
    std::printf("--- Modulation liveness (entire post-Stop window, t+0 to t+%.0fs): "
                "%s CachedKnobValue(0) ---\n",
                checkpoints[4], targets.empty() ? "(control: Filter slot 5)" : targets[0].name);
    std::printf("  min=%.6g  max=%.6g  max-min=%.6g\n", static_cast<double>(result.combKnobMin),
                static_cast<double>(result.combKnobMax), static_cast<double>(combKnobRange));
    if (!targets.empty()) {
        if (!(combKnobRange > kVoidLivenessThreshold)) {
            // VOID, not a refutation -- an EXPECTED void (see
            // the header comment): modulation is
            // transport-gated now, so this branch is the gate working, not an inconclusive
            // sample. Left as a live measurement rather than hand-waved to a constant, so a
            // future change to the transport gate would show up here as a real reading again.
            std::printf(
                "  VOID -- premise eliminated: max-min <= %.3g -- modulation liveness NOT proven.\n"
                "  Expected: modulation_.Step() is transport-gated, so no coefficient\n"
                "  can sweep post-Stop any more. The real cause of the original bug was the drive-stage DC seed, fixed\n"
                "  (app/dsp/Drive.hpp: Process now returns Mangle(input) - Mangle(0.0f)).\n"
                "  This is VOID, not a refutation -- it does NOT fail this binary\n"
                "  (PassResult::ok is untouched by this branch; see its own comment).\n",
                static_cast<double>(kVoidLivenessThreshold));
        } else {
            std::printf("  Liveness CONFIRMED: the comb-feedback knob genuinely swept "
                        "while the transport was stopped.\n");
        }
    }
    std::printf("\n");
    return result;
}

// =========================================================================
// Reproduces the original bug from a FULLY SILENT chain -- no
// modulation anywhere, every coefficient static. Unlike RunPass() above
// (measure only, no verdict -- this file's own header comment, "Purpose:
// measure, not fix"), this case is a GATE: it requires an
// assertion that FAILS before the fix and PASSES only after the fix lands
// ("WRITE THIS BEFORE THE FIX ... this one must be red first"). Mirrors the
// REQUIRE_TRUE convention every *Tests.cpp binary in this directory already
// defines for itself (FroggersAudioRoutingTests.cpp, FroggersDspParityTests.
// cpp, et al. -- each its own translation unit; there is no shared test
// header in this codebase to pull it from instead).
#define REQUIRE_TRUE(expr)                                                 \
    do {                                                                   \
        if (!(expr)) {                                                    \
            std::ostringstream oss;                                       \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #expr; \
            throw std::runtime_error(oss.str());                          \
        }                                                                  \
    } while (false)

// Same -60 dBFS "silence" convention FroggersAudioRoutingTests.cpp's
// stopping_transport_silences_self_sustaining_delay_and_reverb[_with_long_
// release] tests already use for "settled, and stays settled" -- reused
// here rather than re-derived.
constexpr float kSilenceFloorLinear = 1.0e-3f;

// The "first exceeded" marker for the scan below.
constexpr float kS12AudibleMarker = 0.1f;

struct F3ScanResult {
    float peak = 0.0f;
    long firstExceedsBlockIx = -1;  // -1 == kS12AudibleMarker never crossed within the scanned window.
    bool sawNaN = false;
};

// One block-by-block scan of a measurement window, recording the overall
// peak magnitude and the first block at which it crosses kS12AudibleMarker
// -- shared by all three RunF3SilentChainCase() runs below (the seeded case
// and its two mandatory controls), one definition, three call sites.
// Chunking RunBlocks() down to n=1 is bit-exact with an un-chunked
// call (SynthRig::RunBlocks is just a loop of RunOneBlockAt(NextTimestamp()),
// support/SynthRig.hpp -- the same fact this file's own
// runBlocksTrackingLiveness lambda above already relies on), so this changes
// nothing about the audio itself, only how often the harness looks.
F3ScanResult ScanPeakAndFirstCrossing(Rig& rig, std::size_t blocks) {
    F3ScanResult result;
    for (std::size_t i = 0; i < blocks; ++i) {
        rig.ClearOutput();
        rig.RunBlocks(1);
        float blockPeak = 0.0f;
        for (const auto& frame : rig.Output()) {
            for (float sample : frame.channels) blockPeak = std::max(blockPeak, std::fabs(sample));
        }
        result.peak = std::max(result.peak, blockPeak);
        if (result.firstExceedsBlockIx < 0 && blockPeak > kS12AudibleMarker) {
            result.firstExceedsBlockIx = static_cast<long>(i);
        }
        result.sawNaN = result.sawNaN || rig.SawNaN();
    }
    return result;
}

struct F3CaseResult {
    F3ScanResult scan;
    uint8_t observedFlip = 0;
    uint8_t observedHashBits = 0;
};

// Drives the REAL flush path -- Start, brief excitation so every voice
// genuinely leaves Idle (all three VCOs: the Audio bank's VCO1/2/3 pitch
// default to 110/220/330 Hz even unpatched, FroggersParameters.hpp's Audio
// bank layout, so the ASR gate excites all of kNumVoices==3, not just one),
// Stop, then run past RouteAudioSample's transport-stopped fast-release
// substitute (kStopFadeReleaseKnob, ~50ms, FroggersAppCore.hpp -- overrides
// the operator's own Release knob while stopped, so every voice reaches
// Stage::Idle quickly regardless of the Envelope-bank settings this case
// never touches) with ample margin, so the running->stopped edge's real
// ForEachStatefulUnit(Reset) flush (FroggersAppCore.hpp's Stop-transport-
// reset block) has already fired once the scan below starts. Never
// hand-resets anything -- the repro must drive
// the real code path, not hand-reset.
//
// `driveFlipKnob01`/`driveBlendKnob01` are the two knobs this case
// varies (Drive slots 4/7). Every other Drive-bank slot (2/3/5/6/8 -- SRR
// 1/2, Bit depth, Fuzz, Phase) is left at its ordinary 0.0f default
// (FroggersParameters.hpp's Drive bank layout carries no `defaultValue`
// override for any of them, unlike Audio's VCO pitches or Envelope's
// Sustain), and no ModulationDepth is ever registered on anything, on
// purpose: ALL coefficients static, NO modulation depths anywhere -- the
// symptom must reproduce with nothing sweeping.
F3CaseResult RunF3SilentChainCase(float driveFlipKnob01, float driveBlendKnob01, const char* scratchLabel,
                                   const char* label) {
    constexpr double kSampleRateHz = 48000.0;
    constexpr std::size_t kBlockSize = 256;  // FroggersApp::Config().
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(scratchLabel));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    std::printf("################ SILENT-CHAIN: %s ################\n", label);

    // Minimum excitation, same idiom RunPass() above uses -- just enough
    // that the ASR gate genuinely opens and every voice leaves Idle at
    // least once, so "every voice reaching Idle" is a
    // real transition, not a no-op on voices that were trivially Idle the
    // whole time.
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;  // VCO1 pitch.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;  // Drive gain.

    // The two knobs this case varies.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 4).SceneCenter(0) = driveFlipKnob01;  // XOR/Flip.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 7).SceneCenter(0) = driveBlendKnob01;  // Blend.

    rig.Application().TestParameterManager().ComputeAllParameters();  // Converge exactly before RunBlocks.

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(kBlockSize)));
    };

    const std::size_t exciteBlocks = secondsToBlocks(2.0);
    rig.RunBlocks(exciteBlocks);
    timestamp += exciteBlocks;

    // Verifies the knob actually reached flip == 128, rather than assuming
    // it maps as expected: reads the LIVE runtime value RouteAudioSample()
    // actually fed SetFlip() via CachedKnobValue() -- not the SceneCenter
    // argument this function was called with -- via the
    // TestDriveDigitalReorganizer() accessor (FroggersAppCore.hpp,
    // same read-only "Test/inspection access" convention as
    // TestDelay()/TestReverb() on that same accessor list).
    F3CaseResult result;
    result.observedFlip = rig.Application().TestDriveDigitalReorganizer().flip;
    result.observedHashBits = rig.Application().TestDriveDigitalReorganizer().hashBits;

    rig.StopAt(timestamp);

    // Past the ~50ms transport-stopped fast-release substitute with 2x
    // margin -- long enough that every voice has reached Idle and the real
    // ForEachStatefulUnit(Reset) flush has already fired before the scan
    // below starts, but deliberately NOT so long that the scan's own start
    // is past the climb it exists to observe (an earlier revision used 0.3s
    // here and the climb -- being much faster in this minimal patch than
    // the original capture's comb/reverb-at-extremes one, see this
    // function's own call sites: no Filter/Delay/Reverb slot is ever
    // touched, all default to 0.0f -- had already fully happened inside
    // that lead, so every scan started at post-flush-block 0 already
    // pinned; this shorter lead keeps the rise itself inside the scanned
    // window instead of behind it).
    constexpr double kLeadSeconds = 0.1;
    const std::size_t leadBlocks = secondsToBlocks(kLeadSeconds);
    rig.RunBlocks(leadBlocks);
    timestamp += leadBlocks;

    // Runs long enough to cover the climb: the real capture pinned by
    // ~block 40 at 256 frames/48 kHz, so a few seconds of samples is ample.
    // 3s of scanned blocks is ~7x that margin.
    constexpr double kMeasureSeconds = 3.0;
    const std::size_t measureBlocks = secondsToBlocks(kMeasureSeconds);
    result.scan = ScanPeakAndFirstCrossing(rig, measureBlocks);

    if (result.scan.firstExceedsBlockIx < 0) {
        std::printf("%s: flip=%u hashBits=%u peak=%.6g -- never exceeded %.1g in %zu post-flush blocks, sawNaN=%d\n",
                    label, static_cast<unsigned>(result.observedFlip), static_cast<unsigned>(result.observedHashBits),
                    static_cast<double>(result.scan.peak), static_cast<double>(kS12AudibleMarker), measureBlocks,
                    result.scan.sawNaN ? 1 : 0);
    } else {
        std::printf(
            "%s: flip=%u hashBits=%u peak=%.6g -- first exceeded %.1g at post-flush block %ld (~sample %ld), "
            "sawNaN=%d\n",
            label, static_cast<unsigned>(result.observedFlip), static_cast<unsigned>(result.observedHashBits),
            static_cast<double>(result.scan.peak), static_cast<double>(kS12AudibleMarker),
            result.scan.firstExceedsBlockIx, result.scan.firstExceedsBlockIx * static_cast<long>(kBlockSize),
            result.scan.sawNaN ? 1 : 0);
    }
    return result;
}

}  // namespace

int main() {
    using synth_froggers::FroggersBankId;

    // STATUS: delayMod/combMod below are VOID -- premise eliminated. See the file
    // header's "STATUS UPDATE" comment for the full why; short version: modulation is
    // transport-gated so their target knobs cannot move post-Stop any more, and the
    // bug itself
    // was fixed at its real cause -- a static DC seed in the drive stage -- not audio-rate
    // modulation. Both passes are KEPT RUNNING (not deleted) so the historical knob-range/snapshot
    // measurements stay reproducible; each prints its own "VOID -- premise eliminated" below, and
    // neither can fail this binary's exit code (PassResult::ok's comment; the final `return`).

    // Three passes, one variable -- WHICH feedback loop is modulated.
    // Delay slot 1 is Send and slot 2 is Feedback (FroggersParameters.hpp:175,
    // "Delay time", "Send", "Feedback"), so the delay pass modulates BOTH the
    // loop gain and the drive INTO the loop, which is the reported
    // condition and the one Randomize All guarantees.
    static constexpr ModTarget kCombTargets[] = {
        {FroggersBankId::Filter, 5, "Filter slot 5 (comb feedback)"},
    };
    static constexpr ModTarget kDelayTargets[] = {
        {FroggersBankId::Delay, 1, "Delay slot 1 (Send)"},
        {FroggersBankId::Delay, 2, "Delay slot 2 (Feedback)"},
    };

    const PassResult delayMod = RunPass(kDelayTargets, "f3_2d_delay",
                                        "DELAY MODULATED (Send + Feedback, base=0.5, kModSlotVco1Audio depth=+1.0)");
    const PassResult combMod = RunPass(kCombTargets, "f3_2c_mod",
                                       "COMB MODULATED (Filter slot 5, base=0.5, kModSlotVco1Audio depth=+1.0)");
    const PassResult control = RunPass({}, "f3_2c_control",
                                       "CONTROL (base=0.5, no modulation depth -- otherwise identical)");

    std::printf("################ SUMMARY: DELAY vs CONTROL ################\n");
    std::printf("pre-Stop peak:  delay=%.6g  comb=%.6g  control=%.6g\n",
                static_cast<double>(delayMod.prestopPeak), static_cast<double>(combMod.prestopPeak),
                static_cast<double>(control.prestopPeak));
    std::printf("modulated-knob range (post-Stop): delay=%.6g  comb=%.6g  control=%.6g\n",
                static_cast<double>(delayMod.combKnobMax - delayMod.combKnobMin),
                static_cast<double>(combMod.combKnobMax - combMod.combKnobMin),
                static_cast<double>(control.combKnobMax - control.combKnobMin));
    {
        const std::size_t rows = std::min({delayMod.snapshots.size(), combMod.snapshots.size(),
                                           control.snapshots.size()});
        for (std::size_t i = 0; i < rows; ++i) {
            std::printf("t+%.3fs  OUTPUT delay=%.6g comb=%.6g ctrl=%.6g\n", delayMod.snapshots[i].tSeconds,
                        static_cast<double>(delayMod.snapshots[i].outputWindowPeak),
                        static_cast<double>(combMod.snapshots[i].outputWindowPeak),
                        static_cast<double>(control.snapshots[i].outputWindowPeak));
        }
    }

    const PassResult& modulated = delayMod;
    std::printf("################ SUMMARY: MODULATED vs CONTROL ################\n");
    std::printf("pre-Stop peak:  modulated=%.6g  control=%.6g\n", static_cast<double>(modulated.prestopPeak),
                static_cast<double>(control.prestopPeak));
    std::printf("comb-feedback knob range (post-Stop): modulated max-min=%.6g  control max-min=%.6g\n",
                static_cast<double>(modulated.combKnobMax - modulated.combKnobMin),
                static_cast<double>(control.combKnobMax - control.combKnobMin));
    const std::size_t rows = std::min(modulated.snapshots.size(), control.snapshots.size());
    for (std::size_t i = 0; i < rows; ++i) {
        const Snapshot& m = modulated.snapshots[i];
        const Snapshot& c = control.snapshots[i];
        std::printf("t+%.3fs  OUTPUT mod=%.6g ctrl=%.6g | comb mod=%.6g ctrl=%.6g | driveBlendPhase mod=%.6g ctrl=%.6g\n",
                    m.tSeconds, static_cast<double>(m.outputWindowPeak), static_cast<double>(c.outputWindowPeak),
                    static_cast<double>(m.comb), static_cast<double>(c.comb),
                    static_cast<double>(m.driveBlendPhase), static_cast<double>(c.driveBlendPhase));
    }

    // =====================================================================
    // Reproduces the bug from a fully silent chain, all coefficients
    // static, no modulation anywhere -- the gate the fix must turn green.
    // =====================================================================
    std::printf("\n################ SILENT-CHAIN: FROM A FULLY SILENT CHAIN ################\n");
    // [128/255, 129/255) == [0.501960..., 0.505882...); SetFlip() truncates
    // (flip = uint8_t(flipKnob01*255.0f), dsp/Drive.hpp), so 0.503f sits
    // safely inside with margin on both sides -- landing on either edge
    // would risk flip==127 or flip==129 on any rounding noise.
    constexpr float kDriveFlipKnobFor128 = 0.503f;
    // Any positive blend; fully wet (1.0) removes the dry(==chainIn==0) term from
    // DriveBlendPhase::Process's crossfade entirely, leaving the cleanest
    // possible read on the wet path alone.
    constexpr float kDriveBlendKnobWet = 1.0f;

    const F3CaseResult seeded = RunF3SilentChainCase(kDriveFlipKnobFor128, kDriveBlendKnobWet, "s1_2_seeded",
                                                       "SEEDED (Flip->128, Blend=1.0)");
    const F3CaseResult flipZeroControl =
        RunF3SilentChainCase(0.0f, kDriveBlendKnobWet, "s1_2_flip0", "CONTROL: Flip=0 (Blend=1.0)");
    const F3CaseResult blendZeroControl =
        RunF3SilentChainCase(kDriveFlipKnobFor128, 0.0f, "s1_2_blend0", "CONTROL: Blend=0 (Flip->128)");

    std::printf("\n--- Silent-chain summary (peak magnitude, first post-flush block index > %.1g) ---\n",
                static_cast<double>(kS12AudibleMarker));
    std::printf("  seeded (Flip->128,Blend=1.0):  flip=%u peak=%.6g first-exceeds-block=%ld\n",
                static_cast<unsigned>(seeded.observedFlip), static_cast<double>(seeded.scan.peak),
                seeded.scan.firstExceedsBlockIx);
    std::printf("  control Flip=0 (Blend=1.0):    flip=%u peak=%.6g first-exceeds-block=%ld\n",
                static_cast<unsigned>(flipZeroControl.observedFlip), static_cast<double>(flipZeroControl.scan.peak),
                flipZeroControl.scan.firstExceedsBlockIx);
    std::printf("  control Blend=0 (Flip->128):   flip=%u peak=%.6g first-exceeds-block=%ld\n",
                static_cast<unsigned>(blendZeroControl.observedFlip), static_cast<double>(blendZeroControl.scan.peak),
                blendZeroControl.scan.firstExceedsBlockIx);

    int s1_2FailCount = 0;
    const auto s1_2Check = [&](const char* name, auto&& fn) {
        try {
            fn();
            std::printf("[PASS] %s\n", name);
        } catch (const std::exception& ex) {
            ++s1_2FailCount;
            std::printf("[FAIL] %s: %s\n", name, ex.what());
        }
    };

    // Preconditions: prove the rig actually did what was asked before
    // trusting anything measured from it (do not assume the knob maps as
    // expected).
    s1_2Check("flip_knob_maps_to_128_on_seeded_run", [&] { REQUIRE_TRUE(seeded.observedFlip == 128); });
    s1_2Check("flip_control_knob_maps_to_0", [&] { REQUIRE_TRUE(flipZeroControl.observedFlip == 0); });
    s1_2Check("blend_control_leaves_flip_at_128", [&] { REQUIRE_TRUE(blendZeroControl.observedFlip == 128); });

    // Mandatory positive control: the rig must be proven ABLE to
    // measure non-silence, or a clean read on the seeded run below proves
    // nothing. Both controls gate the seed (Flip's XOR, DriveBlendPhase's
    // crossfade) at a DIFFERENT point in the same signal path, so both must
    // read bit-exact zero -- not merely "quiet".
    s1_2Check("flip_zero_control_measures_exact_silence", [&] { REQUIRE_TRUE(flipZeroControl.scan.peak == 0.0f); });
    s1_2Check("blend_zero_control_measures_exact_silence", [&] { REQUIRE_TRUE(blendZeroControl.scan.peak == 0.0f); });

    // THE gate. Desired behaviour: the post-flush peak decays to and stays
    // at ~0 -- same -60 dBFS floor the sibling Stop-silences-the-instrument
    // tests use. MUST be red until the fix lands (do not
    // weaken this to "peak pins near 1.0", which would pass before the fix and fail
    // after it -- a characterization test, not a gate).
    s1_2Check("f3_post_flush_peak_decays_to_and_stays_near_zero",
              [&] { REQUIRE_TRUE(seeded.scan.peak < kSilenceFloorLinear); });

    std::printf("\nSilent-chain: %d check(s) failed (see [FAIL] lines above)\n", s1_2FailCount);

    // delayMod.ok/combMod.ok/control.ok now reflect ONLY a hard setup failure
    // (EnsureModulationDepth == nullptr), never VOID liveness -- see PassResult::ok's comment --
    // so the now-permanent-and-expected VOID reads no longer fail this binary. The silent-chain
    // gate (s1_2FailCount) remains the sole gate on the bug itself.
    return (delayMod.ok && combMod.ok && control.ok && s1_2FailCount == 0) ? 0 : 1;
}
