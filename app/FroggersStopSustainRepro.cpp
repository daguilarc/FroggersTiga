// FroggersStopSustainRepro.cpp -- T4.2 (openspec/changes/
// frogg3rs-post-expansion-consolidation): the runtime capture §7e said was
// the ONE thing reading could not settle, run 2026-08-17 after the operator
// re-reported "the stop button still does not stop sound when things are
// being modulated at audio rate" and ruled the effect must be ISOLATED to
// the Freeze button (superseding this change's own T4.1 closure).
//
// NOT part of the regular suite (F0.2's *Repro.cpp convention) -- built by
// hand exactly like FroggersStopFlushRepro.cpp documents.
//
// WHY THE 2026-08-07 HARNESS CANNOT ANSWER THIS: its F3.2c/F3.2d passes
// concluded audio-rate modulation was VOID because S1a.2 gated modulation on
// the transport, so knobs cannot MOVE post-Stop. §7's mechanism needs no
// post-Stop movement: modulation_.Step() stops being called and the slate
// FREEZES AT LAST VALUES (FroggersAppCore.hpp:889-891 region), so a drive
// knob whose modulated value was high at the Stop edge STAYS high. And the
// three parameters §7b implicates -- Delay slot 9 FbDr, Reverb slot 10
// TkDv, Filter slot 12 CDrv, each a 0.25-4.0 pre-gain on an in-loop
// saturator argument -- did not exist when that harness was written
// (they shipped 2026-08-13, frogg3rs-bank-expansion). No historical pass
// covers the reported condition.
//
// THREE PASSES, one variable each (OMNI §9.1 -- the control proves the
// instrument could have shown a decaying tail):
//   A DRIVES-HIGH + AUDIO-RATE DEPTHS -- the operator's reported condition:
//     drive bases at 1.0 AND kModSlotVco1Audio depths at full positive on
//     all three, so the frozen slate holds them high after Stop.
//   B DRIVES-HIGH, NO DEPTHS -- same bases, no modulation: distinguishes
//     "the frozen modulation holds it up" from "high bases alone do".
//   C CONTROL -- drives at their 0.5 defaults (== unity pre-gain), no
//     depths: must decay to silence; its envelope is the positive control.
//
// Measurement: pre-Stop 100ms peak (liveness gate), then per-second output
// peak for 60s after Stop. The question the numbers answer: does the tail
// decay inside the ~1.05s Grace+fade window before ForEachStatefulUnit
// (Reset) lands (bounded), or persist past it (§7e's re-seed hypothesis)?

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <string>

#include "Froggers.hpp"
#include "support/SynthRig.hpp"

namespace {

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-stop-sustain-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

struct DriveTarget {
    synth_froggers::FroggersBankId bank;
    std::size_t slot;
    const char* name;
};

// §7b's table, verbatim: the three in-loop saturator pre-gains.
constexpr DriveTarget kDriveTargets[] = {
    {synth_froggers::FroggersBankId::Delay, 9, "Delay slot 9 (Feedback drive)"},
    {synth_froggers::FroggersBankId::Reverb, 10, "Reverb slot 10 (Tank drive)"},
    {synth_froggers::FroggersBankId::Filter, 12, "Filter slot 12 (Comb drive)"},
};

struct PassResult {
    bool setupOk = true;
    float prestopPeak = 0.0f;
    float secondPeaks[60] = {};
    float finalPeak = 0.0f;
};

PassResult RunPass(bool drivesHigh, bool audioRateDepths, const char* scratchLabel, const char* passLabel) {
    PassResult result;
    constexpr double kSampleRateHz = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(scratchLabel));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    std::printf("################ %s ################\n", passLabel);

