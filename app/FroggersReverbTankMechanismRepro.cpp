// FroggersReverbTankMechanismRepro.cpp -- settles WHICH of the three candidate mechanisms
// keeps the reverb tank loud for 5+s after Stop, once the envelope-ramp
// bound and the Stop-edge forced-release + effective-value overrides
// are already in place. Pass D of FroggersStopSustainRepro.cpp already
// established the symptom (20/20 randomized trials sustain past t+5s) and
// pass E already showed AllIdle()==1 by ~t+0.5s while reverb_.StateMagnitude()
// stays flat 0.64-0.72 through t+5s. This repro adds the missing
// instrumentation (temporarily wired into FroggersAppCore.hpp, gated behind
// FROGG3RS_P2B_DIAG so it is a no-op in every other build/run) to read the
// exact sample where the AllIdle-triggered clear fires (or does not), and
// the reverb's own INPUT sample immediately after any clear that does fire.
//
// NOT part of the regular suite (the *Repro.cpp convention).
//
// Candidates:
//   1. The clear never fires on the reverb.
//   2. The clear fires, but something re-excites the tank afterwards.
//   3. Something else.
//
// Method: every negative result gets a positive control with the
// number printed. PASS P2B-CONTROL reproduces pass C (drives at unity, no
// audio-rate depths -- must decay cleanly) WITH the same instrumentation on,
// to prove the RESET/SAMPLE print lines the mechanism pass depends on are
// not simply silent by construction. PASS P2B-REPRO reproduces the pass-D
// randomized-trigger condition with the same instrumentation on.

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

#include "Froggers.hpp"
#include "support/SynthRig.hpp"

namespace {

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-reverb-mech-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

// Mirrors FroggersStopSustainRepro.cpp's kDriveTargets (the same three
// in-loop saturator pre-gains that file's own bisection passes narrowed
// down).
struct DriveTarget {
    synth_froggers::FroggersBankId bank;
    std::size_t slot;
    const char* name;
};
constexpr DriveTarget kDriveTargets[] = {
    {synth_froggers::FroggersBankId::Delay, 9, "Delay slot 9 (Feedback drive)"},
    {synth_froggers::FroggersBankId::Reverb, 10, "Reverb slot 10 (Tank drive)"},
    {synth_froggers::FroggersBankId::Filter, 12, "Filter slot 12 (Comb drive)"},
};

// PASS P2B-CONTROL: pass C's own recipe (drives at unity default, no
// audio-rate depths) -- known-clean decay, proves the instrument is live.
void RunControlPass() {
    using synth_froggers::FroggersBankId;
    constexpr double kSampleRateHz = 48000.0;
    ::setenv("FROGG3RS_P2B_DIAG", "1", 1);
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("control"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();

    std::printf("################ PASS P2B-CONTROL: unity drives, no depths (must decay) ################\n");
    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;
    model.PageParameter(FroggersBankId::Delay, 1).SceneCenter(0) = 1.0f;
    model.PageParameter(FroggersBankId::Delay, 2).SceneCenter(0) = 1.0f;
    model.PageParameter(FroggersBankId::Delay, 6).SceneCenter(0) = 1.0f;
    model.PageParameter(FroggersBankId::Filter, 5).SceneCenter(0) = 1.0f;
    model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0) = 1.0f;  // Hold -> fb ~0.99998.
    for (const DriveTarget& target : kDriveTargets) {
        model.PageParameter(target.bank, target.slot).SceneCenter(0) = 0.5f;  // unity pre-gain.
    }
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
    std::printf("prestop peak=%.6g\n", static_cast<double>(prestop));

    std::fprintf(stderr, "P2B ---- STOP ISSUED (control) ----\n");
    rig.StopAt(timestamp);
    for (int second = 0; second < 6; ++second) {
        rig.ClearOutput();
        rig.RunBlocks(secondsToBlocks(1.0));
        float peak = 0.0f;
        for (const auto& frame : rig.Output()) for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
        std::printf("t+%ds peak=%.6g reverbMag=%.6g\n", second + 1, static_cast<double>(peak),
                    static_cast<double>(rig.Application().TestReverb().StateMagnitude()));
    }
}

// PASS P2B-REPRO: the reported gesture (5x Randomize All, then Stop),
// same recipe FroggersStopSustainRepro.cpp's Pass D uses, with the temp
// instrumentation switched on. Retries a small number of trials only because
// Randomize All draws are random; Pass D already measured 20/20 reproducing,
// so this is expected to hit on trial 0.
void RunReproPass() {
    using synth_froggers::FroggersBankId;
    constexpr double kSampleRateHz = 48000.0;
    ::setenv("FROGG3RS_P2B_DIAG", "1", 1);

    for (int trial = 0; trial < 10; ++trial) {
        char label[64];
        std::snprintf(label, sizeof(label), "repro_%02d", trial);
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
        if (prestop <= 0.05f) {
            std::printf("trial %d: VOID (quiet pre-stop, %.6g) -- skipping\n", trial, static_cast<double>(prestop));
            continue;
        }

        std::printf("################ PASS P2B-REPRO trial %d ################\n", trial);
        std::printf("prestop peak=%.6g  Hold(sc)=%.4f  Grit(sc)=%.4f  TkDv(sc)=%.4f\n",
                    static_cast<double>(prestop),
                    static_cast<double>(model.PageParameter(FroggersBankId::Reverb, 8).SceneCenter(0)),
                    static_cast<double>(model.PageParameter(FroggersBankId::Reverb, 11).SceneCenter(0)),
                    static_cast<double>(model.PageParameter(FroggersBankId::Reverb, 10).SceneCenter(0)));
        std::fprintf(stderr, "P2B ---- STOP ISSUED (repro trial %d) ----\n", trial);
        rig.StopAt(timestamp);

        float tail5 = 0.0f;
        for (int second = 0; second < 6; ++second) {
            rig.ClearOutput();
            rig.RunBlocks(secondsToBlocks(1.0));
            float peak = 0.0f;
            for (const auto& frame : rig.Output()) for (float sample : frame.channels) peak = std::max(peak, std::fabs(sample));
            if (second == 4) tail5 = peak;
            std::printf("t+%ds peak=%.6g reverbMag=%.6g allIdle=%d clearPending=%d\n", second + 1,
                        static_cast<double>(peak),
                        static_cast<double>(rig.Application().TestReverb().StateMagnitude()),
                        rig.Application().TestAudioAdsr().AllIdle() ? 1 : 0,
                        0 /* clearPending not test-exposed; see stderr P2B lines instead */);
        }
        std::printf("SUMMARY trial %d: prestop=%.6g t+5s=%.6g %s\n", trial, static_cast<double>(prestop),
                    static_cast<double>(tail5), (tail5 > 1.0e-4f) ? "<-- REPRODUCTION" : "(decayed)");
        if (tail5 > 1.0e-4f) {
            return;  // one clean reproduction with full instrumentation is enough.
        }
    }
}

}  // namespace

int main() {
    RunControlPass();
    std::printf("\n");
    RunReproPass();
    return 0;
}
