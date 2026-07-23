#pragma once

// FroggersScopePanels -- packet 5 (openspec/changes/desktop-v2-sheaf-
// runtime-harmonization, tasks.md 5.1-5.3).
//
// Owns the dual multi-layer Sheaf ScopeVisualizer panels (3 VCO layers + 3
// LFO EF layers) plus the two GangedRandomLfoVisualizer instances for the
// Random S&H 1/2 mod-depth cells, built entirely from the vendored Sheaf
// portable UI (desktop-v2/External/Sheaf/include/synth/PortableUIBuilders.hpp,
// DspScope.hpp, GangedRandomLfoVisualizer.hpp) -- the ONE scope model this
// product reuses (design.md "Reuse vs creation": no parallel scope stack
// beside the vendored one). This is the replacement for kOscilloscopeTaps'
// EF-only sole-viz role (design D3 / spec desktop-v2-scope-visualization /
// tasks.md 5.2): Source/ui/GlobalOscilloscopeDisplay.* is kept running
// unmodified for the not-yet-cut-over MainComponent shell (shell cutover is
// tasks.md section 10), but this class is the product's real dual-scope
// answer going forward -- kOscilloscopeTaps is no longer sole, because the
// LFO panel is now permanently visible alongside the VCO panel instead of
// being an either/or GlobalOscilloscopeSourceGroup toggle.
//
// Layer source mapping (see manifest/FroggersV2AppManifest.hpp
// kPermanentModulationSources, resolved here by stableId so a manifest
// reorder cannot silently desync the binding):
//   VCO panel    -> vco_1_ef, vco_2_ef, vco_3_ef -- the same 3 stableIds
//                   kOscilloscopeTaps already used. The manifest declares no
//                   separate raw-VCO-audio-output modulation source (VCO
//                   audio only appears summed in the vco_pair_* audio-rate
//                   lanes); these envelope-follower taps are the only
//                   per-VCO scalar the control-rate engine tap
//                   (DesktopHostIO::GetCvOut) publishes, so they are reused
//                   as-is for the "VCO outs trio" panel.
//   LFO EF panel -> lfo_1, lfo_2, lfo_3 (group "lfo") -- the same group
//                   GlobalOscilloscopeDisplay's existing
//                   GlobalOscilloscopeSourceGroup::Lfo alternate view
//                   already reads (GlobalOscilloscopeDisplay.cpp
//                   sourceGroupMatches). The manifest declares no distinct
//                   LFO-envelope-follower source, so "LFO EF" resolves to
//                   this raw LFO group; this is exactly what design.md's
//                   TRACE table cites as "Scope UI ... source groups
//                   include LFO @55" as the basis for the dual-panel
//                   replacement.
//
// GangedRandomLfoVisualizer<1> per Random S&H source: Froggers' Random S&H
// 1/2 (src/core/Marbles.hpp) are each a single step-and-hold scalar output,
// not a polyphonic gang of correlated voices (unlike the Sheaf
// StandardModulators<VoiceCount> usage this visualizer template was
// designed for, see synth/StandardModulators.hpp), so VoiceCount=1 is the
// honest fit -- one visualizer, one traced voice, per design D5 ("Random
// S&H 1/2 remain catalog mod sources with GangedRandomLfoVisualizer only").
// Live Marbles-DSP-to-visualizer data wiring is deliberately not built here
// (see packet-5 report); the UiState is present and Draw()-safe unfed,
// matching the packet-2 SheafVendorSmoke_test precedent of an unfed
// GangedRandomLfoUiState.

#include "manifest/FroggersV2AppManifest.hpp"

