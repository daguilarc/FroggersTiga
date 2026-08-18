// FroggersHeadlessTests.cpp -- tasks.md 2.3: headless Init -> ProcessBlock
// test proving FroggersApp produces finite stereo output. Runs via
// synth_rig::SynthRig<FroggersApp> (External/Sheaf's
// tests/support/SynthRig.hpp), the same JUCE-free headless harness Sheaf's
// own miniapp/braid-4 system tests use (tests/miniapp_system_tests.cpp);
// wired into app/Makefile's `test` target (nice make -j2 test).
//
// This TU is part of the app core's build path (it only includes
// Froggers.hpp and Sheaf's JUCE-free rig/engine headers), so it doubles as a
// belt-and-suspenders JUCE guard alongside check_no_juce.cpp (task 2.4).

#include "Froggers.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers headless tests must not see JUCE headers -- the app core must stay JUCE-free"
#endif

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
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
// tests/miniapp_system_tests.cpp's UseScratchRuntimeDataPaths, so startup
// patch loading never observes a shared production location or another
// test's data.
synth::RuntimeDataPaths UseScratchRuntimeDataPaths(const char* testName) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-headless-tests" / testName;
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

TEST_CASE(froggers_init_process_block_produces_finite_stereo_output) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64,
        UseScratchRuntimeDataPaths("init_process_block_finite"));

    // SynthRig's constructor already runs App::Config()/Init(&context_) via
    // Engine::Initialize()+Prepare(); pump several blocks of ProcessBlock and
    // inspect every captured sample. frogg3rs-external-audio-phantom-input
    // (tasks.md T3.2) corrects this comment: it used to claim this test
    // "doubles as proof that FroggersApp tolerates an actually-present input
    // channel," true only while task 2.6 had Config().numAudioInputs at 1.
    // Now that Config() requests zero input channels (FroggersAppCore.hpp's
    // Config() comment), SynthRig allocates zero input buffers
    // (tests/support/SynthRig.hpp:62,72,76: `numInputChannels_` is
    // `config.numAudioInputs`; `RunOneBlockAt`, :454,456, sets `block.inputs
    // = nullptr` and `block.numInputChannels = 0` whenever that count is 0)
    // and passes a null `block.inputs` into every ProcessBlock call below.
    // This test's NaN/finite/stereo assertions still pass, but they no
    // longer exercise an actually-present input channel -- read, not
    // assumed: `Config()` is a static method of the App type SynthRig is
    // templated on, so there is no rig-level override that could request a
    // different channel count for just this one test without a second App
    // type, which is out of this test's scope. What this test still proves:
    // ProcessBlock produces finite stereo output end-to-end through
    // Init()/PrepareToPlay(), including with zero input channels -- exactly
    // the shape the app now always runs in.
    rig.RunBlocks(4);

    REQUIRE_TRUE(!rig.SawNaN());

    const auto& output = rig.Output();
    REQUIRE_TRUE(!output.empty());
    for (const auto& frame : output) {
        REQUIRE_TRUE(frame.channels.size() == 2);  // Config() requests stereo out.
        for (const float sample : frame.channels) {
            REQUIRE_TRUE(std::isfinite(sample));
        }
    }
}

// frogg3rs-external-audio-phantom-input (tasks.md T3.1) rewrites this test,
// not just its assertion: the old name and body argued FOR requesting one
// input channel (task 2.6's own reasoning, since corrected -- see
// FroggersAppCore.hpp's Config() comment). That argument traced
// `numAudioInputs` only as far as "the request reaches JUCE" and never
// followed it through to what JUCE does with a nonzero count: opens the
// platform's DEFAULT input device -- chosen by nobody -- which on this
// machine is always the built-in mic, present whether or not anything is
// plugged in (Runtime.hpp:237,260-261). Requesting a channel never
// delivered "the operator routed something in"; it just opened a phantom
// source Randomize then assigned depth to. Config() now requests ZERO input
// channels, so no device -- default or otherwise -- ever opens, and the two
// external-audio slate slots (13/14, design D5) are disconnected by
// construction (see FroggersAppCore.hpp's ProcessBlock comment).
TEST_CASE(froggers_config_requests_zero_audio_input_channels) {
    const synth::RuntimeConfig config = synth_froggers::FroggersApp::Config();
    REQUIRE_TRUE(config.numAudioInputs == 0);
}

