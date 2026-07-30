#pragma once

// synth_froggers::dsp::Vco -- packet 3 task 3.1 (DSP port).
// openspec/changes/froggers-sheaf-app/tasks.md section 3, item 3.1; design
// D7 ("Froggers VCO topology exposing Sheaf's VCO UIState"). A **copy**
// (design D3), not an include, of the cited Froggers formulas -- see the
// per-block citations below, each read directly from the frozen source
// before porting.
//
// Ported from:
//   - src/core/FroggersEngine.hpp:439-441   pitch 20 Hz-20 kHz exp map
//   - src/core/FroggersEngine.hpp:135-137   x_pmLfoMinHz/MaxHz/Depth
//   - src/core/FroggersEngine.hpp:147-148,150-165  PmDepthScale smoothstep
//   - src/core/FroggersEngine.hpp:706-712   StepIndependentPmLfo
//   - src/core/FroggersEngine.hpp:735-744   the m_simIndependentPm branch
//     (the depth multiply x_pmLfoDepth * PmDepthScale(knob) lives at this
//     CALLER, not inside StepIndependentPmLfo -- ported that way here too)
//   - src/core/VcoWaveEval.hpp:7-23         EvalWaveMorph sine->saw->square
//
// NOT ported (deliberately): the legacy `else` branch at
// FroggersEngine.hpp:752-754, which uses XCPL cross-coupling between VCOs.
// This struct has zero cross-VCO terms by construction -- it holds no
// reference to any other Vco instance, so "porting only the independent-PM
// branch" and "zero cross-VCO terms" are the same guarantee expressed one
// way: nothing here *could* reach another VCO's state.
//
// ============================================================================
// Packet 7 (tasks.md section "7. VCO scopes", task 7.1; design D7) --
// Sheaf's own `WavetableVco<Bits>` (include/synth/DspOscillators.hpp) is the
// cited shape this struct must conform to: `UIState{connected, scope,
// scopeChannel, scopeColor}` (:119-124) + `SetScopeWriterHolder()` (:133-135)
// + `SetScopeColor()` (:137-139) + `PopulateUIState()` (:165-171) -- `:141-163`
// is `Process`, not part of the cited UIState surface. Sheaf's own reference
// struct embeds this directly alongside its DSP `Process()`, in the SAME
// file/class, rather than through a separate wrapper -- this port mirrors
// that placement exactly (same reasoning as design D10's ResonantBump/Comb
// UIState, app/dsp/FilterFx.hpp), which is why this otherwise-pure DSP file
// now takes on a deliberate, minimal Sheaf dependency: `synth/DspScope.hpp`
// (ScopeWriter/ScopeWriterHolder, header-only, no link dependency) and
// `synth/Color.hpp`/`synth/AtomicColor.hpp` (also header-only). This is a
// widening of the "no Sheaf dependency at all" convention packets 3/6
// established for app/dsp/*.hpp -- flagged explicitly in the packet report,
// not silently done; `app/Makefile`'s DSP_TEST_BIN rule gains Sheaf's
// -I include path accordingly (no link-time dependency: everything reached
// here is header-only inline).
//
// Deliberately NOT ported: WavetableVco::Process's cycle-boundary
// RecordStart/marker bookkeeping (DspOscillators.hpp:158-163). Task 7's own
// tests only require "scope channels bound" and "connected flag true after
// processing" -- no marker/cycle-alignment requirement -- so this port
// writes the raw sample to the scope every Process() call and leaves marker
// support unimplemented, flagged here rather than silently reproduced.

#include "DspMath.hpp"

#include "synth/AtomicColor.hpp"
#include "synth/Color.hpp"
#include "synth/DspScope.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>

namespace synth_froggers::dsp {

// src/core/VcoWaveEval.hpp:7-23, verbatim formula.
inline float EvalWaveMorph(float phaseWrapped01, float morph)
{
    if (!std::isfinite(morph))
    {
        morph = 0.0f;
    }
    const float sine = Sine01(phaseWrapped01);
    const float saw = 2.0f * phaseWrapped01 - 1.0f;
    const float square = (phaseWrapped01 < 0.5f) ? 1.0f : -1.0f;
    if (morph <= 0.5f)
    {
        const float t = morph * 2.0f;
        return sine * (1.0f - t) + saw * t;
    }
    const float t = (morph - 0.5f) * 2.0f;
    return saw * (1.0f - t) + square * t;
}

// One Froggers VCO: pitch (exp map), morph (sine->saw->square), and an
// independent per-VCO PM sine LFO, gated to exactly zero depth at/below the
// knob's zero-off floor. Three of these, each fed its own knobs, reproduce
// the ported topology with zero cross-VCO terms.
struct Vco
{
    // Task 7.1 (design D7): the cited UIState shape, verbatim member names
    // and types (DspOscillators.hpp:119-124).
    struct UIState
    {
        std::atomic<bool> connected{false};
        std::atomic<const synth::ScopeWriter*> scope{nullptr};
        std::atomic<std::size_t> scopeChannel{0};
        synth::AtomicColor scopeColor;
    };

    // FroggersEngine.hpp:135-137 (x_pmLfoMinHz/x_pmLfoMaxHz/x_pmLfoDepth).
    static constexpr float kPmLfoMinHz = 0.05f;
    static constexpr float kPmLfoMaxHz = 20.0f;
    static constexpr float kPmLfoDepth = 0.15f;

    // FroggersEngine.hpp:147-148 (x_pmLfoFloor/x_pmLfoRampWidth).
    static constexpr float kPmLfoFloor = 0.02f;
    static constexpr float kPmLfoRampWidth = 0.08f;

    float carrierPhase = 0.0f;
    float pmLfoPhase = 0.0f;

