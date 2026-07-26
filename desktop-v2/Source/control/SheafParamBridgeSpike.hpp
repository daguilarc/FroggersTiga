#pragma once

// SheafParamBridgeSpike -- desktop-v2-sheaf-runtime-harmonization tasks.md
// section 13.1 (bounded spike). PROOF OF SHAPE ONLY: validates that a
// Sheaf-native synth::Parameter's resolved value can reach the existing
// Froggers DSP through the existing (unmodified) DesktopHostIO::SetPageKnob
// boundary.
//
// This header is additive glue for SheafParamSpike_test only. It does NOT
// touch FroggersV2ControlCore / FroggersV2HostBridge and is NOT wired into
// any live bridge path -- it exists to give the real §13.4 bridge rewrite a
// concrete, exercised starting point for the read-method choice below, not
// to prescribe the production glue shape.
//
// Read-method choice: Parameter::CachedKnobValue(voiceIx) -- the
// un-modulated, "as turned" knob position, seeded by the Parameter
// constructor and refreshed by ParameterManager::ComputeAllParameters() ->
// Parameter::SnapCurrentToTarget() -> Parameter::SeedCachedKnobAndUiDisplay
// State() -- is the correct read for this boundary, NOT Parameter::GetRaw().
// GetRaw() already folds in Sheaf-side modulation (it adds
// group.GetModulators().ApplyActive(...) over the parameter's active
// modulation-depth routes on top of the cached/current center). SetPageKnob's
// `value` argument feeds Froggers::Parameter::KnobUpdate, which is the
// *physical knob position* that Froggers' own separate ModMgr / V2Fuego
// modulation stack operates on downstream of this boundary. Bridging
// GetRaw() here would double-apply modulation once both stacks are live
// simultaneously. CachedKnobValue is the Sheaf-side analogue of that raw
// knob position, so it is what this spike forwards.

#include "synth/ParameterModulation.hpp"

#include "DesktopHostIO.hpp"

#include <cstddef>
#include <cstdint>

namespace froggers_v2::spike
{

// Forwards one Sheaf Parameter's current cached (raw, pre-Froggers-side-
// modulation) knob value to the existing Froggers DSP boundary via
// DesktopHostIO::SetPageKnob. Callers must have already driven the Sheaf
// side to a resolved state (Parameter::HandleSetAbsolute(...) followed by
// ParameterManager::ComputeAllParameters()) before calling this -- see the
// spike test for the full sequence.
//
// voiceIx selects which Sheaf voice's cached knob value to read; Froggers
// pages are not per-voice, so callers pick voice 0 unless proving
// multi-voice fan-out is explicitly in scope (it is not, for this spike --
// see the spike's "does NOT yet prove" notes).
inline void PushSheafParamToPageKnob(synth::Parameter& param,
                                     DesktopHostIO& host,
                                     uint8_t page,
                                     uint8_t position,
                                     std::size_t voiceIx = 0)
{
    host.SetPageKnob(page, position, param.CachedKnobValue(voiceIx));
}

}  // namespace froggers_v2::spike