    using synth_froggers::FroggersBankId;
    // Excitation (same recipe the sibling Stop tests use) plus the loop
    // gains the drive pre-gains multiply: Delay send/feedback/mix high,
    // comb feedback high, reverb hold high.
    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;   // VCO1 pitch.
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;   // Drive gain.
    model.PageParameter(FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;   // Send.
    model.PageParameter(FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;   // Feedback -> fbk 0.98.
    model.PageParameter(FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;   // Wet mix.
    model.PageParameter(FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;  // Comb feedback -> +-0.95.
    model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> fb ~0.99998.

    for (const DriveTarget& target : kDriveTargets) {
        synth::Parameter& param = model.PageParameter(target.bank, target.slot);
        param.SceneCenter(0) = drivesHigh ? 1.0f : 0.5f;  // 1.0 -> pre-gain 4.0; 0.5 -> unity.
        if (audioRateDepths) {
            synth::Parameter* depth = param.EnsureModulationDepth(synth_froggers::kModSlotVco1Audio);
            if (depth == nullptr) {
                std::printf("%s: EnsureModulationDepth null for %s -- pass VOID.\n", passLabel, target.name);
                result.setupOk = false;
                return result;
            }
            depth->SceneCenter(0) = 1.0f;  // bipolar: 0.5 == off, 1.0 == full positive.
            std::printf("%s: audio-rate depth on %s\n", passLabel, target.name);
        }
    }

    rig.Application().TestParameterManager().ComputeAllParameters();

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;
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
    std::printf("Pre-Stop peak (100ms): %.6g  sawNaN=%d\n", static_cast<double>(result.prestopPeak), rig.SawNaN());

    rig.StopAt(timestamp);
    std::printf("Stop issued. Per-second post-Stop output peak, 60s:\n");
    const std::size_t oneSecond = secondsToBlocks(1.0);
    for (int second = 0; second < 60; ++second) {
        rig.ClearOutput();
        rig.RunBlocks(oneSecond);
        float peak = 0.0f;
        for (const auto& frame : rig.Output()) {
            for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
        }
        result.secondPeaks[second] = peak;
        if (second < 6 || (second % 10) == 9) {
            std::printf("  t+%2ds: %.6g\n", second + 1, static_cast<double>(peak));
        }
    }
    result.finalPeak = result.secondPeaks[59];
    std::printf("t+60s peak: %.6g  sawNaN=%d\n\n", static_cast<double>(result.finalPeak), rig.SawNaN());
    return result;
}

}  // namespace

// ---------------------------------------------------------------------
// PASS D (added same day): the ACTUAL reported trigger, not a hand-built
// approximation of it. Passes A-C refuted the hand-built condition (all
// silent by t+2s -- the Grace+fade window then the AllIdle clear), yet the
// operator observes sustain in the live app. The delta between rig and live
// is unidentified, so this pass reproduces the operator's own gesture:
// repeated Randomize All presses (the real trigger per §7c), then Stop,
// across many independent trials. Any trial whose post-Stop tail survives
// t+5s is a REPRODUCTION, and its drive-parameter snapshot is printed for
// bisection. Zero reproductions is itself a finding: the mechanism then
// lives in something the rig does not simulate (device callback, sample
// rate, MIDI clock, UI-thread interleaving), recorded as such rather than
// guessed at.
// ---------------------------------------------------------------------

namespace {

enum class Intervention { None, ZeroDepthsAtStop, CalmKnobsAtStop, BothAtStop };

const char* InterventionName(Intervention i) {
    switch (i) {
        case Intervention::None: return "none";
        case Intervention::ZeroDepthsAtStop: return "zero-all-depths-at-stop";
        case Intervention::CalmKnobsAtStop: return "calm-drive+freeze-knobs-at-stop";
        case Intervention::BothAtStop: return "both";
    }
    return "?";
}

void PrintSevenSnapshot(synth_froggers::FroggersParameterModel& model, const char* when) {
    using synth_froggers::FroggersBankId;
    const auto sc = [&](FroggersBankId b, std::size_t slot) {
        return static_cast<double>(model.PageParameter(b, slot).SceneCenter(0));
    };
    const auto ck = [&](FroggersBankId b, std::size_t slot) {
        return static_cast<double>(model.PageParameter(b, slot).CachedKnobValue(0));
    };
    std::printf("  [%s] FbDr sc=%.4f ck=%.4f | TkDv sc=%.4f ck=%.4f | CDrv sc=%.4f ck=%.4f | Frze sc=%.4f ck=%.4f\n",
                when,
                sc(FroggersBankId::Delay, 9), ck(FroggersBankId::Delay, 9),
                sc(FroggersBankId::Reverb, 10), ck(FroggersBankId::Reverb, 10),
                sc(FroggersBankId::Filter, 12), ck(FroggersBankId::Filter, 12),
                sc(FroggersBankId::Delay, 4), ck(FroggersBankId::Delay, 4));
}

int RunRandomizedTrials(int trials, Intervention intervention) {
    using synth_froggers::FroggersBankId;
    constexpr double kSampleRateHz = 48000.0;
    int reproductions = 0;
    for (int trial = 0; trial < trials; ++trial) {
        char label[64];
        std::snprintf(label, sizeof(label), "d_%d_%02d", static_cast<int>(intervention), trial);
        Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(label));
        synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
        model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
        model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
        rig.Application().TestParameterManager().ComputeAllParameters();

        rig.StartAt(0);
        std::uint64_t timestamp = 0;
        const std::size_t blockSize = 256;
        const auto secondsToBlocks = [&](double seconds) -> std::size_t {
            return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
        };

        // The operator's gesture: several Randomize All presses with audio
        // running between them (each press re-freezes nothing -- transport
        // is RUNNING -- but spaces the draws the way live use does).
        for (int press = 0; press < 5; ++press) {
            rig.Application().RequestRandomizeAll();
            rig.RunBlocks(secondsToBlocks(0.4));
            timestamp += secondsToBlocks(0.4);
        }

        rig.ClearOutput();
        rig.RunBlocks(secondsToBlocks(0.1));
        timestamp += secondsToBlocks(0.1);
        float prestop = 0.0f;
        for (const auto& frame : rig.Output())
            for (float sample : frame.channels) prestop = std::max(prestop, std::fabs(sample));

        PrintSevenSnapshot(model, "pre-stop");
        rig.StopAt(timestamp);
        if (intervention == Intervention::ZeroDepthsAtStop || intervention == Intervention::BothAtStop) {
            // Same reach ResetBankValues uses (BankAt + VisibleParameter, which
            // covers Crispy at 14) -- PageParameter(_, 14) aborts, and the
            // first build of this arm died on exactly that (SIGABRT, exit 134).
            for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount; ++bankIx) {
                synth::Bank& bank = model.BankAt(static_cast<synth_froggers::FroggersBankId>(bankIx));
                for (synth::PhysicalEncoderId e = 0; e <= synth_froggers::kFroggersCrispySlot; ++e) {
                    synth::Parameter* param = bank.VisibleParameter(e);
                    if (param != nullptr) {
                        synth_froggers::detail::ZeroExistingModulationDepths(*param);
                    }
                }
            }
        }
        if (intervention == Intervention::CalmKnobsAtStop || intervention == Intervention::BothAtStop) {
            using synth_froggers::FroggersBankId;
            for (const DriveTarget& t : kDriveTargets) {
                model.PageParameter(t.bank, t.slot).SceneCenter(0) = 0.5f;  // unity pre-gain.
            }
            model.PageParameter(FroggersBankId::Delay, 4).SceneCenter(0) = 0.0f;  // Freeze off.
        }
        float tail5 = 0.0f, tail8 = 0.0f;
        for (int second = 0; second < 8; ++second) {
            rig.ClearOutput();
            rig.RunBlocks(secondsToBlocks(1.0));
            float peak = 0.0f;
            for (const auto& frame : rig.Output())
                for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
            if (second == 4) tail5 = peak;
            if (second == 7) tail8 = peak;
        }
        PrintSevenSnapshot(model, "t+8s   ");

        const bool live = prestop > 0.05f;
        const bool sustained = live && tail5 > 1.0e-4f;
        std::printf("trial %2d: prestop=%.6g  t+5s=%.6g  t+8s=%.6g  %s%s\n", trial,
                    static_cast<double>(prestop), static_cast<double>(tail5), static_cast<double>(tail8),
                    live ? "" : "[VOID quiet] ", sustained ? "<-- REPRODUCTION" : "");
        if (sustained) {
            ++reproductions;
            std::printf("  drive-relevant values: FbDr=%.4f TkDv=%.4f CDrv=%.4f Frze=%.4f DlyFb=%.4f CmbFb=%.4f Hold=%.4f\n",
                        static_cast<double>(model.PageParameter(FroggersBankId::Delay, 9).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Reverb, 10).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Filter, 12).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Delay, 4).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Delay, 2).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Filter, 5).SceneCenter(0)),
                        static_cast<double>(model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0)));
        }
    }
    std::printf("PASS D[%s]: %d/%d trials reproduced post-Stop sustain past t+5s\n",
                InterventionName(intervention), reproductions, trials);
    return reproductions;
}

}  // namespace