// frogg3rs-external-audio-phantom-input -- proves T1+T2's mechanism
// end-to-end through the REAL Config()->Init()->ProcessBlock path
// (SynthRig), which FroggersModulationTests.cpp's own external-audio tests
// (e.g. external_audio_cells_present_and_inert_with_no_input) do not reach:
// that file drives FroggersModulationSlate::Step() directly against a bare
// Fixture, passing `externalConnected` as an explicit test-chosen bool, and
// never touches FroggersApp::Config() or ProcessBlock at all. This test
// checks three distinct things a single "not connected" assertion would
// blur together: (1) the app requests zero audio input channels; (2) both
// external-audio sources are still REGISTERED and PRESENT in the slate --
// proven by their registered `name`, not merely a `false` `connected` bit,
// because a slot silently dropped from RegisterSources() instead of
// disconnected would ALSO read a default-constructed `connected == false`
// (synth::ModulatorMetadata's own default, ParameterModulation.hpp) and
// pass a connected-only check while actually being absent; and (3) both
// report connected == false.
//
// OMNI 9.1 positive control (mandatory before trusting any "not connected"
// result): a rig that populates nothing would ALSO read every source as
// disconnected, so "false" alone proves nothing. Prints the total
// registered source count and reads a DIFFERENT, unconditionally-connected
// source (Random S&H 1, slot 0 -- registered `connected = true` in
// RegisterSources(), FroggersModulation.hpp) as `connected == true` in this
// SAME rig, SAME run, before the disconnected checks below -- demonstrating
// the metadata-reading mechanism itself is live, not uniformly false.
TEST_CASE(external_audio_sources_stay_registered_and_disconnected_with_zero_input_channels) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64,
        UseScratchRuntimeDataPaths("external_audio_registered_disconnected"));

    // (1) The root cause this whole change fixes, re-asserted here alongside
    // its slate-level consequence rather than trusted from the sibling test
    // above.
    const synth::RuntimeConfig config = synth_froggers::FroggersApp::Config();
    REQUIRE_TRUE(config.numAudioInputs == 0);

    // Transport RUNNING, real blocks pumped: this exercises the REAL
    // FroggersAppCore::PrepareToPlay()/ProcessBlock() wiring end-to-end, not
    // just RegisterSources()'s one-time Init() default in isolation. T6
    // (openspec/changes/archive/2026-08-10-frogg3rs-external-audio-phantom-input/tasks.md)
    // removed the per-sample call that used to re-derive `.connected` from
    // a per-sample `externalInputConnected` value every sample (was: T2's
    // runtime code, exercised here for that reason) -- slots 13/14 now read
    // `connected == false` purely because RegisterSources() set it once at
    // construction and nothing in the real init/process path ever touches
    // it again. Running real blocks here still matters for a DIFFERENT
    // reason post-T6: it proves externalAudioSource_/externalAudioEfSource_
    // (frozen at their NSDMI defaults, FroggersModulation.hpp) stay defined
    // and finite over actual per-sample execution (SawNaN() below), which
    // is exactly what Modulators::UpdateModValues() needs every sample
    // regardless of connectedness.
    rig.StartAt(0);
    rig.RunBlocks(8);
    REQUIRE_TRUE(!rig.SawNaN());

    synth_froggers::FroggersModulationSlate& modulation = rig.Application().Modulation();

    // Positive control FIRST (OMNI 9.1).
    std::cout << "external_audio_sources_stay_registered_and_disconnected_with_zero_input_channels: "
                 "total registered sources = "
              << synth_froggers::FroggersParameterModel::kNumModulators << "\n";
    REQUIRE_TRUE(synth_froggers::FroggersParameterModel::kNumModulators == 15);
    const synth::ModulatorMetadata& controlSource =
        modulation.Metadata(synth_froggers::kModSlotRandomSh1);
    std::cout << "  positive control -- slot " << synth_froggers::kModSlotRandomSh1 << " (\""
              << controlSource.name << "\") connected = " << std::boolalpha
              << controlSource.connected << "\n";
    REQUIRE_TRUE(controlSource.connected);  // proves the rig/metadata path can read true.

    // (2) Present: both external-audio slots were actually registered by
    // RegisterSources() -- their `name` is the exact string only that call
    // sets -- not silently dropped from the slate.
    const synth::ModulatorMetadata& externalAudio =
        modulation.Metadata(synth_froggers::kModSlotExternalAudio);
    const synth::ModulatorMetadata& externalAudioEf =
        modulation.Metadata(synth_froggers::kModSlotExternalAudioEf);
    REQUIRE_TRUE(externalAudio.name == "External Audio");
    REQUIRE_TRUE(externalAudioEf.name == "External Audio EF");

    // (3) Inert: both report connected == false -- a state the presence
    // checks above already established is not the same thing as absence.
    std::cout << "  external audio (slot " << synth_froggers::kModSlotExternalAudio
              << ") connected = " << std::boolalpha << externalAudio.connected << "\n";
    std::cout << "  external audio EF (slot " << synth_froggers::kModSlotExternalAudioEf
              << ") connected = " << std::boolalpha << externalAudioEf.connected << "\n";
    REQUIRE_TRUE(!externalAudio.connected);
    REQUIRE_TRUE(!externalAudioEf.connected);
}

