// FroggersVisualizerTests.cpp -- tasks.md section "7. VCO scopes" (task
// 7.4) and section "9. Bump/Comb transfer-function visualizers" (task 9.4,
// the "displayed magnitude is bounded rather than drawn out of frame"
// half; the "response matches closed-form at sample points" half lives in
// FroggersDspParityTests.cpp alongside the rest of the ported DSP's own
// parity suite).
//
// The VCO scope checks need a real, constructed FroggersApp (the scope
// wiring happens in its own constructor, and "connected" only becomes true
// after at least one ProcessBlock() call populates the UIState) --
// synth_rig::SynthRig, same headless harness as FroggersAudioRoutingTests.cpp.
// The transfer-function bounded-plot check needs no rig at all: it
// constructs a dsp::Comb::UIState directly and drives
// synth_froggers::TransferFunctionVisualizer over it.

#include "Froggers.hpp"
#include "FroggersTransferFunctionVisualizer.hpp"
#include "dsp/FilterFx.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers visualizer tests must not see JUCE headers -- the app core must stay JUCE-free"
#endif

#include <cmath>
#include <cstddef>
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

synth::RuntimeDataPaths UseScratchRuntimeDataPaths(const char* testName) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-visualizer-tests" / testName;
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

// -----------------------------------------------------------------------
// Task 7.4 -- "scope channels bound; visualizer connected flag true after
// processing." Before any ProcessBlock() call, the UIState the visualizer
// reads has never been populated (default-constructed `connected = false`);
// after at least one block, each VCO's channel is bound and its UIState's
// `connected` flag is true.
// -----------------------------------------------------------------------
TEST_CASE(vco_scope_channels_bound_and_visualizer_connected_after_processing) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("vco_scope"));

    for (std::size_t vcoIx = 0; vcoIx < 3; ++vcoIx) {
        REQUIRE_TRUE(!rig.Application().VcoScopeUiState(vcoIx).connected.load());
    }

    rig.StartAt(0);
    rig.RunBlocks(4);

    for (std::size_t vcoIx = 0; vcoIx < 3; ++vcoIx) {
        const auto& state = rig.Application().VcoScopeUiState(vcoIx);
        REQUIRE_TRUE(state.connected.load());
        REQUIRE_TRUE(state.scope.load() != nullptr);
        // Task 7.2: ReserveChans(1) called once per VCO, in order -- each
        // VCO's own flat channel index should equal its own index.
        REQUIRE_TRUE(state.scopeChannel.load() == vcoIx);
    }

    // Task 7.3: ScopeVisualizer<UIState> renders with NO app-level drawing
    // code (this test file adds none) -- Draw() must produce at least one
    // command once the underlying scope has live data.
    auto& visualizer = rig.Application().VcoScopeVisualizer();
    visualizer.SetBounds(synth::ui::Bounds{0.0f, 0.0f, 300.0f, 120.0f});
    const std::vector<synth::ui::DrawCommand> commands = visualizer.Draw();
    REQUIRE_TRUE(!commands.empty());
}

// -----------------------------------------------------------------------
// UI-rework ITEM 3 failing-test-first (design.md A3d, tasks.md B.3,
// 2026-07-29): before this fix, dsp::Vco::Process() wrote its raw,
// PRE-gate output to the scope unconditionally, so the VCO scope visibly
// animated even with the transport never started -- operator: "i haven't
// clicked play at all yet, and the VCO oscilloscope still shows waves
// moving. so, doesn't make much sense." With the transport never started,
// `audioAdsr_` never leaves `Stage::Idle` (VcoAdsrState::init() leaves the
// gate low, and setGate() is never called with `true` here -- see
// FroggersAppCore::ProcessBlock's own "closed outright" comment), so every
// gated voice level is exactly 0.0f and the post-gate scope write
// (RouteAudioSample(), FroggersAppCore.hpp) writes exactly 0.0f every
// sample for all three VCO channels -- i.e. every trace is flat. This does
// NOT drive the transport at all (no `rig.StartAt(...)`, matching
// FroggersAudioRoutingTests.cpp's own silent-while-stopped convention) --
// RouteAudioSample()/AdvanceIndex() still run every sample regardless of
// transport state (FroggersAppCore.hpp's own comment on that), so the
// scope reader is non-empty; its values must still all be zero.
// -----------------------------------------------------------------------
TEST_CASE(vco_scope_traces_are_flat_when_transport_never_started) {
    Rig rig(/*patchPumpBudgetBlocks=*/64, UseScratchRuntimeDataPaths("vco_scope_flat_before_play"));

    // Transport is stopped by default -- deliberately no rig.StartAt(...).
    rig.RunBlocks(1);

    for (std::size_t vcoIx = 0; vcoIx < 3; ++vcoIx) {
        const auto& state = rig.Application().VcoScopeUiState(vcoIx);
        REQUIRE_TRUE(state.connected.load());
        const synth::ScopeWriter* const writer = state.scope.load();
        REQUIRE_TRUE(writer != nullptr);

        const synth::ScopeReader reader(writer, state.scopeChannel.load(), /*numXSamples=*/64);
        REQUIRE_TRUE(!reader.Empty());
        for (int x = 0; x < 64; ++x) {
            REQUIRE_TRUE(reader.Get(static_cast<double>(x)) == 0.0f);
        }
    }
}

