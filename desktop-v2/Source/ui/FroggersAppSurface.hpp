#pragma once

// FroggersAppSurface -- packet 3 stub (tasks.md 3.1) extended by packet 5
// (openspec/changes/desktop-v2-sheaf-runtime-harmonization, tasks.md
// 5.1-5.3) and packet 7 increment 1 (tasks.md 7.2, design.md "Layout
// addendum" Candidate A). Relocated out of Source/FroggersApp.hpp into its
// own JUCE-free header so it (and its Sheaf-vendored + manifest
// dependencies) can be exercised standalone, without pulling in
// AudioEngine/JUCE (FroggersAppSurface_test.cpp).
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
// Packet 7 increment 1 adds the Candidate-A unified-layout skeleton on top
// of the packet-5 scopes: a chrome-placeholder rectangle beside the scopes
// (real transport/global-command relocate is a later increment, tasks.md
// 7.3), a 6-tab module selector (Audio, Envelope, Filter, Drive, Reverb,
// Delay) holding a single portable active-module index (DispatchAction is
// its only writer -- design.md "no parallel page-state"), and the active
// module's <=16-slot 4x4 grid. Audio is ported fully as the reference
// pattern: it reads the same source SubmodulePagePanel.cpp's refresh() reads
// today (Source/control/FroggersV2ControlCore.hpp's visibleCount() /
// visibleRowForSlot() / effectiveRow(), and
// Source/V2DesktopPageDisplayNames.hpp's forHostPageRow() for labels) --
// just JUCE-free and rendered through synth::ui (EncoderDraw.hpp's
// BuildEncoderDrawCommands) instead of juce::Label / EncoderRingComponent.
// The other five modules render a single labeled stub node while active;
// they get fully ported in later increments (do not build their grids here).
//
// This is deliberately NOT the full unified Application surface layout yet
// (mod-detail-grid drill-in swap, transport/global chrome, Envelope/Filter/
// Drive/Reverb/Delay content) -- those are later tasks.md section 7 packets.
// This file is also not wired into Main.cpp / MainComponent (shell cutover
// is tasks.md section 10) and does not touch cross-couplers or the DSP/
// control-core's own authority (packet 8's 16-slot Crunchy/Crispy map is
// unaffected -- the grid here only reads FroggersV2ControlCore, it does not
// change its semantics).

#include "ui/FroggersScopePanels.hpp"

#include "V2DesktopPageDisplayNames.hpp"
#include "control/FroggersV2ControlCore.hpp"

