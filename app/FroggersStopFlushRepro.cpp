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

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers stop-flush repro must not see JUCE headers"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
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

}  // namespace

int main() {
    constexpr double kSampleRateHz = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("f3_1"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    // Minimum excitation so there is genuine signal to begin with (matches
    // the recipe FroggersAudioRoutingTests.cpp's sibling Stop tests use).
    model.PageParameter(synth_froggers::FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;   // VCO1 pitch.
    model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;   // Drive gain.
    // F3.1's stated patch.
    model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> max.
    model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> max.

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
    float prestopPeak = 0.0f;
    for (const auto& frame : rig.Output()) {
        for (float sample : frame.channels) prestopPeak = std::max(prestopPeak, std::fabs(sample));
    }
    std::printf("Pre-Stop peak (100ms window after 2s excitation): %.6g  (sawNaN=%d)\n",
                static_cast<double>(prestopPeak), rig.SawNaN());
    if (prestopPeak <= 0.1f) {
        std::printf("WARNING: instrument was not genuinely loud before Stop -- everything below is meaningless.\n");
    }

    // Stop the transport (mirrors FroggersAudioRoutingTests.cpp's
    // established StopAt(timestamp) idiom: timestamp is the exact running
    // tally of blocks already executed via RunBlocks, so the Stop message
    // lines up with the very next block).
    rig.StopAt(timestamp);
    std::printf("\nStop issued at timestamp=%llu (t=0 for everything below)\n\n",
                static_cast<unsigned long long>(timestamp));

    // Checkpoints: a few early ones for context, plus the three F3.1 asks
    // for explicitly (5s/30s/60s).
    const double checkpoints[] = {0.1, 1.0, 5.0, 30.0, 60.0};
    double elapsedSeconds = 0.0;
    for (double checkpoint : checkpoints) {
        const double remaining = checkpoint - elapsedSeconds;
        const std::size_t leadBlocks = secondsToBlocks(remaining) > secondsToBlocks(0.02)
            ? secondsToBlocks(remaining) - secondsToBlocks(0.02)
            : 0;
        rig.RunBlocks(leadBlocks);
        timestamp += leadBlocks;
        elapsedSeconds += static_cast<double>(leadBlocks) * static_cast<double>(blockSize) / kSampleRateHz;

        rig.ClearOutput();
        const std::size_t windowBlocks = secondsToBlocks(0.02);
        rig.RunBlocks(windowBlocks);
        timestamp += windowBlocks;
        elapsedSeconds += static_cast<double>(windowBlocks) * static_cast<double>(blockSize) / kSampleRateHz;

        float windowPeak = 0.0f;
        for (const auto& frame : rig.Output()) {
            for (float sample : frame.channels) windowPeak = std::max(windowPeak, std::fabs(sample));
        }
        const Snapshot snap = TakeSnapshot(rig, elapsedSeconds, windowPeak);
        PrintSnapshot(snap);
        std::printf("  sawNaN=%d\n\n", rig.SawNaN());
    }

    return 0;
}
