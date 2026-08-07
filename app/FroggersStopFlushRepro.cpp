// FroggersStopFlushRepro.cpp -- F3.1 (openspec/changes/
// frogg3rs-blowout-and-drilldown-repair/tasks.md): headless measurement
// harness for the operator's "Stop doesn't stop" report ("it has now been
// over a minute since i stopped audio, and it's still coming out ... the
// oscilloscope isn't moving. something is stuck in an infinite loop").
//
// NOT part of the regular suite (not wired into app/Makefile's `test`
// target, per F0.2's *Repro.cpp convention) -- a one-off investigation
// binary, built the same way FroggersCrunchyBlowupRepro.cpp documents:
//
//   cd /path/to/FroggersTiga && nice clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersStopFlushRepro.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_stop_flush_repro
//
// Purpose: measure, not fix (M1/systematic-debugging is binding for F1-F3 --
// no fix before the recorded root cause). Instruments every unit
// `RecoverPoisonedUnitState()` (FroggersAppCore.hpp:1377) walks -- via the
// existing TestXxx() "Test/inspection access" accessors it already exposes,
// plus two new ones this task adds (TestDelay()/TestReverb(), read-only,
// mirroring the same convention), and two new StateMagnitude() diagnostics
// on dsp::StereoDelay/dsp::Reverb themselves (they previously had only
// StateFinite() -- Tier 1 only, deliberately, per those methods' own
// comments -- these new methods are NOT wired into RecoverPoisonedUnitState,
// purely a read-only measurement addition) -- and prints a magnitude table
// at t+5s, t+30s and t+60s after Stop, on a patch with comb feedback at max
// and reverb Hold at max (the traced candidate's stated repro patch).
//
// Patch choice: ONLY comb feedback (Filter slot 5) and reverb Hold (Reverb
// slot 8) are pushed to their extremes, per F3.1's literal instruction --
// plus the minimum excitation (VCO pitch, Drive gain) the two existing
// sibling tests in FroggersAudioRoutingTests.cpp
// (stopping_transport_silences_self_sustaining_delay_and_reverb[_with_long_
// release]) already establish is needed for there to be any signal at all.
// No Delay-bank parameters are touched, so `delay_` is expected to read
// near-zero throughout (Send defaults to 0, dsp/Delay.hpp's own early-return
// guard) -- itself a useful confirming data point, not an oversight.
//
// ---------------------------------------------------------------------
// F3.2c EXTENSION (openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/
// tasks.md): re-run of the above, after F3.3 (1c37657) landed the
// all-14-unit Stop flush, with ONE deliberate delta -- an audio-rate
// modulation depth from kModSlotVco1Audio on Filter slot 5 (comb
// feedback), so `fb` genuinely sweeps while the transport is stopped and
// the comb has just been zeroed. RunPass() below runs MODULATED then
// CONTROL through the identical code path (the `withModulation` flag is
// the only difference), isolating the modulation depth as the one
// variable under test.
//
// This measurement's FIRST run (2026-08-07, commit 572e486) was INVALID
// and was retracted: it left Filter slot 5's base `SceneCenter(0)` at
// F3.1's 1.0f (comb feedback already at maximum) and added a
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

// F3.2c retraction requirement #2 (see the header comment): the void
// threshold separating "genuinely swept" from "pinned/clamped". A full-
// depth sweep is expected to span roughly the upper half of the knob's
// [0,1] range (order 0.1-0.5, tasks.md's own wording); a pinned/clamped
// knob -- the invalid first run's actual failure mode -- reads EXACTLY
// flat (max-min == 0.0f) or differs only by the currentCenter_ slew's
// already-settled residual (many orders of magnitude smaller, since the
// 2s pre-Stop excitation gives it far longer than its convergence time to
// reach target). 1e-3 sits three-plus orders of magnitude below a real
// sweep and comfortably above float noise, with wide margin either way.
constexpr float kVoidLivenessThreshold = 1e-3f;

struct PassResult {
    bool ok = true;  // false on a hard setup failure (EnsureModulationDepth == nullptr) or void liveness.
    float prestopPeak = 0.0f;
    float combKnobMin = std::numeric_limits<float>::infinity();
    float combKnobMax = -std::numeric_limits<float>::infinity();
    std::vector<Snapshot> snapshots;
};

// Runs F3.1's patch/schedule exactly, with one flag-gated delta (F3.2c's
// corrected re-run, see the header comment): when `withModulation` is
// true, Filter slot 5 (comb feedback) additionally gets a full-positive
// audio-rate modulation depth from kModSlotVco1Audio. `scratchLabel` gives
// each pass its own RuntimeDataPaths so the two Rig instances never share
// scratch state.
// F3.2d (2026-08-07, operator): the modulated LOOP is now a parameter, not a
// hardcoded one. F3.2c tested ONLY Filter slot 5 (comb feedback), found no
// sustain, and its result was written up as "parametric oscillation is dead as
// F3's root cause" -- generalizing from one stage to the whole mechanism, which
// the measurement never supported. The operator's own diagnosis: the parametric
// pumping shows up as the FILTER blowout (F2), while the infinite decay (F3) is
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
    // F3.2c correction #1 (header comment): base MID-RANGE, not 1.0f --
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
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> max, same as F3.1.

    // Every modulated target's base sits MID-RANGE so a non-negative
    // contribution has somewhere to travel (F3.2c correction #1) -- pinning a
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

    rig.Application().TestParameterManager().ComputeAllParameters();  // B7.5.0: converge exactly before RunBlocks.

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

    // F3.2c correction #2 (header comment): track Filter slot 5's
    // CachedKnobValue(0) at every block boundary across the WHOLE
    // post-Stop window below (both the "lead" and 20ms "window"
    // sub-phases of every checkpoint gap) by chunking every RunBlocks
    // call down to n=1 -- bit-exact with the un-chunked form, see header
    // comment. This is the modulation-liveness assertion the invalid
    // first run omitted.
    const auto runBlocksTrackingLiveness = [&](std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            rig.RunBlocks(1);
            const float knob = livenessParam.CachedKnobValue(0);
            result.combKnobMin = std::min(result.combKnobMin, knob);
            result.combKnobMax = std::max(result.combKnobMax, knob);
        }
    };

    // Checkpoints: a few early ones for context, plus the three F3.1 asks
    // for explicitly (5s/30s/60s).
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
            std::printf("  VOID: max-min <= %.3g -- modulation liveness NOT proven. "
                        "No refutation/confirmation may be drawn from this pass.\n",
                        static_cast<double>(kVoidLivenessThreshold));
            result.ok = false;
        } else {
            std::printf("  Liveness CONFIRMED: the comb-feedback knob genuinely swept "
                        "while the transport was stopped.\n");
        }
    }
    std::printf("\n");
    return result;
}

}  // namespace

int main() {
    using synth_froggers::FroggersBankId;

    // F3.2d: three passes, one variable -- WHICH feedback loop is modulated.
    // Delay slot 1 is Send and slot 2 is Feedback (FroggersParameters.hpp:175,
    // "Delay time", "Send", "Feedback"), so the delay pass modulates BOTH the
    // loop gain and the drive INTO the loop, which is the operator's reported
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

    return (delayMod.ok && combMod.ok && control.ok) ? 0 : 1;
}
