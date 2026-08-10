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

    // Transport RUNNING, real blocks pumped: modulation_.Step() only runs
    // when `transportRunningNow` (FroggersAppCore.hpp's ProcessBlock), so
    // this exercises T2's new runtime code -- the literal-false
    // `externalInputConnected` reaching `SetExternalAudioConnected` every
    // sample -- rather than resting on RegisterSources()'s one-time Init()
    // default, which alone would not prove the ProcessBlock wiring survived
    // T1/T2 intact.
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