// ---------------------------------------------------------------------
// PASS E: per-unit energy timeline for ONE randomized trial. D2-D4 falsified
// the drive/freeze/depth hypotheses (5/5 sustain in every arm), so this
// locates the generator directly: which stateful unit still holds energy at
// t+5s, whether the voices actually reached Idle, and whether the AllIdle
// clear fired (all unit magnitudes snapping to ~0 then REGROWING would mean
// re-seeding; never snapping means the clear never fired).
// ---------------------------------------------------------------------

namespace {

void PrintUnitRow(const char* label, float value) {
    std::printf("  %-24s %.6g\n", label, static_cast<double>(value));
}

void PrintUnitTimelineRow(Rig& rig, double tSeconds, float windowPeak) {
    synth_froggers::FroggersApp& app = rig.Application();
    std::printf("--- t+%.1fs post-Stop  (AllIdle=%d, output window peak %.6g) ---\n",
                tSeconds, app.TestAudioAdsr().AllIdle() ? 1 : 0, static_cast<double>(windowPeak));
    PrintUnitRow("audioVco1_", app.TestAudioVco(0).StateMagnitude());
    PrintUnitRow("audioVco2_", app.TestAudioVco(1).StateMagnitude());
    PrintUnitRow("audioVco3_", app.TestAudioVco(2).StateMagnitude());
    PrintUnitRow("driveBlendPhase_", app.TestDriveBlendPhase().StateMagnitude());
    PrintUnitRow("drive.oversampler", app.TestDriveOversampler().StateMagnitude());
    PrintUnitRow("filterChain.peak", app.TestFilterPeak().StateMagnitude());
    PrintUnitRow("filterChain.scoopNotch", app.TestFilterScoopNotch().StateMagnitude());
    PrintUnitRow("filterChain.comb", app.TestFilterComb().StateMagnitude());
    PrintUnitRow("delay_", app.TestDelay().StateMagnitude());
    PrintUnitRow("reverb_", app.TestReverb().StateMagnitude());
    PrintUnitRow("outputLimiter_.envelope", app.TestOutputLimiter().envelope);
}

void RunEnergyTimeline() {
    using synth_froggers::FroggersBankId;
    constexpr double kSampleRateHz = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("e_timeline"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
    rig.Application().TestParameterManager().ComputeAllParameters();

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
    };
    for (int press = 0; press < 5; ++press) {
        rig.Application().RequestRandomizeAll();
        rig.RunBlocks(secondsToBlocks(0.4));
        timestamp += secondsToBlocks(0.4);
    }
    rig.StopAt(timestamp);

    const double checkpoints[] = {0.5, 1.0, 1.5, 2.0, 3.0, 5.0};
    double elapsed = 0.0;
    for (double t : checkpoints) {
        const double advance = t - elapsed;
        rig.ClearOutput();
        rig.RunBlocks(secondsToBlocks(advance));
        elapsed = t;
        float peak = 0.0f;
        for (const auto& frame : rig.Output())
            for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
        PrintUnitTimelineRow(rig, t, peak);
    }
}

}  // namespace