    // FroggersEngine.hpp:439-441 -- one VCO's pitch knob (0..1) mapped
    // exponentially across 20 Hz-20 kHz, expressed as a phase increment
    // (cycles/sample: freq/sampleRate) so Process() only adds-and-wraps.
    static float PitchToPhaseIncrement(float pitchKnob01, float sampleRate)
    {
        return ExpMapCompute(20.0f / sampleRate, 20000.0f / sampleRate, pitchKnob01);
    }

    // FroggersEngine.hpp:150-165 (PmDepthScale). Thresholds at :147-148.
    static float PmDepthScale(float pmKnob01)
    {
        if (pmKnob01 <= kPmLfoFloor)
        {
            return 0.0f;
        }
        const float rampTop = kPmLfoFloor + kPmLfoRampWidth;
        if (pmKnob01 >= rampTop)
        {
            return 1.0f;
        }
        const float t = (pmKnob01 - kPmLfoFloor) / kPmLfoRampWidth;
        return t * t * (3.0f - 2.0f * t);
    }

    // FroggersEngine.hpp:706-712 (StepIndependentPmLfo) -- advances this
    // VCO's own PM LFO by one sample; returns its PRE-advance sine value.
    // pmKnob01 maps exponentially to [kPmLfoMinHz, kPmLfoMaxHz].
    float StepPmLfo(float pmKnob01, float sampleRate)
    {
        const float hz = ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmKnob01);
        const float lfoValue = Sine01(pmLfoPhase);
        pmLfoPhase = WrapPhase(pmLfoPhase + hz / sampleRate);
        return lfoValue;
    }

    // FroggersEngine.hpp:735-744, the m_simIndependentPm branch only.
    float Process(float pitchKnob01, float morphKnob01, float pmKnob01, float sampleRate)
    {
        const float phaseIncrement = PitchToPhaseIncrement(pitchKnob01, sampleRate);
        // :741-743 -- the depth multiply is at the caller of
        // StepIndependentPmLfo, not inside it; ported the same way.
        const float pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(pmKnob01, sampleRate);
        const float modulatedPhase = WrapPhase(carrierPhase + pmOffset);
        const float output = EvalWaveMorph(modulatedPhase, morphKnob01);
        carrierPhase = WrapPhase(carrierPhase + phaseIncrement);

        // UI-rework ITEM 3 (design.md A3d, tasks.md B.3, 2026-07-29): this
        // struct used to write every raw sample straight to the reserved
        // scope channel here, UNCONDITIONALLY -- before the ASR gate
        // (dsp::MixOscVoices/VcoAdsrState::apply, VoiceEnvelope.hpp) had any
        // chance to run, since MixOscVoices is only called by the caller
        // AFTER all three Vco::Process() calls return (FroggersAppCore.hpp's
        // RouteAudioSample()). That let the scope visibly animate before
        // Play was ever pressed (operator: "i haven't clicked play at all
        // yet, and the VCO oscilloscope still shows waves moving"), because
        // this raw, pre-gate `output` is nonzero regardless of gate state.
        // The write now happens at the CALL SITE instead, after
        // MixOscVoices applies the gate, using the same per-VCO
        // ScopeWriterHolder members FroggersAppCore already owns directly
        // (see RouteAudioSample()) -- so this struct no longer writes to the
        // scope itself at all; `scopeWriterHolder_`/`SetScopeWriterHolder()`
        // stay only for `PopulateUIState()` below, which merely tells the UI
        // which ScopeWriter/channel to poll, not when to write a sample.
        return output;
    }

    // Task 7.1 (design D7): SetScopeWriterHolder/SetScopeColor/PopulateUIState,
    // same shape as WavetableVco's (DspOscillators.hpp:133-135,137-139,165-171).
    void SetScopeWriterHolder(synth::ScopeWriterHolder* holder)
    {
        scopeWriterHolder_ = holder;
    }

    void SetScopeColor(synth::Color color)
    {
        scopeColor_ = color;
    }

    void PopulateUIState(UIState& state) const
    {
        state.scopeColor.Store(scopeColor_);
        const bool connected = scopeWriterHolder_ != nullptr && scopeWriterHolder_->Writer() != nullptr;
        state.connected.store(connected);
        state.scope.store(connected ? scopeWriterHolder_->Writer() : nullptr);
        state.scopeChannel.store(connected ? scopeWriterHolder_->FlatChan() : 0);
    }

    // Task 2.2 (per-unit recovery, app/FroggersAppCore.hpp): zeros only this
    // VCO's own recursive state (the two running phases) -- does NOT touch
    // scopeWriterHolder_/scopeColor_ (wiring, not signal state) or any knob
    // input, since those are recomputed fresh from the caller's arguments on
    // the very next Process() call regardless. Both phases are ordinarily
    // kept in [0,1) by WrapPhase, but `carrierPhase + pmOffset` (:155) can
    // latch non-finite if pmOffset itself ever is (WrapPhase's `floor` of a
    // NaN is NaN, and NaN then propagates through every subsequent
    // WrapPhase() forever) -- Reset() is this unit's only way back.
    void Reset()
    {
        carrierPhase = 0.0f;
        pmLfoPhase = 0.0f;
    }

    // Tasks 2.3/2.4 (Tier 1/Tier 2 recovery): both are pure recursive state,
    // no coefficients to exclude.
    bool StateFinite() const { return std::isfinite(carrierPhase) && std::isfinite(pmLfoPhase); }
    float StateMagnitude() const { return std::max(std::fabs(carrierPhase), std::fabs(pmLfoPhase)); }

private:
    synth::Color scopeColor_ = synth::Color::Cyan;
    synth::ScopeWriterHolder* scopeWriterHolder_ = nullptr;
};

}  // namespace synth_froggers::dsp