#include "synth/AtomicColor.hpp"
#include "synth/Color.hpp"
#include "synth/DspRandomLfo.hpp"
#include "synth/DspScope.hpp"
#include "synth/GangedRandomLfoVisualizer.hpp"
#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace froggers_v2::ui
{

// Mirrors SheafVendorSmoke_test's SmokeScopeLayer field set exactly --
// synth::ui::ScopeVisualizer<LayerState>::DrawVisible() only ever reads
// these four fields (PortableUIBuilders.hpp WaveformLayerDrawState
// snapshot).
struct ScopeLayerState
{
    std::atomic<bool> connected{false};
    synth::AtomicColor scopeColor;
    std::atomic<const synth::ScopeWriter*> scope{nullptr};
    std::atomic<std::size_t> scopeChannel{0};
};

class FroggersScopePanels
{
public:
    static constexpr std::size_t kVcoLayerCount = 3;
    static constexpr std::size_t kLfoEfLayerCount = 3;
    static constexpr std::size_t kScopeChannelCount = kVcoLayerCount + kLfoEfLayerCount;
    static constexpr std::size_t kScopeFrames = 512;
    static constexpr std::size_t kScopeVisualizerSamples = 128;
    static constexpr std::size_t kRandomShCount = 2;

    static constexpr std::array<const char*, kVcoLayerCount> kVcoLayerStableIds{
        {"vco_1_ef", "vco_2_ef", "vco_3_ef"}};
    static constexpr std::array<const char*, kLfoEfLayerCount> kLfoEfLayerStableIds{
        {"lfo_1", "lfo_2", "lfo_3"}};
    static constexpr const char* kRandomSh1StableId = "random_marbles_1";
    static constexpr const char* kRandomSh2StableId = "random_marbles_2";

    FroggersScopePanels()
        : m_scopeWriter(kScopeChannelCount, kScopeFrames)
        , m_vcoChans(m_scopeWriter.ReserveChans(kVcoLayerCount))
        , m_lfoEfChans(m_scopeWriter.ReserveChans(kLfoEfLayerCount))
        , m_vcoLayers{}
        , m_lfoEfLayers{}
        , m_vcoLayerPtrs{{&m_vcoLayers[0], &m_vcoLayers[1], &m_vcoLayers[2]}}
        , m_lfoEfLayerPtrs{{&m_lfoEfLayers[0], &m_lfoEfLayers[1], &m_lfoEfLayers[2]}}
        , m_vcoScope(m_vcoLayerPtrs, -1.0f, 1.0f, kScopeVisualizerSamples, false)
        , m_lfoEfScope(m_lfoEfLayerPtrs, -1.0f, 1.0f, kScopeVisualizerSamples, false)
        , m_randomShUiState{}
        , m_randomShVisualizer0(m_randomShUiState[0])
        , m_randomShVisualizer1(m_randomShUiState[1])
    {
        for (std::size_t i = 0; i < kVcoLayerCount; ++i)
        {
            BindLayer(m_vcoLayers[i], m_vcoChans, i, kVcoLayerStableIds[i]);
            m_vcoModIndices[i] = ModIndexForStableId(kVcoLayerStableIds[i]);
        }
        for (std::size_t i = 0; i < kLfoEfLayerCount; ++i)
        {
            BindLayer(m_lfoEfLayers[i], m_lfoEfChans, i, kLfoEfLayerStableIds[i]);
            m_lfoEfModIndices[i] = ModIndexForStableId(kLfoEfLayerStableIds[i]);
        }
    }

    FroggersScopePanels(const FroggersScopePanels&) = delete;
    FroggersScopePanels& operator=(const FroggersScopePanels&) = delete;

    synth::ui::ScopeVisualizer<ScopeLayerState>& VcoScope() { return m_vcoScope; }
    const synth::ui::ScopeVisualizer<ScopeLayerState>& VcoScope() const { return m_vcoScope; }
    synth::ui::ScopeVisualizer<ScopeLayerState>& LfoEfScope() { return m_lfoEfScope; }
    const synth::ui::ScopeVisualizer<ScopeLayerState>& LfoEfScope() const { return m_lfoEfScope; }

    synth::ui::GangedRandomLfoVisualizer<1>& RandomShVisualizer(std::size_t index)
    {
        return index == 0 ? m_randomShVisualizer0 : m_randomShVisualizer1;
    }
    const synth::ui::GangedRandomLfoVisualizer<1>& RandomShVisualizer(std::size_t index) const
    {
        return index == 0 ? m_randomShVisualizer0 : m_randomShVisualizer1;
    }

    // Portable-visualizers spec "Hidden visualizer emits no node": returns
    // nullptr for any stableId that is not a Random S&H mod-depth cell, so
    // callers building the mod-detail NodeTree can pass the result straight
    // into synth::ui::Builder::Visualizer(id, ptr) unconditionally -- no
    // Step chance / Deja vu / Bag size / Slew (deleted packet-4 page)
    // lookup exists or is needed here.
    synth::ui::Visualizer* VisualizerForModSource(const char* stableId)
    {
        if (stableId == nullptr)
        {
            return nullptr;
        }
        if (std::strcmp(stableId, kRandomSh1StableId) == 0)
        {
            return &m_randomShVisualizer0;
        }
        if (std::strcmp(stableId, kRandomSh2StableId) == 0)
        {
            return &m_randomShVisualizer1;
        }
        return nullptr;
    }

    std::uint8_t VcoModIndex(std::size_t layer) const { return m_vcoModIndices.at(layer); }
    std::uint8_t LfoEfModIndex(std::size_t layer) const { return m_lfoEfModIndices.at(layer); }

    // Pushes one sample tick into every reserved scope channel and advances
    // + publishes the shared synth::ScopeWriter once -- both panels read
    // the same writer, just different reserved channel ranges (the "one
    // scope model").
    void PushTick(const std::array<float, kVcoLayerCount>& vcoValues,
                 const std::array<float, kLfoEfLayerCount>& lfoEfValues)
    {
        for (std::size_t i = 0; i < kVcoLayerCount; ++i)
        {
            m_vcoChans.Write(i, vcoValues[i]);
        }
        for (std::size_t i = 0; i < kLfoEfLayerCount; ++i)
        {
            m_lfoEfChans.Write(i, lfoEfValues[i]);
        }
        m_scopeWriter.AdvanceIndex(1);
        m_scopeWriter.Publish();
    }

    // Test-only accessors (FroggersScopePanels_test.cpp): expose the raw
    // layer state so coverage can assert the two panels share one
    // synth::ScopeWriter without duplicating the private binding logic.
    ScopeLayerState& VcoLayerForTest(std::size_t index) { return m_vcoLayers.at(index); }
    ScopeLayerState& LfoEfLayerForTest(std::size_t index) { return m_lfoEfLayers.at(index); }

private:
    static void BindLayer(ScopeLayerState& layer,
                          const synth::ScopeWriterHolder& chans,
                          std::size_t relativeChan,
                          const char* stableId)
    {
        layer.connected.store(true, std::memory_order_relaxed);
        layer.scopeColor.Store(ColorForStableId(stableId));
        layer.scope.store(chans.Writer(), std::memory_order_relaxed);
        layer.scopeChannel.store(chans.FlatChan(relativeChan), std::memory_order_relaxed);
    }

    static synth::Color ColorForStableId(const char* stableId)
    {
        using froggers_v2::manifest::kPermanentModulationSources;
        for (const auto& source : kPermanentModulationSources)
        {
            if (source.stableId != nullptr && std::strcmp(source.stableId, stableId) == 0)
            {
                const std::uint32_t rgb = source.colorRgb;
                return synth::Color::Rgb(static_cast<std::uint8_t>((rgb >> 16) & 0xffu),
                                         static_cast<std::uint8_t>((rgb >> 8) & 0xffu),
                                         static_cast<std::uint8_t>(rgb & 0xffu));
            }
        }
        return synth::Color::Grey;
    }

    static std::uint8_t ModIndexForStableId(const char* stableId)
    {
        using froggers_v2::manifest::kPermanentModulationSources;
        for (std::uint8_t i = 0; i < kPermanentModulationSources.size(); ++i)
        {
            const char* candidate = kPermanentModulationSources[i].stableId;
            if (candidate != nullptr && std::strcmp(candidate, stableId) == 0)
            {
                return i;
            }
        }
        return UINT8_MAX;
    }

    synth::ScopeWriter m_scopeWriter;
    synth::ScopeWriterHolder m_vcoChans;
    synth::ScopeWriterHolder m_lfoEfChans;
    std::array<ScopeLayerState, kVcoLayerCount> m_vcoLayers;
    std::array<ScopeLayerState, kLfoEfLayerCount> m_lfoEfLayers;
    std::array<ScopeLayerState*, kVcoLayerCount> m_vcoLayerPtrs;
    std::array<ScopeLayerState*, kLfoEfLayerCount> m_lfoEfLayerPtrs;
    synth::ui::ScopeVisualizer<ScopeLayerState> m_vcoScope;
    synth::ui::ScopeVisualizer<ScopeLayerState> m_lfoEfScope;
    std::array<synth::GangedRandomLfoUiState<1>, kRandomShCount> m_randomShUiState;
    synth::ui::GangedRandomLfoVisualizer<1> m_randomShVisualizer0;
    synth::ui::GangedRandomLfoVisualizer<1> m_randomShVisualizer1;
    std::array<std::uint8_t, kVcoLayerCount> m_vcoModIndices{};
    std::array<std::uint8_t, kLfoEfLayerCount> m_lfoEfModIndices{};
};

} // namespace froggers_v2::ui
