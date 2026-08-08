#pragma once

// synth_froggers::dsp::FiniteOnly / synth_froggers::dsp::Magnitude -- F3.3
// SPEC CORRECTED 2026-08-07 (openspec/changes/
// archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md): the two
// per-unit fault-
// recovery tiers app/FroggersAppCore.hpp's RecoverUnitIfNeeded()/
// RecoverIfNonFinite() implement -- Tier 1 (finiteness only) and Tier 2
// (finiteness, then a sustained-over-ceiling magnitude watch). Every
// dsp:: struct that participates in recovery declares which tier IT gets
// at the single call site that enumerates it (each struct's own
// `ForEachStatefulUnit`, dsp/FilterFx.hpp's FilterFxChain, dsp/Drive.hpp's
// FrogBlock, FroggersAppCore.hpp's own composed walk) by passing one of
// these two tag TYPES, never by testing the unit's own interface (e.g.
// `if constexpr (requires { unit.StateMagnitude(); })`). Inferring would
// silently reclassify a unit's fault recovery the moment a read-only
// diagnostic is added to it -- exactly what happened when F3.1 added
// StateMagnitude() to dsp::StereoDelay/dsp::Reverb purely as a measurement
// aid: both structs' own comments require them to stay Tier-1-only (a
// BIBO-stable feedback loop can legitimately settle to a large-but-finite
// steady state under sustained loud input, so Tier 2's ceiling would
// misfire on them), and an interface-inferred tier would have promoted
// them to Tier 2 the instant that diagnostic landed.
//
// TYPES, not an enum: the tier must be resolvable at COMPILE time so
// `if constexpr` can guard which of RecoverUnitIfNeeded()/RecoverIfNonFinite()
// actually gets instantiated for a given unit -- a runtime `enum class`
// value read through a plain `if` does not prevent instantiation, so
// RecoverUnitIfNeeded() (which requires `unit.StateMagnitude()`) would still
// get type-checked against every FiniteOnly-tagged unit's type, including
// ones (dsp::OutputLimiter) that never defined that method. The first
// attempt at this task used a runtime `enum class Recovery` for exactly
// that reason and six of ten test binaries failed to compile as a result;
// see this task's own SPEC CORRECTED note in tasks.md for the full account.
//
// WHY THIS FILE, NOT dsp/FilterFx.hpp: FilterFx.hpp's own header comment
// declares it a *port* -- "a **copy** (design D3) of the cited Froggers
// formulas" -- of frozen firmware DSP. These two tag types are a general-
// purpose recovery-policy vocabulary shared by FilterFx.hpp's own
// FilterFxChain, Drive.hpp's FrogBlock, and FroggersAppCore.hpp -- not a
// ported filter formula. Same reasoning dsp/Limiter.hpp's own header
// comment gives for why OutputLimiter was extracted out of FilterFx.hpp
// ("out of a file whose own header comment scopes it to comb/peak/scoop
// routing"); this file follows that precedent rather than re-arguing it.

namespace synth_froggers::dsp {

struct FiniteOnly {};  // Tier 1: reset only if the unit's state has gone non-finite.
struct Magnitude {};   // Tier 2: Tier 1, plus reset after a sustained over-ceiling magnitude.

}  // namespace synth_froggers::dsp