// A2 (tasks.md CONSOLIDATED PUSH) -- end-to-end proof that the
// ComputeAllParameters() reseed added to FroggersAppCore::ProcessFrame()
// (FroggersAppCore.hpp) actually reaches the display through the REAL
// production path: RequestRandomizeAll() (UI/message thread) ->
// ProcessFrame() (audio thread, drained by synth::Engine once per block,
// before ProcessBlock() -- this class's own header comment) -> the reseed.
// FroggersModulationTests.cpp's own randomize tests call
// FroggersModulation.hpp's RandomizeAll()/RandomizePage() directly against a
// bare ParameterManager, which never exercises FroggersAppCore::
// ProcessFrame() at all -- this is the one place in the suite that proves
// the fix is wired into the real request-bridge path, not just correct in
// isolation.
TEST_CASE(randomize_all_request_through_process_frame_updates_the_display) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64,
        UseScratchRuntimeDataPaths("randomize_all_updates_display"));
    rig.RunBlocks(2);  // let Init()'s default patch / first ProcessFrame settle.

    rig.Application().RequestRandomizeAll();
    rig.RunBlocks(1);  // ProcessFrame() drains the request and reseeds, same block.

    // Randomize All (drill-in level 0) touches every top-level parameter
    // across all six banks (design D14) -- find any depth it materialized
    // and confirm the DISPLAY (not just the commanded value) moved.
    constexpr float kNeutral = 0.5f;
    constexpr float kTolerance = 1e-4f;
    bool foundMovedDisplay = false;
    for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount && !foundMovedDisplay; ++bankIx) {
        const auto bankId = static_cast<synth_froggers::FroggersBankId>(bankIx);
        for (std::size_t paramIx = 0; paramIx < synth_froggers::kFroggersParamsPerBank; ++paramIx) {
            synth::Parameter& parameter = rig.Application().Parameters().PageParameter(bankId, paramIx);
            for (std::size_t modIx = 0; modIx < synth_froggers::FroggersParameterModel::kNumModulators; ++modIx) {
                synth::Parameter* depth = parameter.ModulationDepthParameter(modIx);
                if (depth != nullptr && std::fabs(depth->UIDisplayCenter(0) - kNeutral) > kTolerance) {
                    foundMovedDisplay = true;
                    break;
                }
            }
            if (foundMovedDisplay) {
                break;
            }
        }
    }
    REQUIRE_TRUE(foundMovedDisplay);
}

// A4 -- LastRandomizePartial() defaults false and stays false across an
// ordinary Randomize All request with ample storage headroom (the default
// FroggersModulationSlate::kDepthParameterStorageCapacity, 1200, is well
// above the 793-915 ceiling design D14 documents), proving the accessor is
// wired and does not false-positive on a healthy randomize.
TEST_CASE(randomize_all_with_ample_capacity_reports_not_partial) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64,
        UseScratchRuntimeDataPaths("randomize_all_not_partial"));
    rig.RunBlocks(2);

    REQUIRE_TRUE(!rig.Application().LastRandomizePartial());
    rig.Application().RequestRandomizeAll();
    rig.RunBlocks(1);
    REQUIRE_TRUE(!rig.Application().LastRandomizePartial());
}

