#pragma once

// FroggersAppSurface -- packet 3 stub (tasks.md 3.1) extended by packet 5
// (openspec/changes/desktop-v2-sheaf-runtime-harmonization, tasks.md
// 5.1-5.3). Relocated out of Source/FroggersApp.hpp into its own
// JUCE-free header so it (and its Sheaf-vendored + manifest dependencies)
// can be exercised standalone, without pulling in AudioEngine/JUCE
// (FroggersAppSurface_test.cpp).
//
// BuildTree() composes the surface's synth::ui::NodeTree via the vendored
// synth::ui::Builder (PortableUIBuilders.hpp), which is where the two dual
// ScopeVisualizer panels (task 5.1) and the GangedRandomLfoVisualizer on the
// Random S&H 1/2 mod-depth cells (task 5.3) become visible/testable through
// the actual Surface contract, using Builder::Visualizer(id, ptr) -- the
// same helper that implements the portable-visualizers spec's "Hidden
// visualizer emits no node" scenario (it no-ops when the pointer is null or
// the visualizer reports !Visible()).
//
// This is deliberately NOT the full unified Application surface layout
// (encoder bank, per-module sections, transport/global chrome) -- that is
// tasks.md section 7, a later packet. The bounds below are provisional
// placeholders sized only so the panels/cells have a nonzero drawable area
// for tests; the real 1280x920 fit is section 7's job. This file is also
// not wired into Main.cpp / MainComponent (shell cutover is tasks.md
// section 10) and does not touch cross-couplers (packet 7) or the 16-slot
// bank (packet 8).

#include "ui/FroggersScopePanels.hpp"

#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"

namespace froggers_v2
{
class FroggersAppSurface final : public synth::ui::Surface
{
public:
    synth::ui::NodeTree BuildTree() override
    {
        m_scopePanels.VcoScope().SetBounds(kVcoScopeBounds);
        m_scopePanels.VcoScope().SetVisible(true);
        m_scopePanels.LfoEfScope().SetBounds(kLfoEfScopeBounds);
        m_scopePanels.LfoEfScope().SetVisible(true);

        synth::ui::Builder builder;
        builder.Root("froggers_app_surface", kSurfaceBounds)
            .Visualizer("vco_scope_panel", &m_scopePanels.VcoScope())
            .Visualizer("lfo_ef_scope_panel", &m_scopePanels.LfoEfScope());

        BindRandomShCell(builder, "mod_depth_random_sh_1", kRandomSh1CellBounds,
                          froggers_v2::ui::FroggersScopePanels::kRandomSh1StableId);
        BindRandomShCell(builder, "mod_depth_random_sh_2", kRandomSh2CellBounds,
                          froggers_v2::ui::FroggersScopePanels::kRandomSh2StableId);

        return builder.Build();
    }

    void SetActionHandler(synth::ui::Surface::ActionHandler) override {}
    void DispatchAction(const synth::ui::Action&) override {}

    froggers_v2::ui::FroggersScopePanels& ScopePanels() { return m_scopePanels; }
    const froggers_v2::ui::FroggersScopePanels& ScopePanels() const { return m_scopePanels; }

private:
    static constexpr synth::ui::Bounds kSurfaceBounds{0.0f, 0.0f, 640.0f, 400.0f};
    static constexpr synth::ui::Bounds kVcoScopeBounds{0.0f, 0.0f, 300.0f, 160.0f};
    static constexpr synth::ui::Bounds kLfoEfScopeBounds{320.0f, 0.0f, 300.0f, 160.0f};
    static constexpr synth::ui::Bounds kRandomSh1CellBounds{0.0f, 180.0f, 80.0f, 80.0f};
    static constexpr synth::ui::Bounds kRandomSh2CellBounds{90.0f, 180.0f, 80.0f, 80.0f};

    void BindRandomShCell(synth::ui::Builder& builder,
                          const char* nodeId,
                          synth::ui::Bounds bounds,
                          const char* stableId)
    {
        synth::ui::Visualizer* visualizer = m_scopePanels.VisualizerForModSource(stableId);
        if (visualizer != nullptr)
        {
            visualizer->SetBounds(bounds);
            visualizer->SetVisible(true);
        }
        builder.Visualizer(nodeId, visualizer);
    }

    froggers_v2::ui::FroggersScopePanels m_scopePanels;
};
} // namespace froggers_v2
