#pragma once

// synth_froggers::FroggersApp -- outer composed application (packet 10 of
// the froggers-sheaf-app change, openspec/changes/froggers-sheaf-app/
// tasks.md section "10. Surface layout (ported v2 design)"; design D11).
//
// This is packets 1-9's own `FroggersApp` class body, split in two the same
// way apps/braid-4 splits Braid4Core.hpp / Braid4UI.hpp / Braid4.hpp:
//   - FroggersAppCore.hpp: Config()/Init()/PrepareToPlay()/ProcessBlock()
//     and every DSP/parameter-model member -- the full
//     `synth::SynthApplicationCore` contract, unchanged in substance from
//     packets 1-9, plus packet 10's own UI-thread -> audio-thread request
//     bridge (ProcessFrame(), the Request*/Display* API) -- see that file's
//     header comment for why the bridge exists.
//   - FroggersUiSurface.hpp: the portable `synth::ui::Surface` -- the real
//     layout (scopes, chrome band, 16-slot grid, in-place mod-detail swap)
//     that packets 1-9 deliberately left as a one-label placeholder.
//
// `synth_froggers::FroggersApp` (this file) composes them exactly like
// Braid4.hpp composes Braid4Core + Braid4UiSurface: it derives from
// FroggersAppCore, adds only the FroggersUiSurface member, and wires
// `Init()` to call `ui_.Attach(context, this)`. Every existing test TU's
// `#include "Froggers.hpp"` / `synth_rig::SynthRig<synth_froggers::
// FroggersApp>` / `synth_froggers::FroggersApp` keeps compiling and running
// unchanged -- this file's own public surface (Config/Init/PrepareToPlay/
// ProcessBlock/ProcessFrame/PortableSurface, plus every packet 1-9 test
// accessor) is identical to before, just now split across a base class and
// this thin derived one.
//
// This header (and everything it includes) is the app core: it must never
// include a JUCE header (task 2.4). The launcher-registration path
// (FroggersRegistration.hpp, task 2.2) is allowed to reach JUCE; this file
// is not.

#include "FroggersAppCore.hpp"
#include "FroggersUiSurface.hpp"

#include "synth/AppConcepts.hpp"

namespace synth_froggers {

class FroggersApp final : public FroggersAppCore {
public:
    void Init(synth::AppContext* context) {
        FroggersAppCore::Init(context);
        // Task 10.1 (design D11): wire the portable surface to this app's
        // context/state, the same call Braid4.hpp makes
        // (`ui_.Attach(context, this)`).
        ui_.Attach(context, this);
    }

    synth::ui::Surface& PortableSurface() { return ui_; }

private:
    FroggersUiSurface ui_;
};

static_assert(synth::SynthApplication<FroggersApp>,
              "FroggersApp must satisfy the full synth::SynthApplication concept "
              "(SynthApplicationCore + PortableSurface())");

}  // namespace synth_froggers
