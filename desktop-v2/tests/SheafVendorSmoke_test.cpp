// SheafVendorSmoke_test -- packet 2 compile gate for the vendored Sheaf
// Runtime/Engine/portable-visualizer slice
// (openspec/changes/desktop-v2-sheaf-runtime-harmonization, tasks 2.1-2.3).
//
// Proves the vendored code under desktop-v2/External/Sheaf compiles and
// links against this product's own JUCE modules, with no network
// FetchContent of Sheaf (see desktop-v2/CMakeLists.txt: SheafRuntimeVendor
// sources are copied in-tree, not fetched).
//
// This test intentionally does NOT wire SYNTH_RUNTIME_MAIN / instantiate
// synth_runtime::ShellApplication<AppType> as a running JUCE application --
// that cutover is tasks.md section 10 (a later packet). It only proves:
//   - synth::Engine<App> (and therefore synth::GridManager and
//     synth::RuntimeUIState, which Engine's constructor unconditionally
//     wires regardless of the App type) compiles and constructs for a
//     minimal stub App satisfying synth::SynthApplication, even though
//     FroggersApp does not exist yet (packet 3, tasks.md 3.1) and leaves
//     grids unused (tasks.md 2.2).
//   - synth::ScopeWriter, synth::ui::ScopeVisualizer, and
//     synth::ui::GangedRandomLfoVisualizer construct and draw.
//   - The JUCE-facing runtime headers (runtime/Runtime.hpp,
//     runtime/Shell.hpp, runtime/MainPane.hpp, juce/PortableJuceBackend.hpp,
//     juce/RuntimePagesJuce.hpp, juce/MidiHandlers.hpp) parse cleanly
//     against this product's linked JUCE modules.

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "runtime/Runtime.hpp"
#include "runtime/Shell.hpp"
#include "runtime/MainPane.hpp"
#include "juce/PortableJuceBackend.hpp"
#include "juce/RuntimePagesJuce.hpp"
#include "juce/MidiHandlers.hpp"

#include "synth/AppConcepts.hpp"
#include "synth/Engine.hpp"
#include "synth/RuntimeUIState.hpp"
#include "synth/ButtonGrid.hpp"
#include "synth/DspScope.hpp"
#include "synth/DspRandomLfo.hpp"
#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"
#include "synth/GangedRandomLfoVisualizer.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

// Minimal stub satisfying synth::SynthApplication -- NOT FroggersApp
// (packet 3, tasks.md 3.1). Exists only so synth::Engine<StubApp> below
// forces real template instantiation of the vendored Engine / GridManager /
// RuntimeUIState machinery instead of leaving it as unexercised dead code.
class StubSurface final : public synth::ui::Surface
{
public:
    synth::ui::NodeTree BuildTree() override { return {}; }
    void SetActionHandler(synth::ui::Surface::ActionHandler) override {}
    void DispatchAction(const synth::ui::Action&) override {}
};

class StubApp
{
public:
    static synth::RuntimeConfig Config()
    {
        synth::RuntimeConfig config;
        config.appName = "SheafVendorSmoke";
        return config;
    }

    void Init(synth::AppContext*) {}
    void ProcessBlock(synth::AudioBlock&) {}
    synth::ui::Surface& PortableSurface() { return m_surface; }

private:
    StubSurface m_surface;
};

static_assert(synth::SynthApplication<StubApp>,
              "StubApp must satisfy synth::SynthApplication for "
              "synth::Engine<StubApp> to instantiate the vendored "
              "Engine/GridManager/RuntimeUIState code paths below");

// Minimal LayerState satisfying ScopeVisualizer<LayerState>'s DrawVisible()
// field access (connected / scopeColor / scope / scopeChannel).
struct SmokeScopeLayer
{
    std::atomic<bool> connected{false};
    synth::AtomicColor scopeColor;
    std::atomic<const synth::ScopeWriter*> scope{nullptr};
    std::atomic<std::size_t> scopeChannel{0};
};

bool test_engine_grid_runtime_ui_state_compile()
{
    synth::Engine<StubApp> engine([]() -> std::uint64_t { return 0; });
    (void) engine;
    return true;
}

bool test_scope_writer_and_visualizer_compile()
{
    synth::ScopeWriter writer;
    (void) writer;

    SmokeScopeLayer layer;
    std::array<SmokeScopeLayer*, 1> layers{ &layer };
    synth::ui::ScopeVisualizer<SmokeScopeLayer> scope(layers, -1.0f, 1.0f, 64, false);
    scope.SetBounds(synth::ui::Bounds{0.0f, 0.0f, 100.0f, 40.0f});
    scope.SetVisible(true);
    const auto commands = scope.Draw();
    (void) commands;
    return true;
}

bool test_ganged_random_lfo_visualizer_compiles()
{
    constexpr std::size_t kVoiceCount = 3;
    synth::GangedRandomLfoUiState<kVoiceCount> uiState;
    synth::ui::GangedRandomLfoVisualizer<kVoiceCount> visualizer(uiState);
    visualizer.SetBounds(synth::ui::Bounds{0.0f, 0.0f, 100.0f, 40.0f});
    visualizer.SetVisible(true);
    const auto commands = visualizer.Draw();
    (void) commands;
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_engine_grid_runtime_ui_state_compile() && ok;
    ok = test_scope_writer_and_visualizer_compile() && ok;
    ok = test_ganged_random_lfo_visualizer_compiles() && ok;

    if (!ok)
    {
        std::printf("FAIL: SheafVendorSmoke_test\n");
        return 1;
    }
    std::printf("PASS: SheafVendorSmoke_test\n");
    return 0;
}