// ---------------------------------------------------------------------
// PASS F: minimal-repro confirmation of the traced root cause. Hand patch,
// NOTHING randomized, exactly two suspects raised: Envelope Curve (slot 12)
// and Grace (slot 13). If curve~1 + grace>0 alone reproduce the post-Stop
// sustain, the root cause is CONFIRMED as ComputeRampStep's asymptotic
// regime (step -> step^2/remaining at curve==1, durations ~ 1/(1-curve))
// composed with the pending-release ladder leaving Attack/Decay untouched
// while grace is active. Arms: curve 1.0 / 0.999 / 0.5 / 0.0, grace 0.5
// in the first three and 0.5 in the last (grace alone must NOT reproduce).
// ---------------------------------------------------------------------

namespace {

void RunCurveArm(float curveKnob, float graceKnob, const char* label) {
    using synth_froggers::FroggersBankId;
    constexpr double kSampleRateHz = 48000.0;
    char scratch[64];
    std::snprintf(scratch, sizeof(scratch), "f_curve_%s", label);
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(scratch));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;   // VCO1 pitch.
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;   // Drive gain.
    // The loud-stuck regime needs the voice caught mid-DECAY near its peak:
    // ease-in decay's slow START keeps the level lingering near 1.0. Default
    // Decay (fast) would complete and default Sustain (0) would make a stuck
    // voice silent -- either would fail to reproduce for the WRONG reason.
    model.PageParameter(FroggersBankId::Envelope, 1).SceneCenter(0) = 0.5f;   // Decay VCO1: mid.
    model.PageParameter(FroggersBankId::Envelope, 2).SceneCenter(0) = 0.7f;   // Sustain VCO1: audible.
    model.PageParameter(FroggersBankId::Envelope, 12).SceneCenter(0) = curveKnob;  // Curve.
    model.PageParameter(FroggersBankId::Envelope, 13).SceneCenter(0) = graceKnob;  // Grace.
    rig.Application().TestParameterManager().ComputeAllParameters();

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
    };
    rig.RunBlocks(secondsToBlocks(2.0));
    timestamp += secondsToBlocks(2.0);
    rig.ClearOutput();
    rig.RunBlocks(secondsToBlocks(0.1));
    timestamp += secondsToBlocks(0.1);
    float prestop = 0.0f;
    for (const auto& frame : rig.Output())
        for (float sample : frame.channels) prestop = std::max(prestop, std::fabs(sample));

    rig.StopAt(timestamp);
    float tail5 = 0.0f, tail10 = 0.0f;
    for (int second = 0; second < 10; ++second) {
        rig.ClearOutput();
        rig.RunBlocks(secondsToBlocks(1.0));
        float peak = 0.0f;
        for (const auto& frame : rig.Output())
            for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
        if (second == 4) tail5 = peak;
        if (second == 9) tail10 = peak;
    }
    std::printf("curve=%.3f grace=%.2f: prestop=%.6g  t+5s=%.6g  t+10s=%.6g  AllIdle@t+10=%d  %s\n",
                static_cast<double>(curveKnob), static_cast<double>(graceKnob),
                static_cast<double>(prestop), static_cast<double>(tail5), static_cast<double>(tail10),
                rig.Application().TestAudioAdsr().AllIdle() ? 1 : 0,
                (prestop > 0.05f && tail5 > 1.0e-4f) ? "<-- REPRODUCTION" : "");
}

}  // namespace

