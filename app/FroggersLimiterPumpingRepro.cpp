// FroggersLimiterPumpingRepro.cpp -- F2.0a/F2.0b (openspec/changes/
// archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md): headless measurement
// harness for "measure the RIGHT quantity" (F2.0a) and "measure the
// OPERATOR'S ACTUAL condition" (F2.0b). NOT part of the regular suite (not
// wired into app/Makefile's `test` target, per F0.2's *Repro.cpp
// convention) -- a one-off measurement binary, built the same way
// FroggersStopFlushRepro.cpp documents:
//
//   cd /path/to/FroggersTiga && nice clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersLimiterPumpingRepro.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_limiter_pumping_repro
//
// Purpose: measure, not fix. Every existing number for the master limiter's
// engagement (F2.0's table) is `minEnvelopeSeen`, a minimum, which cannot
// distinguish "dipped once for one block" from "rode flat at 0.9858 for all
// 256 blocks". Pumping is audible VARIATION in gain, not a low minimum --
// this harness reports duty cycle (fraction of blocks with envelope <
// 0.999), min-to-max range, mean, and a direction-change count of the
// envelope trace (a rough "does it oscillate or sit flat" proxy: 0 means
// monotonic/flat, a large count means it is wiggling), on two patches:
//
//   Scenario A (F2.0a) -- the existing hostile patch, static knobs, no
//   modulation. Bit-identical to FroggersAudioRoutingTests.cpp's
//   master_limiter_stays_at_unity_across_hostile_patch (comb feedback +
//   comb/peak + reverb Hold + reverb wet + Drive all at max, PLUS Filter
//   Crispy at max), measured over the same 256-block window that test uses.
//
//   Scenario B (F2.0b) -- the operator's actual condition: RequestRandomizeAll()
//   through the real ProcessFrame() drain (same method the Randomize All
//   button calls, per FroggersRandomizeAllRepro.cpp's established
//   convention), THEN Filter Crispy forced to max on top, then the same
//   256-block measurement.
//
// Read-only: no production file is touched by this harness.

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers limiter pumping repro must not see JUCE headers"
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

