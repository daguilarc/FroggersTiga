#pragma once

// FroggersApp -- packet 3 (openspec/changes/desktop-v2-sheaf-runtime-
// harmonization, tasks.md 3.1). The concrete type satisfying the full
// vendored synth::SynthApplication concept (Config/Init/ProcessBlock +
// PortableSurface; see
// desktop-v2/External/Sheaf/include/synth/AppConcepts.hpp), so that a future
// synth_runtime::ShellApplication<FroggersApp> (tasks.md section 10, the
// Runtime shell cutover) can host it. Audio behavior is entirely
// FroggersAppCore's (Config/Init/PrepareToPlay/ProcessBlock, delegating to
// the existing AudioEngine + FroggersV2ControlCore/facade path per design
// D2); this file adds only the PortableSurface() hook the full concept
// requires beyond SynthApplicationCore.
//
// froggers_v2::FroggersAppSurface (Source/ui/FroggersAppSurface.hpp) started
// as this packet's minimal stub (mirroring the StubSurface precedent in
// SheafVendorSmoke_test.cpp) and is now, as of packet 5 (tasks.md 5.1-5.3),
// the real dual-scope + Random S&H ganged-visualizer surface. It was
// relocated to its own JUCE-free header so it can be exercised standalone
// (FroggersAppSurface_test.cpp) without AudioEngine/JUCE. Building the full
// unified Application surface (encoder bank, per-module sections) is
// tasks.md section 7, a later packet.

#include "FroggersAppCore.hpp"
#include "ui/FroggersAppSurface.hpp"

#include "synth/AppConcepts.hpp"
#include "synth/PortableUI.hpp"

class FroggersApp : public FroggersAppCore
{
public:
    // synth::SynthApplication's remaining member beyond
    // SynthApplicationCore.
    synth::ui::Surface& PortableSurface() { return m_surface; }

    // Packet 5 (tasks.md 5.1): after the existing AudioEngine DSP path
    // (FroggersAppCore::ProcessBlock, untouched -- no DSP rewrite) has
    // produced this block's audio, pull the same control-rate scalars
    // Source/ui/GlobalOscilloscopeDisplay.cpp already reads via
    // DesktopHostIO::GetCvOut and push them into the portable surface's
    // Sheaf ScopeWriter, giving the dual ScopeVisualizer panels real layer
    // data. GetCvOut is a const read of already-computed per-block mod
    // values (DesktopHostIO::updateMarblesScopeAccum reads the same array
    // the same way, immediately after m_engine.ProcessBlock), so this
    // cannot perturb FroggersApp's audio output -- see FroggersApp_test.cpp
    // test_process_block_matches_direct_facade_path, unchanged and still
    // green after this addition. Hides (does not override; App is used by
    // concrete type in synth::Engine<App>, not through a Surface-style
    // pointer/reference) FroggersAppCore::ProcessBlock.
    void ProcessBlock(synth::AudioBlock& block)
    {
        FroggersAppCore::ProcessBlock(block);

        froggers_v2::ui::FroggersScopePanels& panels = m_surface.ScopePanels();
        DesktopHostIO& host = audioEngine().getHost();
        panels.PushTick(
            {host.GetCvOut(panels.VcoModIndex(0)),
             host.GetCvOut(panels.VcoModIndex(1)),
             host.GetCvOut(panels.VcoModIndex(2))},
            {host.GetCvOut(panels.LfoEfModIndex(0)),
             host.GetCvOut(panels.LfoEfModIndex(1)),
             host.GetCvOut(panels.LfoEfModIndex(2))});
    }

private:
    froggers_v2::FroggersAppSurface m_surface;
};

static_assert(synth::SynthApplication<FroggersApp>,
              "FroggersApp must satisfy synth::SynthApplication for "
              "synth::Engine<FroggersApp> (and the future Sheaf Runtime "
              "shell, tasks.md section 10) to drive it");
