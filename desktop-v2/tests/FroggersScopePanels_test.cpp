// FroggersScopePanels_test -- packet 5 (openspec/changes/desktop-v2-sheaf-
// runtime-harmonization, tasks.md 5.1-5.4).
//
// Dual-panel smoke coverage (task 5.4b): constructs the vendored Sheaf
// ScopeVisualizer/ScopeWriter/GangedRandomLfoVisualizer machinery via
// Source/ui/FroggersScopePanels.hpp (the "one scope model" this product
// reuses -- no parallel scope stack), pushes synthetic sample data through
// the shared synth::ScopeWriter, and Draw()s both the VCO panel (3 layers)
// and the LFO EF panel (3 layers), asserting no crash and the expected
// per-panel draw-command counts (background + midline + one polyline per
// connected layer, since drawMarkers is off). Also proves the two
// GangedRandomLfoVisualizer<1> instances attached to the Random S&H 1/2
// mod-depth cells (design D5 "GangedRandomLfoVisualizer only") Draw()
// safely even before any Marbles data is wired in (mirrors the packet-2
// SheafVendorSmoke_test precedent of an unfed GangedRandomLfoUiState).
//
// Does NOT wire into Main.cpp / MainComponent (shell cutover is tasks.md
// section 10, a later packet) and does NOT touch cross-couplers (packet 7)
// or the 16-slot bank (packet 8).

#include "ui/FroggersScopePanels.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

using froggers_v2::ui::FroggersScopePanels;
using froggers_v2::ui::ScopeLayerState;

namespace
{

bool test_layer_counts_and_stable_id_bindings()
{
    if (FroggersScopePanels::kVcoLayerCount != 3)
    {
        std::printf("FAIL: expected 3 VCO layers, got %zu\n", FroggersScopePanels::kVcoLayerCount);
        return false;
    }
    if (FroggersScopePanels::kLfoEfLayerCount != 3)
    {
        std::printf("FAIL: expected 3 LFO EF layers, got %zu\n", FroggersScopePanels::kLfoEfLayerCount);
        return false;
    }

    static constexpr std::array<const char*, 3> kExpectedVco{{"vco_1_ef", "vco_2_ef", "vco_3_ef"}};
    static constexpr std::array<const char*, 3> kExpectedLfoEf{{"lfo_1", "lfo_2", "lfo_3"}};
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (std::strcmp(FroggersScopePanels::kVcoLayerStableIds[i], kExpectedVco[i]) != 0)
        {
            std::printf("FAIL: VCO layer %zu stableId mismatch\n", i);
            return false;
        }
        if (std::strcmp(FroggersScopePanels::kLfoEfLayerStableIds[i], kExpectedLfoEf[i]) != 0)
        {
            std::printf("FAIL: LFO EF layer %zu stableId mismatch\n", i);
            return false;
        }
    }

    FroggersScopePanels panels;
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (panels.VcoModIndex(i) == UINT8_MAX)
        {
            std::printf("FAIL: VCO layer %zu did not resolve to a manifest mod index\n", i);
            return false;
        }
        if (panels.LfoEfModIndex(i) == UINT8_MAX)
        {
            std::printf("FAIL: LFO EF layer %zu did not resolve to a manifest mod index\n", i);
            return false;
        }
    }
    return true;
}

bool test_ganged_random_visualizer_attached_to_random_sh_cells_only()
{
    FroggersScopePanels panels;

    synth::ui::Visualizer* random1 = panels.VisualizerForModSource(FroggersScopePanels::kRandomSh1StableId);
    synth::ui::Visualizer* random2 = panels.VisualizerForModSource(FroggersScopePanels::kRandomSh2StableId);
    if (random1 == nullptr || random2 == nullptr)
    {
        std::printf("FAIL: Random S&H 1/2 mod-depth cells did not resolve a visualizer\n");
        return false;
    }
    if (random1 == random2)
    {
        std::printf("FAIL: Random S&H 1 and 2 must not share the same visualizer instance\n");
        return false;
    }
    if (random1 != &panels.RandomShVisualizer(0) || random2 != &panels.RandomShVisualizer(1))
    {
        std::printf("FAIL: VisualizerForModSource did not return the RandomShVisualizer(index) identity\n");
        return false;
    }

    // Spec "Hidden visualizer emits no node" precondition: any stableId that
    // is not a Random S&H mod-depth cell must publish no visualizer at all
    // (not just an invisible one), so callers never attempt to draw one.
    static constexpr std::array<const char*, 4> kNonRandomShIds{
        {"vco_pair_12", "lfo_1", "external_audio_rate", "does_not_exist"}};
    for (const char* stableId : kNonRandomShIds)
    {
        if (panels.VisualizerForModSource(stableId) != nullptr)
        {
            std::printf("FAIL: '%s' must not resolve to a ganged random visualizer\n", stableId);
            return false;
        }
    }
    if (panels.VisualizerForModSource(nullptr) != nullptr)
    {
        std::printf("FAIL: null stableId must resolve to nullptr\n");
        return false;
    }
    return true;
}

