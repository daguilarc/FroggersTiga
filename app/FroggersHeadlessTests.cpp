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
    // inspect every captured sample. Since task 2.6 raised
    // Config().numAudioInputs to 1, SynthRig now allocates one real input
    // buffer/pointer and passes a non-null `block.inputs`/
    // `numInputChannels==1` into every ProcessBlock call below
    // (tests/support/SynthRig.hpp:62,72,76,454,456) -- this test doubling as
    // proof that FroggersApp tolerates an actually-present input channel
    // (previously impossible at numAudioInputs==0) without producing
    // non-finite output.
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

// Task 2.6 (tasks.md section 2; design D5 slots 13-14): Config() must
// request at least one input channel, or the modulation slate's two
// external-audio sources are permanently `.connected = false` by
// construction, no matter what is cabled -- see Froggers.hpp's Config()
// comment for the traced host contract (Runtime.hpp:237,260-261;
// SynthRig.hpp:62,72,76,454,456) that confirms 1 channel is what both
// external-audio slate slots need (they are derived from the same signal,
// design D5). The full "connected tracks actual cabling" behavior is
// exercised once the slate exists (packet 6, tasks 6.5/6.6); this test pins
// the input-channel request itself, which is task 2.6's own scope.
TEST_CASE(froggers_config_requests_exactly_one_audio_input_channel) {
    const synth::RuntimeConfig config = synth_froggers::FroggersApp::Config();
    REQUIRE_TRUE(config.numAudioInputs == 1);
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
