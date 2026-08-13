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

    // Strict-executor packet (Ring Mod, Audio slots 9-11; task A1): the
    // internal ring-mod carrier's own frequency range -- deliberately NOT
    // PitchToPhaseIncrement's 20/20000 Hz pitch literals. 20 Hz-5000 Hz
    // covers sub-audio "throb" through the classic clangy ring-mod register
    // while staying well below Nyquist at every sample rate this app
    // supports, so the carrier itself never folds into noise before the
    // ring-modulated product does.
    static constexpr float kRingModMinHz = 20.0f;
    static constexpr float kRingModMaxHz = 5000.0f;

    // Task A2: Ring Mod's OWN zero-off floor/ramp -- a separate pair of
    // constants from kPmLfoFloor/kPmLfoRampWidth (not merely a separate
    // call), since a bare product carrier is a much more drastic audible
    // change at any nonzero depth than PM's phase offset, so a slightly
    // higher floor and wider ramp keeps the bottom of the knob's travel
    // forgiving.
    static constexpr float kRingModFloor = 0.05f;
    static constexpr float kRingModRampWidth = 0.10f;

    float carrierPhase = 0.0f;
    float pmLfoPhase = 0.0f;
    // Task A1: this VCO's own internal ring-mod carrier phase -- stepped the
    // same way carrierPhase is (WrapPhase(phase + increment)), never derived
    // from or read by any other Vco instance.
    float ringCarrierPhase = 0.0f;

    // F3.3 SPEC CORRECTED 2026-08-07 (openspec/changes/
    // archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md): Tier
    // 2's "how many
    // consecutive seconds has this unit's state stayed over
    // kMaxUnitStateMagnitude" counter (app/FroggersAppCore.hpp's
    // RecoverUnitIfNeeded), owned HERE rather than in FroggersAppCore --
    // it is this unit's own recovery bookkeeping, not shared state, so it
    // belongs with the rest of this struct's state rather than in a
    // parallel member on the enumerating parent (the original placement
    // required a second, address-keyed lookup table to pair a unit back to
    // its own counter once the enumeration went hierarchical; see this
    // task's own SPEC CORRECTED note for why that was wrong).
    float overCeilingSeconds = 0.0f;

    // FroggersEngine.hpp:439-441 -- one VCO's pitch knob (0..1) mapped
    // exponentially across 20 Hz-20 kHz, expressed as a phase increment
    // (cycles/sample: freq/sampleRate) so Process() only adds-and-wraps.
    static float PitchToPhaseIncrement(float pitchKnob01, float sampleRate)
    {
        return ExpMapCompute(20.0f / sampleRate, 20000.0f / sampleRate, pitchKnob01);
    }

    // FroggersEngine.hpp:150-165 (PmDepthScale). Thresholds at :147-148.
    // Body lives in dsp::TrueZeroDepthTaper (DspMath.hpp), shared with any
    // other knob needing the same true-zero taper shape with its own
    // floor/ramp width.
    static float PmDepthScale(float pmKnob01)
    {
        return TrueZeroDepthTaper(pmKnob01, kPmLfoFloor, kPmLfoRampWidth);
    }

    // Task B (PM rate, Audio slot 12): this VCO's own ring-mod depth taper
    // uses TrueZeroDepthTaper with Ring Mod's own floor/ramp (task A2) --
    // NOT kPmLfoFloor/kPmLfoRampWidth, no second copy of the taper itself.
    static float RingModDepthScale(float ringModKnob01)
    {
        return TrueZeroDepthTaper(ringModKnob01, kRingModFloor, kRingModRampWidth);
    }

    // Task A1: ring-mod carrier's phase increment -- same ExpMapCompute
    // shape PitchToPhaseIncrement uses for pitch, over kRingModMinHz/MaxHz.
    static float RingModPhaseIncrement(float ringModKnob01, float sampleRate)
    {
        return ExpMapCompute(kRingModMinHz / sampleRate, kRingModMaxHz / sampleRate, ringModKnob01);
    }

    // FroggersEngine.hpp:706-712 (StepIndependentPmLfo) -- advances this
    // VCO's own PM LFO by one sample; returns its PRE-advance sine value.
    // Strict-executor packet (task B): the RATE argument is now the shared
    // PM-rate knob (Audio slot 12, one knob feeding all three VCOs'
    // StepPmLfo calls), decoupled from the per-VCO PM depth knob that used
    // to drive both -- pmRateKnob01 maps exponentially to [kPmLfoMinHz,
    // kPmLfoMaxHz], same formula as before, just fed by a different knob.
    float StepPmLfo(float pmRateKnob01, float sampleRate)
    {
        const float hz = ExpMapCompute(kPmLfoMinHz, kPmLfoMaxHz, pmRateKnob01);
        const float lfoValue = Sine01(pmLfoPhase);
        pmLfoPhase = WrapPhase(pmLfoPhase + hz / sampleRate);
        return lfoValue;
    }

    // FroggersEngine.hpp:735-744, the m_simIndependentPm branch, plus this
    // packet's Ring Mod (task A) and PM-rate decoupling (task B). pmKnob01
    // still drives ONLY this VCO's own PM depth (PmDepthScale); pmRateKnob01
    // is the new shared Audio-slot-12 rate knob; ringModKnob01 is this VCO's
    // own Ring Mod knob (Audio slot 9/10/11).
    float Process(float pitchKnob01, float morphKnob01, float pmKnob01, float pmRateKnob01, float ringModKnob01,
                  float sampleRate)
    {
        const float phaseIncrement = PitchToPhaseIncrement(pitchKnob01, sampleRate);
        // :741-743 -- the depth multiply is at the caller of
        // StepIndependentPmLfo, not inside it; ported the same way.
        const float pmOffset = kPmLfoDepth * PmDepthScale(pmKnob01) * StepPmLfo(pmRateKnob01, sampleRate);
        const float modulatedPhase = WrapPhase(carrierPhase + pmOffset);
        const float dry = EvalWaveMorph(modulatedPhase, morphKnob01);

        // Task A: this SAME VCO's own internal ring-mod carrier -- stepped
        // every sample regardless of depth (same practice as pmLfoPhase
        // above, so raising the knob never causes a phase jump), multiplied
        // against `dry` (the pre-ASR-gate wave-morph output -- this struct
        // has no ASR gate of its own; that happens later in
        // dsp::MixOscVoices/VcoAdsrState::apply, so "pre-gate" is the only
        // choice available here and the one this port makes). Blended by
        // RingModDepthScale's taper amount so the bottom of the knob is an
        // exact identity and the top is a full ring-mod product; convex
        // blend of two values already in [-1,1] (dry, and dry*carrier, whose
        // magnitude is <= |dry| since |carrier| <= 1) stays in [-1,1].
        const float ringCarrier = Sine01(ringCarrierPhase);
        ringCarrierPhase = WrapPhase(ringCarrierPhase + RingModPhaseIncrement(ringModKnob01, sampleRate));
        const float ringAmount = RingModDepthScale(ringModKnob01);
        const float output = dry * (1.0f - ringAmount) + dry * ringCarrier * ringAmount;

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
        ringCarrierPhase = 0.0f;
        overCeilingSeconds = 0.0f;
    }

    // Tasks 2.3/2.4 (Tier 1/Tier 2 recovery): all three are pure recursive
    // state (ringCarrierPhase added by the Ring Mod strict-executor packet,
    // task A1), no coefficients to exclude.
    bool StateFinite() const
    {
        return std::isfinite(carrierPhase) && std::isfinite(pmLfoPhase) && std::isfinite(ringCarrierPhase);
    }
    float StateMagnitude() const
    {
        return std::max({std::fabs(carrierPhase), std::fabs(pmLfoPhase), std::fabs(ringCarrierPhase)});
    }

private:
    synth::Color scopeColor_ = synth::Color::Cyan;
    synth::ScopeWriterHolder* scopeWriterHolder_ = nullptr;
};

}  // namespace synth_froggers::dsp