namespace {

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-limiter-pumping-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

// Mirrors FroggersAudioRoutingTests.cpp's ApplyPatchNow (B7.5.0 settling
// rule): a SceneCenter write is only ~81% applied after one block
// (Parameter::ProcessSamplePhase1's periodic smoothed Compute, alpha 0.0994
// every 16 samples); ComputeAllParameters() converges it exactly instead.
void ApplyPatchNow(Rig& rig) {
    rig.Application().TestParameterManager().ComputeAllParameters();
}

// Mirrors FroggersAudioRoutingTests.cpp:111's PeakAbs -- "assert some output
// sample exceeds a small epsilon in magnitude", the liveness check.
float PeakAbs(const std::vector<Rig::OutputFrame>& frames) {
    float peak = 0.0f;
    for (const auto& frame : frames) {
        for (const float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
    }
    return peak;
}

struct EnvelopeStats {
    std::size_t blocks = 0;
    std::size_t belowUnityCount = 0;   // blocks with envelope < 0.999
    float minEnvelope = 1.0f;
    float maxEnvelope = 0.0f;
    double meanEnvelope = 0.0;
    int directionChanges = 0;          // local extrema in the trace; 0 == monotonic/flat
};

// F2.0a's own instruction: report duty cycle, min-to-max range, and mean --
// NOT just minEnvelopeSeen, which cannot distinguish "dipped once" from
// "rode flat". directionChanges is an added rough oscillation-vs-flat
// signal: counts every time the block-to-block envelope delta changes sign,
// i.e. every local peak/valley in the trace.
template <typename Limiter>
EnvelopeStats MeasureEnvelope(Rig& rig, Limiter& limiter, std::size_t blocks) {
    EnvelopeStats stats;
    stats.blocks = blocks;
    double sum = 0.0;
    float prevEnv = 0.0f;
    float prevDelta = 0.0f;
    bool havePrevEnv = false;
    bool havePrevDelta = false;
    for (std::size_t block = 0; block < blocks; ++block) {
        rig.RunBlocks(1);
        const float env = limiter.envelope;
        sum += static_cast<double>(env);
        stats.minEnvelope = std::min(stats.minEnvelope, env);
        stats.maxEnvelope = std::max(stats.maxEnvelope, env);
        if (env < 0.999f) ++stats.belowUnityCount;
        if (havePrevEnv) {
            const float delta = env - prevEnv;
            if (delta != 0.0f) {
                if (havePrevDelta && prevDelta != 0.0f && (delta > 0.0f) != (prevDelta > 0.0f)) {
                    ++stats.directionChanges;
                }
                prevDelta = delta;
                havePrevDelta = true;
            }
        }
        prevEnv = env;
        havePrevEnv = true;
    }
    stats.meanEnvelope = sum / static_cast<double>(blocks);
    return stats;
}

void PrintStats(const EnvelopeStats& s) {
    const double dutyCycle = static_cast<double>(s.belowUnityCount) / static_cast<double>(s.blocks);
    std::printf("  blocks                 %zu\n", s.blocks);
    std::printf("  duty cycle (<0.999)    %.6f  (%zu/%zu blocks)\n", dutyCycle, s.belowUnityCount, s.blocks);
    std::printf("  min envelope           %.6f\n", static_cast<double>(s.minEnvelope));
    std::printf("  max envelope           %.6f\n", static_cast<double>(s.maxEnvelope));
    std::printf("  range (max-min)        %.6f\n", static_cast<double>(s.maxEnvelope - s.minEnvelope));
    std::printf("  mean                   %.6f\n", s.meanEnvelope);
    std::printf("  direction changes      %d  (0 == monotonic/flat; large == oscillating)\n", s.directionChanges);
}

}  // namespace

int main() {
    constexpr std::size_t kMeasureBlocks = 256;  // matches master_limiter_stays_at_unity_across_hostile_patch's window.

    // ---------------------------------------------------------------
    // Scenario A (F2.0a): the existing hostile patch, static knobs.
    // Bit-identical setup to FroggersAudioRoutingTests.cpp's
    // master_limiter_stays_at_unity_across_hostile_patch.
    // ---------------------------------------------------------------
    {
        Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("f2_0a_static"));
        synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

        model.PageParameter(synth_froggers::FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +0.95
        model.PageParameter(synth_froggers::FroggersBankId::Filter, 7).SceneCenter(0) = 1.0f;  // Comb/Peak -> all comb
        model.PageParameter(synth_froggers::FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> ceiling
        model.PageParameter(synth_froggers::FroggersBankId::Reverb, 0).SceneCenter(0) = 1.0f;  // fully wet
        model.PageParameter(synth_froggers::FroggersBankId::Drive, 0).SceneCenter(0) = 1.0f;   // maximum Drive
        model.Crispy(synth_froggers::FroggersBankId::Filter).SceneCenter(0) = 1.0f;
        ApplyPatchNow(rig);

        rig.StartAt(0);
        auto& limiter = rig.Application().TestOutputLimiter();
        const EnvelopeStats stats = MeasureEnvelope(rig, limiter, kMeasureBlocks);

        std::printf("=== M2 / F2.0a: existing hostile patch (static, no modulation) ===\n");
        std::printf("PeakAbs (measurement window): %.6f   sawNaN=%d\n",
                     static_cast<double>(PeakAbs(rig.Output())), rig.SawNaN());
        PrintStats(stats);
        std::printf("\n");
    }

    // ---------------------------------------------------------------
    // Scenario B (F2.0b): the operator's actual condition.
    // RequestRandomizeAll() through the real ProcessFrame() drain, THEN
    // Filter Crispy forced to max on top.
    // ---------------------------------------------------------------
    {
        Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("f2_0b_randomize"));
        synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

        rig.StartAt(0);
        // ~1s of steady-state default-patch audio before touching Randomize
        // All, matching FroggersRandomizeAllRepro.cpp's own convention (not
        // load-bearing for the measurement itself, just realistic context).
        constexpr double kSampleRateHz = 48000.0;
        constexpr std::size_t kBlockSize = 256;
        const std::size_t oneSecondBlocks =
            static_cast<std::size_t>(std::ceil(kSampleRateHz / static_cast<double>(kBlockSize)));
        rig.RunBlocks(oneSecondBlocks);

        // Fire Randomize All the real way -- FroggersApp::RequestRandomizeAll()
        // is the exact method FroggersUiSurface::HandleAction calls for the
        // button. ProcessFrame() (FroggersAppCore.hpp:427) is invoked once
        // per block, after message drains and BEFORE that block's own audio,
        // and itself calls ComputeAllParameters() once RandomizeAll has run
        // (:534) -- so a single RunBlocks(1) both drains the pending request
        // and converges every write it made; no ramp, no extra ApplyPatchNow
        // needed for this part.
        rig.Application().RequestRandomizeAll();
        rig.RunBlocks(1);
        std::printf("Post-RequestRandomizeAll: LastRandomizePartial=%d\n",
                     rig.Application().LastRandomizePartial());

        // THEN Filter Crispy to max on top, exactly as F2.0b specifies. This
        // is a plain SceneCenter write (not through RandomizeAll's own
        // converge-immediately path), so ApplyPatchNow() as in Scenario A.
        model.Crispy(synth_froggers::FroggersBankId::Filter).SceneCenter(0) = 1.0f;
        ApplyPatchNow(rig);

        // Measure exactly as in Scenario A/M2: 256 blocks, envelope sampled
        // once per block. ClearOutput() first so PeakAbs is scoped to this
        // window, not the 1s pre-roll.
        rig.ClearOutput();
        auto& limiter = rig.Application().TestOutputLimiter();
        const EnvelopeStats stats = MeasureEnvelope(rig, limiter, kMeasureBlocks);

        std::printf("\n=== M3 / F2.0b: post-Randomize-All + Filter Crispy max ===\n");
        std::printf("PeakAbs (measurement window): %.6f   sawNaN=%d\n",
                     static_cast<double>(PeakAbs(rig.Output())), rig.SawNaN());
        PrintStats(stats);
    }

    return 0;
}
