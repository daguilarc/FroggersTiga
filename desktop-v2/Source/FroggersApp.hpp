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
// The surface returned here is intentionally a minimal stub -- it mirrors the
// StubSurface precedent already vendored in SheafVendorSmoke_test.cpp
// (packet 2) rather than inventing real UI content. Building the actual
// Application surface (dual ScopeVisualizer panels, encoder bank, mod detail
// grid) is tasks.md section 5+, a later packet; this packet's scope is only
// the host-boundary type + its headless test (tasks.md 3.3).

#include "FroggersAppCore.hpp"

#include "synth/AppConcepts.hpp"
#include "synth/PortableUI.hpp"

namespace froggers_v2
{
class FroggersAppSurface final : public synth::ui::Surface
{
public:
    synth::ui::NodeTree BuildTree() override { return {}; }
    void SetActionHandler(synth::ui::Surface::ActionHandler) override {}
    void DispatchAction(const synth::ui::Action&) override {}
};
} // namespace froggers_v2

class FroggersApp : public FroggersAppCore
{
public:
    // synth::SynthApplication's remaining member beyond
    // SynthApplicationCore.
    synth::ui::Surface& PortableSurface() { return m_surface; }

private:
    froggers_v2::FroggersAppSurface m_surface;
};

static_assert(synth::SynthApplication<FroggersApp>,
              "FroggersApp must satisfy synth::SynthApplication for "
              "synth::Engine<FroggersApp> (and the future Sheaf Runtime "
              "shell, tasks.md section 10) to drive it");