#include "synth/EncoderDraw.hpp"
#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace froggers_v2
{
class FroggersAppSurface final : public synth::ui::Surface
{
public:
    // Candidate-A tab order (design.md "Layout addendum"): Audio is the only
    // module ported to portable UI this increment (tasks.md 7.2's reference
    // pattern); the other five render BuildModuleStub()'s placeholder while
    // active. Index 0 (Audio) is the default active module.
    static constexpr std::size_t kModuleCount = 6;
    static constexpr std::array<const char*, kModuleCount> kModuleIds{
        {"audio", "envelope", "filter", "drive", "reverb", "delay"}};
    static constexpr std::array<const char*, kModuleCount> kModuleLabels{
        {"Audio", "Envelope", "Filter", "Drive", "Reverb", "Delay"}};
    static constexpr std::size_t kAudioModuleIndex = 0;

    synth::ui::NodeTree BuildTree() override
    {
        m_scopePanels.VcoScope().SetBounds(kVcoScopeBounds);
        m_scopePanels.VcoScope().SetVisible(true);
        m_scopePanels.LfoEfScope().SetBounds(kLfoEfScopeBounds);
        m_scopePanels.LfoEfScope().SetVisible(true);

        synth::ui::Builder builder;
        builder.Root("froggers_app_surface", kSurfaceBounds)
            .Visualizer("vco_scope_panel", &m_scopePanels.VcoScope())
            .Visualizer("lfo_ef_scope_panel", &m_scopePanels.LfoEfScope())
            .Draw("chrome_placeholder", kChromePlaceholderBounds, BuildChromePlaceholderCommands());

        BindRandomShCell(builder, "mod_depth_random_sh_1", kRandomSh1CellBounds,
                          froggers_v2::ui::FroggersScopePanels::kRandomSh1StableId);
        BindRandomShCell(builder, "mod_depth_random_sh_2", kRandomSh2CellBounds,
                          froggers_v2::ui::FroggersScopePanels::kRandomSh2StableId);

        BuildModuleTabs(builder);
        BuildActiveModuleGrid(builder);

        return builder.Build();
    }

    void SetActionHandler(synth::ui::Surface::ActionHandler) override {}

    // Single active-module authority (design.md "Single active-module
    // authority (no parallel page-state)"): the tab Toggle nodes'
    // select_module Action is the only writer of m_activeModuleIndex, and
    // BuildActiveModuleGrid() is the only reader. Unknown action names/values
    // are ignored (no-op), matching packet 5's DispatchAction no-op default.
    void DispatchAction(const synth::ui::Action& action) override
    {
        if (action.name != kSelectModuleAction)
        {
            return;
        }
        for (std::size_t i = 0; i < kModuleCount; ++i)
        {
            if (action.value == kModuleIds[i])
            {
                m_activeModuleIndex = i;
                return;
            }
        }
    }

    std::size_t ActiveModuleIndex() const { return m_activeModuleIndex; }

    froggers_v2::ui::FroggersScopePanels& ScopePanels() { return m_scopePanels; }
    const froggers_v2::ui::FroggersScopePanels& ScopePanels() const { return m_scopePanels; }

    // Test-only accessor: the Audio module's real data source (same one
    // SubmodulePagePanel reads today), ported here JUCE-free.
    froggers_v2::FroggersV2ControlCore& AudioControlCore() { return m_audioCore; }

private:
    static constexpr const char* kSelectModuleAction = "select_module";
    static constexpr synth::ui::Bounds kSurfaceBounds{0.0f, 0.0f, 1280.0f, 920.0f};
    static constexpr synth::ui::Bounds kVcoScopeBounds{20.0f, 10.0f, 300.0f, 180.0f};
    static constexpr synth::ui::Bounds kLfoEfScopeBounds{340.0f, 10.0f, 300.0f, 180.0f};
    static constexpr synth::ui::Bounds kChromePlaceholderBounds{660.0f, 10.0f, 460.0f, 180.0f};
    static constexpr synth::ui::Bounds kRandomSh1CellBounds{1130.0f, 10.0f, 140.0f, 80.0f};
    static constexpr synth::ui::Bounds kRandomSh2CellBounds{1130.0f, 100.0f, 140.0f, 80.0f};
    static constexpr synth::ui::Bounds kModuleGridBounds{20.0f, 250.0f, 640.0f, 650.0f};
    static constexpr std::size_t kGridColumns = 4;
    static constexpr float kGridCellGap = 6.0f;

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

    void BuildModuleTabs(synth::ui::Builder& builder)
    {
        for (std::size_t i = 0; i < kModuleCount; ++i)
        {
            const std::string tabId = std::string("tab_") + kModuleIds[i];
            builder.Toggle(tabId,
                           kModuleLabels[i],
                           i == m_activeModuleIndex,
                           synth::ui::Action::WithValue(kSelectModuleAction, kModuleIds[i]));
        }
    }

    void BuildActiveModuleGrid(synth::ui::Builder& builder)
    {
        if (m_activeModuleIndex == kAudioModuleIndex)
        {
            BuildAudioModuleGrid(builder);
            return;
        }
        const std::string stubId = std::string(kModuleIds[m_activeModuleIndex]) + "_grid_stub";
        builder.Label(stubId, std::string("Coming next: ") + kModuleLabels[m_activeModuleIndex]);
    }

    // Reference pattern (tasks.md 7.2): reads the same source
    // SubmodulePagePanel::refresh() reads today
    // (desktop-v2/Source/ui/SubmodulePagePanel.cpp) -- visibleCount() /
    // visibleRowForSlot() / effectiveRow() from FroggersV2ControlCore, and
    // V2DesktopPageDisplayNames::forHostPageRow() for labels -- just
    // JUCE-free and rendered through EncoderDraw.hpp's ring commands instead
    // of juce::Label / EncoderRingComponent.
    void BuildAudioModuleGrid(synth::ui::Builder& builder)
    {
        constexpr std::uint8_t kAudioPage = 0;
        const std::uint8_t visible = m_audioCore.visibleCount();
        for (std::uint8_t slot = 0; slot < visible; ++slot)
        {
            const std::uint8_t row = m_audioCore.visibleRowForSlot(slot);
            const char* label = V2DesktopPageDisplayNames::forHostPageRow(kAudioPage, row);
            const synth::ui::Bounds cell = GridCellBounds(slot);

            builder.Label(std::string("audio_grid_label_") + std::to_string(slot), label);

            const froggers_v2::FroggersV2ControlCore::EffectiveRow effective =
                m_audioCore.effectiveRow(kAudioPage, row);
            synth::ui::EncoderDrawState ring;
            ring.connected = true;
            ring.shortLabel = label;
            ring.baseColor = synth::Color::Rgb(120, 170, 220);
            ring.voiceCount = 1;
            synth::ui::EncoderVoiceDrawState voice;
            voice.value = effective.effective;
            voice.minValue = effective.arcMin;
            voice.maxValue = effective.arcMax;
            voice.indicatorColor = ring.baseColor;
            ring.voices.push_back(voice);

            builder.Draw(std::string("audio_grid_ring_") + std::to_string(slot),
                        cell,
                        synth::ui::BuildEncoderDrawCommands(ring, cell));
        }
    }

    static synth::ui::Bounds GridCellBounds(std::uint8_t slot)
    {
        const float cellW =
            (kModuleGridBounds.width - kGridCellGap * static_cast<float>(kGridColumns - 1)) /
            static_cast<float>(kGridColumns);
        const std::size_t col = slot % kGridColumns;
        const std::size_t row = slot / kGridColumns;
        return synth::ui::Bounds{
            kModuleGridBounds.x + static_cast<float>(col) * (cellW + kGridCellGap),
            kModuleGridBounds.y + static_cast<float>(row) * (cellW + kGridCellGap),
            cellW,
            cellW,
        };
    }

    static std::vector<synth::ui::DrawCommand> BuildChromePlaceholderCommands()
    {
        std::vector<synth::ui::DrawCommand> commands;
        commands.push_back(synth::ui::DrawCommand::Fill(kChromePlaceholderBounds, synth::Color::Rgb(20, 22, 24)));
        commands.push_back(
            synth::ui::DrawCommand::StrokeRect(kChromePlaceholderBounds, synth::Color::Rgb(60, 64, 68), 1.0f));
        synth::ui::TextStyle style;
        style.align = synth::ui::TextAlign::Center;
        commands.push_back(synth::ui::DrawCommand::Text(
            kChromePlaceholderBounds, "Transport / Global Commands (placeholder)", style));
        return commands;
    }

    froggers_v2::ui::FroggersScopePanels m_scopePanels;
    froggers_v2::FroggersV2ControlCore m_audioCore;
    std::size_t m_activeModuleIndex = kAudioModuleIndex;
};
} // namespace froggers_v2