bool test_ganged_random_visualizer_draws_safely_unfed()
{
    FroggersScopePanels panels;
    for (std::size_t i = 0; i < 2; ++i)
    {
        auto& visualizer = panels.RandomShVisualizer(i);
        visualizer.SetBounds(synth::ui::Bounds{0.0f, 0.0f, 80.0f, 80.0f});
        visualizer.SetVisible(true);
        const auto commands = visualizer.Draw();
        if (commands.empty())
        {
            std::printf("FAIL: unfed GangedRandomLfoVisualizer(%zu) drew no commands (expected background+axis)\n", i);
            return false;
        }
    }
    return true;
}

bool test_dual_panel_smoke_expected_layer_counts()
{
    FroggersScopePanels panels;

    panels.VcoScope().SetBounds(synth::ui::Bounds{0.0f, 0.0f, 200.0f, 80.0f});
    panels.VcoScope().SetVisible(true);
    panels.LfoEfScope().SetBounds(synth::ui::Bounds{0.0f, 90.0f, 200.0f, 80.0f});
    panels.LfoEfScope().SetVisible(true);

    // Push enough ticks that ScopeReader's no-marker fallback window is
    // non-empty (DspScope.hpp ScopeReader ctor: empty when
    // publishedIndex <= 1), with distinct per-layer values so this is real
    // per-layer sample data, not a shared scalar.
    constexpr int kTicks = 64;
    for (int t = 0; t < kTicks; ++t)
    {
        const float phase = static_cast<float>(t) * 0.1f;
        panels.PushTick(
            {std::sin(phase), std::sin(phase + 1.0f), std::sin(phase + 2.0f)},
            {std::cos(phase), std::cos(phase + 1.0f), std::cos(phase + 2.0f)});
    }

    const auto vcoCommands = panels.VcoScope().Draw();
    const auto lfoEfCommands = panels.LfoEfScope().Draw();

    // Background Fill + midline Line + one Polyline per connected,
    // non-empty layer (drawMarkers is off) -- see
    // PortableUIBuilders.hpp BuildScopeWaveformCommands.
    constexpr std::size_t kExpectedCommandsPerPanel = 2 + FroggersScopePanels::kVcoLayerCount;
    if (vcoCommands.size() != kExpectedCommandsPerPanel)
    {
        std::printf("FAIL: VCO panel drew %zu commands, expected %zu\n",
                    vcoCommands.size(),
                    kExpectedCommandsPerPanel);
        return false;
    }
    if (lfoEfCommands.size() != kExpectedCommandsPerPanel)
    {
        std::printf("FAIL: LFO EF panel drew %zu commands, expected %zu\n",
                    lfoEfCommands.size(),
                    kExpectedCommandsPerPanel);
        return false;
    }

    std::size_t vcoPolylineCount = 0;
    for (const auto& command : vcoCommands)
    {
        if (command.kind == synth::ui::DrawCommand::Kind::Polyline)
        {
            ++vcoPolylineCount;
        }
    }
    if (vcoPolylineCount != FroggersScopePanels::kVcoLayerCount)
    {
        std::printf("FAIL: VCO panel drew %zu polylines, expected %zu (one per layer)\n",
                    vcoPolylineCount,
                    FroggersScopePanels::kVcoLayerCount);
        return false;
    }

    std::size_t lfoEfPolylineCount = 0;
    for (const auto& command : lfoEfCommands)
    {
        if (command.kind == synth::ui::DrawCommand::Kind::Polyline)
        {
            ++lfoEfPolylineCount;
        }
    }
    if (lfoEfPolylineCount != FroggersScopePanels::kLfoEfLayerCount)
    {
        std::printf("FAIL: LFO EF panel drew %zu polylines, expected %zu (one per layer)\n",
                    lfoEfPolylineCount,
                    FroggersScopePanels::kLfoEfLayerCount);
        return false;
    }

    return true;
}

bool test_scope_channels_share_one_writer()
{
    // "One scope model" (design.md "Reuse"): both panels must be fed by the
    // SAME synth::ScopeWriter instance, not two independent writers.
    FroggersScopePanels panels;
    ScopeLayerState& vcoLayer0 = panels.VcoLayerForTest(0);
    ScopeLayerState& lfoEfLayer0 = panels.LfoEfLayerForTest(0);
    const synth::ScopeWriter* vcoWriter = vcoLayer0.scope.load();
    const synth::ScopeWriter* lfoEfWriter = lfoEfLayer0.scope.load();
    if (vcoWriter == nullptr || lfoEfWriter == nullptr || vcoWriter != lfoEfWriter)
    {
        std::printf("FAIL: VCO and LFO EF layers must share one synth::ScopeWriter\n");
        return false;
    }
    if (vcoLayer0.scopeChannel.load() == lfoEfLayer0.scopeChannel.load())
    {
        std::printf("FAIL: VCO and LFO EF layer 0 must not alias the same scope channel\n");
        return false;
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_layer_counts_and_stable_id_bindings() && ok;
    ok = test_ganged_random_visualizer_attached_to_random_sh_cells_only() && ok;
    ok = test_ganged_random_visualizer_draws_safely_unfed() && ok;
    ok = test_dual_panel_smoke_expected_layer_counts() && ok;
    ok = test_scope_channels_share_one_writer() && ok;

    if (!ok)
    {
        std::printf("FAIL: FroggersScopePanels_test\n");
        return 1;
    }
    std::printf("PASS: FroggersScopePanels_test\n");
    return 0;
}