int main() {
    const PassResult a = RunPass(true, true, "a_mod", "PASS A: drives HIGH + audio-rate depths (reported condition)");
    const PassResult b = RunPass(true, false, "b_base", "PASS B: drives HIGH, no depths");
    const PassResult c = RunPass(false, false, "c_ctrl", "PASS C: CONTROL, drives unity, no depths");

    std::printf("################ SUMMARY ################\n");
    std::printf("pre-Stop peaks:  A=%.6g  B=%.6g  C=%.6g  (all must be loud or the run is VOID)\n",
                static_cast<double>(a.prestopPeak), static_cast<double>(b.prestopPeak),
                static_cast<double>(c.prestopPeak));
    std::printf("t+5s peaks:      A=%.6g  B=%.6g  C=%.6g\n",
                static_cast<double>(a.secondPeaks[4]), static_cast<double>(b.secondPeaks[4]),
                static_cast<double>(c.secondPeaks[4]));
    std::printf("t+60s peaks:     A=%.6g  B=%.6g  C=%.6g\n",
                static_cast<double>(a.finalPeak), static_cast<double>(b.finalPeak),
                static_cast<double>(c.finalPeak));
    const bool voidRun = !(a.setupOk && b.setupOk && c.setupOk) ||
                         a.prestopPeak <= 0.1f || b.prestopPeak <= 0.1f || c.prestopPeak <= 0.1f;
    if (voidRun) {
        std::printf("RUN VOID -- a pass failed setup or was not loud pre-Stop; no conclusion may be drawn.\n");
        return 1;
    }
    std::printf("\n################ PASS D: randomized-trigger sweep ################\n");
    // T1.4 (frogg3rs-stop-isolation-and-legible-labels tasks.md, ORDERING
    // audit note): re-run AFTER T2.1 lands, at the original 20-trial count
    // the investigation that opened this whole change used (proposal.md
    // SS1: "20/20 trials sustain past t+5s") -- bumped from 5 here
    // (unchanged D2-D4 bisection passes below still run their original 5,
    // they are diagnostic, not this task's own re-run).
    RunRandomizedTrials(20, Intervention::None);
    std::printf("\n################ PASS D2: bisection -- zero all depths at Stop ################\n");
    RunRandomizedTrials(5, Intervention::ZeroDepthsAtStop);
    std::printf("\n################ PASS D3: bisection -- calm drive+freeze knobs at Stop ################\n");
    RunRandomizedTrials(5, Intervention::CalmKnobsAtStop);
    std::printf("\n################ PASS D4: bisection -- both ################\n");
    RunRandomizedTrials(5, Intervention::BothAtStop);
    std::printf("\n################ PASS E: per-unit energy timeline ################\n");
    RunEnergyTimeline();
    std::printf("\n################ PASS F: minimal repro -- Curve x Grace only ################\n");
    RunCurveArm(1.0f, 0.5f, "c100_g50");
    RunCurveArm(0.999f, 0.5f, "c999_g50");
    RunCurveArm(0.5f, 0.5f, "c50_g50");
    RunCurveArm(0.0f, 0.5f, "c0_g50");
    return 0;
}