// -----------------------------------------------------------------------
// Task 9.4 -- "self-oscillating comb feedback produces only finite plot
// values, and the displayed magnitude is bounded rather than drawn out of
// frame." Drives dsp::Comb::UIState with the comb's own worst-case
// self-oscillating configuration (Comb::GetFeedback's +-0.95 ceiling --
// item 1, design.md A2, was +-1.1 -- with a near-unity lowpass so the loop
// gain's magnitude gets close to 1 across a wide band) directly into
// TransferFunctionVisualizer -- no FroggersApp needed, since this is
// purely about the Visualizer/UIState contract task 9.1/9.2 add.
// -----------------------------------------------------------------------
TEST_CASE(self_oscillating_comb_visualizer_plot_stays_within_bounds) {
    synth_froggers::dsp::Comb::UIState state;
    state.feedback.store(0.95f);       // Comb::GetFeedback's maximum magnitude (item 1).
    state.lowPassAlpha.store(0.999f);  // near-unity lowpass -- broad, near-1 passband.
    state.delaySamples.store(1);       // shortest delay -- widest periodic lobe.

    synth_froggers::TransferFunctionVisualizer visualizer(state, synth::Color::Blue);
    const synth::ui::Bounds bounds{0.0f, 0.0f, 320.0f, 160.0f};
    visualizer.SetBounds(bounds);

    const std::vector<synth::ui::DrawCommand> commands = visualizer.Draw();
    REQUIRE_TRUE(!commands.empty());
    bool sawAnyPoint = false;
    for (const auto& command : commands) {
        for (const auto& point : command.points) {
            sawAnyPoint = true;
            REQUIRE_TRUE(std::isfinite(point.x));
            REQUIRE_TRUE(std::isfinite(point.y));
            // "drawn out of frame" would mean a y far outside the panel's
            // own bounds -- assert the polyline's y-coordinates stay
            // within (a small epsilon around) [bounds.y, bounds.y+height],
            // not merely finite.
            REQUIRE_TRUE(point.y >= bounds.y - 1.0e-3f);
            REQUIRE_TRUE(point.y <= bounds.y + bounds.height + 1.0e-3f);
        }
    }
    REQUIRE_TRUE(sawAnyPoint);
}

// A non-self-oscillating comb (feedback 0) should plot a flat 0 dB
// response everywhere -- a sanity check that the bounding above is not
// simply clamping every response to the same value regardless of input.
TEST_CASE(silent_comb_visualizer_plots_flat_unity_response) {
    synth_froggers::dsp::Comb::UIState state;
    state.feedback.store(0.0f);
    state.lowPassAlpha.store(0.5f);
    state.delaySamples.store(100);

    synth_froggers::TransferFunctionVisualizer visualizer(state, synth::Color::Blue);
    const synth::ui::Bounds bounds{0.0f, 0.0f, 320.0f, 160.0f};
    visualizer.SetBounds(bounds);

    const std::vector<synth::ui::DrawCommand> commands = visualizer.Draw();
    REQUIRE_TRUE(commands.size() == 1);
    REQUIRE_TRUE(!commands[0].points.empty());
    // 0 dB maps to the vertical midpoint of [minDb, maxDb] = [-40, 40].
    const float expectedY = bounds.y + bounds.height * 0.5f;
    for (const auto& point : commands[0].points) {
        REQUIRE_TRUE(std::fabs(point.y - expectedY) < 1.0f);
    }
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