// T1.3(b) (openspec/changes/frogg3rs-stop-isolation-and-legible-labels/
// tasks.md): the pass-F minimal repro (proposal.md SS1, "Pass F" --
// FroggersStopSustainRepro.cpp's RunCurveArm, NOT part of the regular
// suite per the *Repro.cpp convention), promoted into a REAL suite test so
// the regression it guards against cannot silently come back. Curve
// (Envelope slot 12) at exactly 1.0, Grace (slot 13) active, VCO1's own
// Decay/Sustain set so the ease-in Decay's slow start lingers near peak
// (the loud-stuck regime) -- everything else default. PRE-FIX (T1.1's
// ComputeRampStep progress floor absent), measured 2026-08-17 by that
// repro: curve=1.0 held output flat at 0.939 at t+10s post-Stop,
// AllIdle()==0 at every checkpoint through t+5s -- the voice never reached
// Idle, so FroggersAppCore's delay/reverb clear (gated on AllIdle()) never
// fired, and the instrument never actually stopped after Stop. This test
// pins the fix: AllIdle() within 2.0s of Stop, and output peak below 1e-4
// within 2.5s of Stop.
TEST_CASE(stop_silences_curve_one_grace_active_voice_within_bound_pass_f_repro_as_suite_test) {
    synth_rig::SynthRig<synth_froggers::FroggersApp> rig(
        /*patchPumpBudgetBlocks=*/64,
        UseScratchRuntimeDataPaths("stop_silences_curve_one_grace_active"));
    synth_froggers::FroggersParameterModel& model = rig.Application().Parameters();
    using synth_froggers::FroggersBankId;

    model.PageParameter(FroggersBankId::Audio, 0).SceneCenter(0) = 0.5f;   // VCO1 pitch.
    model.PageParameter(FroggersBankId::Drive, 0).SceneCenter(0) = 0.8f;   // Drive gain.
    // Same recipe as pass F's RunCurveArm (FroggersStopSustainRepro.cpp):
    // mid Decay + audible Sustain on VCO1 (ease-in Decay's slow start keeps
    // level lingering near peak -- default fast Decay or silent Sustain
    // would fail to reproduce the bug for the WRONG reason), Curve at
    // exactly 1.0 (the value that made ComputeRampStep's per-sample
    // progress unbounded pre-T1.1), Grace active (a pending release defers
    // through Attack/Decay instead of forcing Release immediately -- the
    // condition Pass F needed to expose the stuck-mid-Decay regime).
    // Attack/Release/VCO2/VCO3 stay at their registered defaults, exactly
    // as the repro leaves them.
    model.PageParameter(FroggersBankId::Envelope, 1).SceneCenter(0) = 0.5f;   // Decay VCO1: mid.
    model.PageParameter(FroggersBankId::Envelope, 2).SceneCenter(0) = 0.7f;   // Sustain VCO1: audible.
    model.PageParameter(FroggersBankId::Envelope, 12).SceneCenter(0) = 1.0f;  // Curve: exactly 1.0.
    model.PageParameter(FroggersBankId::Envelope, 13).SceneCenter(0) = 0.5f;  // Grace: active.
    rig.Application().TestParameterManager().ComputeAllParameters();

    rig.StartAt(0);
    std::uint64_t timestamp = 0;
    const std::size_t blockSize = 256;
    constexpr double kSampleRateHz = 48000.0;
    const auto secondsToBlocks = [&](double seconds) -> std::size_t {
        return static_cast<std::size_t>(std::ceil((seconds * kSampleRateHz) / static_cast<double>(blockSize)));
    };

    rig.RunBlocks(secondsToBlocks(2.0));
    timestamp += secondsToBlocks(2.0);

    rig.ClearOutput();
    rig.RunBlocks(secondsToBlocks(0.1));
    timestamp += secondsToBlocks(0.1);
    float prestopPeak = 0.0f;
    for (const auto& frame : rig.Output()) {
        for (float sample : frame.channels) prestopPeak = std::max(prestopPeak, std::fabs(sample));
    }
    // Liveness gate (OMNI SS9.1's positive control): the arm must actually
    // be audible before Stop, or a silent-by-accident run would make the
    // post-Stop assertions below vacuous.
    REQUIRE_TRUE(prestopPeak > 0.05f);

    rig.StopAt(timestamp);

    // AllIdle() within 2.0s of Stop -- the ADSR side of the fix: every
    // voice actually reaches Stage::Idle (was: stuck mid-Decay forever,
    // AllIdle()==0 through t+5s pre-fix).
    rig.RunBlocks(secondsToBlocks(2.0));
    REQUIRE_TRUE(rig.Application().TestAudioAdsr().AllIdle());

    // Silence within 2.5s of Stop -- the audible consequence: once AllIdle()
    // fires, FroggersAppCore's delay/reverb clear can run and the wet tail
    // finishes decaying (was: output held flat at 0.939 at t+10s pre-fix).
    rig.ClearOutput();
    rig.RunBlocks(secondsToBlocks(0.5));
    float postStopPeak = 0.0f;
    for (const auto& frame : rig.Output()) {
        for (float sample : frame.channels) postStopPeak = std::max(postStopPeak, std::fabs(sample));
    }
    REQUIRE_TRUE(!rig.SawNaN());
    REQUIRE_TRUE(postStopPeak < 1.0e-4f);
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
