#pragma once

// synth_froggers::{FroggersModulationSlate, FroggersModulationDrillIn,
// RandomizeAll, RandomizePage, ApplyFroggersDefaultPatch} -- packet 6 of the
// froggers-sheaf-app change (openspec/changes/froggers-sheaf-app/tasks.md,
// section "6. Modulation slate + drill-in", tasks 6.1-6.12; design D5, D8,
// D8a, D9b, D14, D16). Strict-executor scope: ONLY tasks 6.1-6.12. Section
// "6a. Audio routing" (parameters -> DSP -> stereo output) is a DIFFERENT,
// later task and is NOT touched here -- the VCO/EF DSP stepped below exists
// solely to produce the six modulation-source values design D5 assigns to
// slots 6-11; none of it is summed to FroggersApp::ProcessBlock's stereo
// output bus yet (that stays task 2.1's silent placeholder until 6a wires
// it). Packet 3's DSP port (app/dsp/*.hpp) is reused as-is; nothing here
// re-derives a Froggers formula.
//
// ============================================================================
// 6.1/6.2 -- registration, in design D5's load-bearing order
// ============================================================================
// `StandardModulators` is NOT used (D5, revised): `kRandomCount = 4` is a
// hard `static constexpr` (include/synth/StandardModulators.hpp:27) with four
// hardcoded visualizers (:46-49,188-191), so it cannot yield six S&H sources,
// and with slots 0-5 already taken by the six Random S&H sources there is
// nowhere left to put its four randoms anyway. All 15 sources are registered
// directly via `Modulators::SetModulationSource`
// (src/ParameterModulation.cpp:552-572, reached here through
// `ParameterGroup::SetModulationSource`, the same public wrapper). Standalone
// Sheaf pieces reused: `NoiseModulatorProcessor` (DspNoise.hpp:48-54),
// `NoiseWaveformVisualizer`, `GangedRandomLfoVisualizer` (:247-248).
//
// Slot order (design D5, task 6.2) -- SetModulationSource bounds-checks only
// and has no reservation registry, so registration order is load-bearing;
// each slot below is registered by its own explicit call (never a loop that
// could silently reorder), and task 6.6's test asserts all 15 by identity:
//   0-5   Random S&H 1-6 (5 from the ported RandomShLane, #6 a Sheaf
//         GangedRandomLfoProcessor<1> -- design D8)
//   6-8   VCO 1/2/3 audio out
//   9-11  VCO 1/2/3 envelope follower
//   12    Noise
//   13-14 external audio rate, external audio envelope follower
//
// ============================================================================
// Clock/rate wiring (packet 8, tasks.md section "8. Marbles: clock +
// visualizer", tasks 8.1/8.3; design D8/D8a) -- IMPLEMENTED here.
// ============================================================================
// The five RandomShLane sources' `Increment()` (advance to a new held value)
// is now driven by MasterClock ticks: `FroggersApp::ProcessBlock` computes
// the transport quarter-note position ONCE per sample via its own
// `TransportQuarterNotesAt()` helper (task 6b's clock-read helper, reused
// here rather than re-derived -- see task 8.1's own note below) and passes
// that same `std::optional<double>` into `Step()`, which runs each of the
// five sources' own `synth::Phasor2Tick` (multiplier 1/2/3/1, source #5
// pre-scaling `time` by 1/4 with multiplier 1 -- design D8a's rate table)
// and calls `Increment()` on a tick. Source #6 is NOT tick-driven (design
// D8): `PrepareBlockClock()`, called once per block from
// `FroggersApp::ProcessBlock` BEFORE the per-sample loop, recomputes its
// `GangedRandomLfoInput` from `block.clockPlan->QuarterNotesPerSample()` so
// one full move-cycle spans 16 quarter notes (four bars, D8a), clamping to
// a safe fallback (120 BPM-equivalent) when no clock plan is available so
// `GangedRandomLfoProcessor::Process`'s `ValidateRandomTimingConfig` never
// throws (`DspRandomLfo.hpp:26-30`).
//
// ============================================================================
// Source-value convention: normalized [0,1], matching Sheaf's own modulator
// sources
// ============================================================================
// Verified, not invented: `GangedRandomLfoProcessor`'s targets are drawn via
// `DefaultRandomDrawSource::Uniform01()` (DspRandomLfo.hpp:277-279,
// std::uniform_real_distribution<float>{0.0f,1.0f}) and
// `NoiseModulatorProcessor::Process()` writes `random_.UniformOpen01()`
// (DspNoise.hpp:69-71) -- both already [0,1]. `apps/braid-4/Braid4Core.hpp`
// renormalizes its own bipolar audio/LFO sources into [0,1] before
// registering them (`NormalizeMatrixOutput`, `0.5 + 0.5*clamp(x,-1,1)`,
// :420-422,367-378) rather than passing raw bipolar signal through. This
// port follows the same convention: `RandomShLane::Process()` and
// `VcoEnvelopeFollowers`/`SingleEnvelopeFollower` are already [0,1] (packet
// 3), and the two genuinely bipolar signals here (VCO audio, external audio)
// are renormalized with the identical `0.5 + 0.5*clamp(x,-1,1)` formula
// below (`NormalizeBipolarToUnit`).
//
// ============================================================================
// 6.4 -- drill-in level cap (design D5, resolved)
// ============================================================================
// Sheaf's `Bank` has no level concept at all: one `Parameter* selected_`
// (ParameterModulation.hpp:656) plus one bool computed from it
// (`ShowingModulation()`, :2710-2712); `Bank::HandlePress` opens a
// modulation view for ANY non-selected pressed cell regardless of how deep
// the current view already is (`OpenModulationView`, :2813-2859), so it will
// happily descend to a third, fourth, ... level. `FroggersModulationDrillIn`
// below is the app-side level counter (0 = parameter grid, 1 = level-1
// mod-detail grid, 2 = level-2 depth grid) that refuses to forward a press
// that would open a third level, and otherwise defers entirely to Bank's
// native behavior for PRESSES (including `Deselect()`'s own
// full-exit-from-any-level semantics, unchanged and un-changeable -- it is
// Sheaf's, not this app's). REVISED by E.2 (design A7a, operator override
// 2026-07-29): `FroggersModulationDrillIn::Back()` no longer simply forwards
// that raw `Deselect()` call for every level -- from level 2 it now
// synthesizes a one-level pop app-side (`Deselect()` then re-press the
// remembered level-1 encoder id), landing back on the level-1 view instead
// of a full exit. See `Back()`'s own comment for the exact mechanism.
//
// ============================================================================
// 6.7-6.11 -- randomize (design D14): Sheaf remains the only mutator
// ============================================================================
// `Bank::RandomizeModulationDepths` and the `Modifier::Random`/`RandomMod`
// dispatch inside `Bank::HandlePress` (src/ParameterModulation.cpp:2633-2650,
// :2861-2913) are PRIVATE to `Bank` -- there is no public API to invoke them
// on an arbitrary parameter directly. The sanctioned public path (also how
// `ParameterManager::SelectBankForSlot`/`NavigateBankForSlot` themselves
// trigger a bank-wide randomize, :3490-3532) is: hold the desired modifier
// (`ParameterManager::SetRandomHeld`/`SetRandomModHeld`, both public), then
// dispatch a press at the target cell; `Bank::HandlePress`'s modifier branch
// (:2633-2642) applies `Modifier::Random`/`RandomMod` to that ONE cell's
// parameter and returns before touching `selected_`/level state at all. This
// file drives exactly that mechanism, cell by cell, so the app "chooses the
// target set" (which cells get pressed, and in what view) while Sheaf
// performs every mutation, per D14/the amended resolved-decision 14. Nothing
// here reimplements `RandomizeModulationDepths`'s coin-flip loop or
// `RandomizeVisibleValue`.
//
// `Bank::ApplyModifierToTopLevel` (public) is deliberately NOT used for the
// parameter-page cases below: it applies to every cell in `topLevel_`
// unconditionally, including the shared Crunchy at slot 15, and D14 requires
// Crunchy excluded. Cell-by-cell press dispatch (via
// `FroggersModulationDrillIn::PressEncoder`, which never presses slot 15)
// is what makes the exclusion possible without touching Bank's private
// surface.

#include "FroggersParameters.hpp"
#include "FroggersRandomShVisualizer.hpp"
#include "dsp/EnvelopeFollowers.hpp"
#include "dsp/RandomShLane.hpp"
#include "dsp/Vco.hpp"

#include "synth/Color.hpp"
#include "synth/DspNoise.hpp"
#include "synth/DspPhasor2Tick.hpp"
#include "synth/DspRandomLfo.hpp"
#include "synth/GangedRandomLfoVisualizer.hpp"
#include "synth/NoiseWaveformVisualizer.hpp"
#include "synth/ParameterModulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace synth_froggers {

// Design D5's slate order, named for readability at call sites; values are
// the literal slot indices task 6.2 specifies.
enum FroggersModulatorSlot : std::size_t {
    kModSlotRandomSh1 = 0,
    kModSlotRandomSh2 = 1,
    kModSlotRandomSh3 = 2,
    kModSlotRandomSh4 = 3,
    kModSlotRandomSh5 = 4,
    kModSlotRandomSh6 = 5,
    kModSlotVco1Audio = 6,
    kModSlotVco2Audio = 7,
    kModSlotVco3Audio = 8,
    kModSlotVco1Ef = 9,
    kModSlotVco2Ef = 10,
    kModSlotVco3Ef = 11,
    kModSlotNoise = 12,
    kModSlotExternalAudio = 13,
    kModSlotExternalAudioEf = 14,
};

inline constexpr std::size_t kFroggersNumRandomShLanes = 5;  // sources 1-5; #6 is the GangedRandomLfo

// 0.5 + 0.5*clamp(x,-1,1): the same renormalization apps/braid-4 uses for its
// own bipolar audio/LFO modulation sources (Braid4Core.hpp:420-422), so a
// genuinely-bipolar signal (VCO audio, external audio) matches this
// framework's established [0,1] modulator-source convention (see this file's
// header comment).
inline float NormalizeBipolarToUnit(float bipolar)
{
    const float clamped = std::min(std::max(bipolar, -1.0f), 1.0f);
    return 0.5f + 0.5f * clamped;
}

// Bank::HandlePress's single-argument overload derives its own physical
// layout from `Bank::CompactPhysicalLayout()`, which returns only the
// encoder ids this bank has actually registered a parameter at
// (topLevel_.size(), src/ParameterModulation.cpp:2915-2921) -- 11 for every
// Froggers bank (9 page params + Crispy + Crunchy; D5a's slots 9-13 are
// deliberately left empty). `Bank::OpenModulationView` requires
// `physicalLayout.size() >= numModulators + 1` = 16
// (ParameterModulation.cpp:2818-2821), so the single-arg overload throws
// ("modulation view has more modulators than slot depth positions") the
// first time any parameter here is pressed. The two-arg overload takes an
// EXPLICIT layout instead; `BankSlot::PhysicalEncoders()` (public) already
// holds the full 0-15 layout (`FroggersParameterModel::Init` adds all 16
// physical encoders to the shared slot before any bank registers a single
// parameter), so every press dispatch below uses that instead of the
// single-arg convenience overload.
inline std::span<const synth::PhysicalEncoderId> FullPhysicalLayout(synth::Bank& bank) {
    return bank.AssociatedSlot()->PhysicalEncoders();
}

// Owns the DSP behind design D5's 15 modulation sources and their
// registration. Constructed once, never moved (its `source6Visualizer_`
// member holds a reference into `gangedRandomLfo6_`'s own UiState, so this
// object's address must stay stable for its whole lifetime -- exactly the
// same convention `FroggersParameterModel`/`FroggersApp` already follow as
// plain, non-relocated members).
class FroggersModulationSlate {
public:
    FroggersModulationSlate()
        : randomShLanes_{
              dsp::lanes::MakeSource1(kRandomShSeeds[0]),
              dsp::lanes::MakeSource2(kRandomShSeeds[1]),
              dsp::lanes::MakeSource3(kRandomShSeeds[2]),
              dsp::lanes::MakeSource4(kRandomShSeeds[3]),
              dsp::lanes::MakeSource5(kRandomShSeeds[4]),
          },
          noiseProcessor_(/*voiceCount=*/1),
          noiseVisualizer_(synth::Color::White),
          source6Visualizer_(gangedRandomLfo6_.UiState()),
          // Task 8.3: the five X-style sources' own new Visualizer (design
          // D8a), one per lane, each bound to this instance's own
          // randomShLaneUiStates_[i] (populated each block by
          // PublishUiState()) -- same "constructed once, never moved"
          // convention as source6Visualizer_ above (Visualizer deletes
          // copy/move, so these cannot live in a std::array of value types;
          // see this class's own header comment on address stability).
          randomShLaneVisualizer1_(randomShLaneUiStates_[0], LaneColor(0)),
          randomShLaneVisualizer2_(randomShLaneUiStates_[1], LaneColor(1)),
          randomShLaneVisualizer3_(randomShLaneUiStates_[2], LaneColor(2)),
          randomShLaneVisualizer4_(randomShLaneUiStates_[3], LaneColor(3)),
          randomShLaneVisualizer5_(randomShLaneUiStates_[4], LaneColor(4)) {}

    FroggersModulationSlate(const FroggersModulationSlate&) = delete;
    FroggersModulationSlate& operator=(const FroggersModulationSlate&) = delete;

    // task 6.1/6.2: register all 15 sources, in D5's order, one explicit
    // call per slot.
    // Provisioned depth-parameter storage (see below): design D14 states the
    // level-1 ceiling directly (915 parameters across 61 top-level
    // parameters, 793 with external audio disconnected) plus up to 225 more
    // for ONE focused parameter's level-2 fan-out (task 6.8's 240-total
    // scope, of which 15 already count toward the 915). 915 + 225 = 1140;
    // 1200 rounds that up with headroom. This is NOT the full 15x15x61
    // level-2 matrix (13,725), which design D14 explicitly says is never
    // materialized in one pass.
    static constexpr std::size_t kDepthParameterStorageCapacity = 1200;

    // `extraDepthCapacity` defaults to the provisioning task 6.8 needs
    // (kDepthParameterStorageCapacity); tests pass a small override to
    // deterministically exercise the CanAllocate()-exhaustion / partial-
    // randomize detection (task 6.8) without needing to actually drive 900+
    // real allocations.
    void Init(synth::ParameterGroup& group, std::size_t extraDepthCapacity = kDepthParameterStorageCapacity) {
        group_ = &group;
        RegisterSources();

        // task 6.8's materialization ceiling (915 L1 depths, plus one
        // focused parameter's 225 L2 depths) exceeds packet 4's
        // kMaxParameters=64 initial batch (deliberately sized for only the
        // 61 top-level parameters, per FroggersParameterModel's own Init
        // comment: "Modulation-depth parameters (packet 6; up to 915
        // level-1 plus more at level 2) are deliberately NOT sized for here
        // -- that growth rides ParameterGroup's own storage-batch request
        // mechanism... once a later packet wires the modulation slate").
        // This IS that later packet: provision the extra capacity directly
        // (rather than relying on the async ParameterStorageBatchNeeded /
        // ParameterMessageOutBus / Engine::MessageThreadTick path, which
        // exists for hosts that pump a message thread -- this app's tests
        // construct a bare ParameterManager with no such pump running).
        group_->AddParameterStorageBatch(synth::MakeParameterStorageBatch(
            group_->Config(), group_->GestureCount(), extraDepthCapacity));
    }

    // Sample-rate-dependent setup (VCO pitch mapping needs the real rate at
    // Process()-call time, not here; EF coefficients and the GangedRandomLfo
    // lookup table are the two pieces that must be prepared once up front).
    // C1 (openspec/changes/archive/2026-08-07-frogg3rs-blowout-and-drilldown-repair/tasks.md
    // F8.1 -- found during that task's required §12 trace, not in its
    // original six-site enumeration): this method's only production caller
    // is FroggersAppCore::PrepareToPlay() (`modulation_.Prepare(sampleRate)`,
    // its very first statement), which now validates the host's sample rate
    // ONCE before any downstream use, including this call (see that
    // method's own §12 trace). So `sampleRate` here is always already
    // positive, and the `48000.0` re-guard this used to duplicate --a THIRD
    // fallback value, disagreeing with both the six 44100.0 sites and
    // Limiter's 1.0f -- is unreachable.
    void Prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        vcoEnvelopeFollowers_.SetSampleRate(static_cast<float>(sampleRate_));
        externalAudioEf_.SetSampleRate(static_cast<float>(sampleRate_));
        gangedRandomLfo6_.Prepare(sampleRate_);
    }

    struct VcoDrive {
        float pitch01 = 0.5f;
        float shape01 = 0.5f;
        float phaseMod01 = 0.0f;
    };

    // Task 8.1 (design D8/D8a): recomputes source #6's tempo-following
    // GangedRandomLfoInput. Called ONCE PER BLOCK (matching design D8's own
    // "recomputing... each block") from FroggersApp::ProcessBlock, BEFORE
    // the per-sample loop that calls Step() -- `quarterNotesPerSample` is
    // `block.clockPlan->QuarterNotesPerSample()` when a clock plan exists,
    // nullopt otherwise (a plan-independent rate, so this does not need the
    // per-sample transport-position optional Step() takes).
    void PrepareBlockClock(std::optional<double> quarterNotesPerSample) {
        // Fallback: MasterClock::kDefaultTempoBpm (120 BPM)'s quarter-note
        // duration, so the config below stays valid (Process() below throws
        // on invalid input, DspRandomLfo.hpp:26-30) even with no clock plan
        // at all (transport never started, or a rejected/zero-frame
        // callback) -- required by task 8.1's own "clamp finite and
        // positive before it reaches the audio thread."
        constexpr double kFallbackQuarterNoteSeconds = 0.5;
        double quarterNoteSeconds = kFallbackQuarterNoteSeconds;
        if (quarterNotesPerSample.has_value() && std::isfinite(*quarterNotesPerSample) &&
            *quarterNotesPerSample > 0.0 && sampleRate_ > 0.0) {
            quarterNoteSeconds = 1.0 / (*quarterNotesPerSample * sampleRate_);
        }

        // Design D8/D8a: "one move spans 16 quarter notes" (four bars).
        // Split between the "waiting" (holding still) and "moving"
        // (transitioning) phases at the SAME 2:1 ratio (and the same
        // sigma-to-mu fraction) packet 6's original fixed placeholder used
        // (waiting mu=3.0/sigma=0.5, moving mu=1.5/sigma=0.3) -- an
        // implementer choice for how the 16-QN total round splits, flagged
        // for by-ear tuning like design D8a's other constants (task 8.6),
        // not derived from any cited source.
        constexpr double kQuarterNotesPerMove = 16.0;
        const double totalRoundSeconds = kQuarterNotesPerMove * quarterNoteSeconds;
        const double waitingMuSeconds = totalRoundSeconds * (2.0 / 3.0);
        const double movingMuSeconds = totalRoundSeconds * (1.0 / 3.0);

        currentGangedLfoInput_ = synth::GangedRandomLfoInput{
            .waiting = {.muSeconds = waitingMuSeconds,
                        .sigmaSeconds = waitingMuSeconds * (0.5 / 3.0),
                        .internalSigmaHz = 0.1},
            .moving = {.muSeconds = movingMuSeconds,
                       .sigmaSeconds = movingMuSeconds * (0.3 / 1.5),
                       .internalSigmaHz = 0.15},
            .targetInternalSigma = 0.15f,
        };
    }

    // Called once per sample, BEFORE FroggersParameterModel::ProcessSample
    // (whose group_->UpdateModValues() call reads these sources' current
    // values through the pointers registered below) -- see FroggersApp's
    // ProcessBlock for the call order.
    //
    // `vco1`/`vco2`/`vco3` are each VCO's OWN Audio-bank pitch/shape/PM
    // knobs (post-fuego cached values from the PREVIOUS sample, since this
    // Step() runs before the current sample's fuego/modulation resolve --
    // see the class-level comment on the one-sample latency this implies
    // for any parameter a VCO-audio source itself modulates, e.g. design
    // D16's cross-VCO pitch default). Zero cross-VCO terms (design D7): each
    // Vco::Process call only ever reads its OWN three knobs.
    void Step(const VcoDrive& vco1, const VcoDrive& vco2, const VcoDrive& vco3,
              std::optional<double> transportQuarterNotes) {
        // Task 8.1 (design D8/D8a): tick each of the five X-style lanes at
        // its own rate against the SAME already-computed transport
        // quarter-note position FroggersApp::ProcessBlock's own
        // TransportQuarterNotesAt() helper produced for the ASR gate (task
        // 6b) -- reusing that one value here, rather than re-deriving the
        // null-check/guard/Try-call sequence a second time, is exactly what
        // task 6b.3 asks packet 8 to do. `Phasor2Tick::Input` rejects
        // `multiplier <= 0` (DspPhasor2Tick.hpp:17-22), so rate
        // MULTIPLICATIONS use `multiplier` (sources 1/4 = 1, source 2 = 2,
        // source 3 = 3) and the one rate DIVISION (source 5, once per bar =
        // 4 quarter notes) pre-scales `time` by 1/4 with multiplier = 1.
        StepClockDrivenLanes(transportQuarterNotes);
        for (std::size_t i = 0; i < kFroggersNumRandomShLanes; ++i) {
            randomShLaneOutputs_[i] = randomShLanes_[i].Process();
        }

        // Random S&H 6 (Y-style, design D8's GangedRandomLfoProcessor<1>
        // mechanism resolution). `currentGangedLfoInput_` is recomputed once
        // per block by PrepareBlockClock() (task 8.1) so this source is
        // tempo-proportional rather than tick-driven (design D8: NOT
        // phase-locked to the quarter-note grid).
        gangedRandomLfo6_.Process(currentGangedLfoInput_);
        randomSh6Output_ = gangedRandomLfo6_.Output(0);

        // VCO audio (slots 6-8): each VCO stepped from only its own knobs.
        const float sr = static_cast<float>(sampleRate_);
        const float vco1Raw = vco1_.Process(vco1.pitch01, vco1.shape01, vco1.phaseMod01, sr);
        const float vco2Raw = vco2_.Process(vco2.pitch01, vco2.shape01, vco2.phaseMod01, sr);
        const float vco3Raw = vco3_.Process(vco3.pitch01, vco3.shape01, vco3.phaseMod01, sr);
        vco1AudioSource_ = NormalizeBipolarToUnit(vco1Raw);
        vco2AudioSource_ = NormalizeBipolarToUnit(vco2Raw);
        vco3AudioSource_ = NormalizeBipolarToUnit(vco3Raw);

        // VCO envelope followers (slots 9-11); already [0,1] (packet 3).
        float efOut[dsp::VcoEnvelopeFollowers::kNumTaps];
        vcoEnvelopeFollowers_.Process(vco1Raw, vco2Raw, vco3Raw, efOut);
        vco1EfSource_ = efOut[0];
        vco2EfSource_ = efOut[1];
        vco3EfSource_ = efOut[2];

        // Noise (slot 12); already [0,1] (DspNoise.hpp).
        noiseProcessor_.Process();

        // External audio (slots 13-14) -- deliberately NOT stepped here.
        // T6 (openspec/changes/archive/2026-08-10-frogg3rs-external-audio-phantom-input/
        // tasks.md): this used to recompute externalAudioSource_ /
        // externalAudioEfSource_ from an `externalAudioSample` parameter and
        // call SetExternalAudioConnected(externalInputConnected) every
        // sample, but FroggersAppCore::ProcessBlock's only production caller
        // fed a compile-time `0.0f`/`false` (Config() now requests zero
        // audio input channels -- see that class's own comment), so this
        // was recomputing the same two constants ~96,000x/second.
        // externalAudioSource_/externalAudioEfSource_ (below) stay at their
        // NSDMI defaults forever now: 0.5f == NormalizeBipolarToUnit(0.0f)
        // exactly, and 0.0f == what a zero-initialized SingleEnvelopeFollower
        // fed a constant 0.0f holds forever (target <= level from sample
        // one, so `level` never moves off its own 0.0f default --
        // dsp/EnvelopeFollowers.hpp's Process()) -- bit-identical to what
        // this used to compute every sample, and still a defined, finite
        // value for Modulators::UpdateModValues() to dereference regardless
        // (design D5's "present but inert" contract). `.connected` likewise
        // stays whatever RegisterSources() set (false, below) -- nothing
        // here calls SetExternalAudioConnected() anymore. That method is
        // KEPT, unchanged, as the writer a future re-enable will need (see
        // FroggersAppCore.hpp's Config()/ProcessBlock comments for what
        // re-enabling requires); a caller that wants connected=true today
        // (e.g. a test) calls it directly instead of through Step() --
        // see FroggersModulationTests.cpp's Fixture::StepOnce.
    }

    // task 6.5: "Flip connected back to true when an input appears" -- the
    // cell stays pushed with a null parameter whenever this is false
    // (Sheaf's own OpenModulationView/EnsureModulationDepthParameter
    // behavior, ParameterModulation.cpp:2784-2786,2843-2852), so the slate
    // never changes size or order regardless of cabling. T6: no longer
    // called per-sample from Step() (production never had a varying value
    // to feed it); kept as the standalone writer a real re-enable
    // derivation, or a test wanting connected=true, calls explicitly.
    void SetExternalAudioConnected(bool connected) {
        group_->GetModulators().Metadata(kModSlotExternalAudio).connected = connected;
        group_->GetModulators().Metadata(kModSlotExternalAudioEf).connected = connected;
    }

    bool ExternalAudioConnected() const {
        return group_->GetModulators().Metadata(kModSlotExternalAudio).connected;
    }

    // Task 8.3: publishes this block's visualizer-facing state -- the five
    // X-style lanes' UiState (task 8.4's "visualizer state matches the
    // bag") and source #6's GangedRandomLfo UiState (its own PublishUiState,
    // the same call apps/braid-4 and apps/miniapp make once per block for
    // their own StandardModulators instances). Called once per block from
    // FroggersApp::ProcessBlock, AFTER the per-sample loop (mirroring
    // Braid4Core::ProcessBlock's own end-of-block
    // scopeWriter_.Publish()/PopulateUIState()/PublishUiState() sequence).
    void PublishUiState() {
        for (std::size_t i = 0; i < kFroggersNumRandomShLanes; ++i) {
            randomShLanes_[i].PopulateUiState(randomShLaneUiStates_[i]);
        }
        gangedRandomLfo6_.PublishUiState();
    }

    // Test/inspection accessors (also usable by a later packet's UI wiring).
    synth::ParameterGroup& Group() { return *group_; }
    float SourceValue(std::size_t modIx) const { return group_->GetModulators().Value(0, modIx); }
    const synth::ModulatorMetadata& Metadata(std::size_t modIx) const {
        return group_->GetModulators().Metadata(modIx);
    }
    // Task 8.4: reflects randomShLanes_[laneIx]'s current index/bag as of the
    // last PublishUiState() call -- lets a test observe lane advance without
    // reaching into this class's private DSP state.
    const dsp::RandomShLane::UiState& RandomShLaneUiState(std::size_t laneIx) const {
        return randomShLaneUiStates_.at(laneIx);
    }
    // Task 8.4: "source #6 is verified as tempo-proportional rather than
    // counted in ticks" -- exposes PrepareBlockClock()'s own recomputed
    // config directly, so a test can check the muSeconds/tempo relationship
    // by formula rather than statistically waiting for real LFO rounds to
    // complete (which involves random normal draws and would be slow/flaky).
    const synth::GangedRandomLfoInput& CurrentGangedLfoInputForTest() const { return currentGangedLfoInput_; }

private:
    // Task 8.1: one shared per-sample tick call, so the 8.4 test suite can
    // exercise this without wiring up a real MasterClock/AudioBlock (a bare
    // std::optional<double> is exactly what FroggersApp::ProcessBlock's own
    // TransportQuarterNotesAt() helper already returns).
    void StepClockDrivenLanes(std::optional<double> transportQuarterNotes) {
        auto tickLane = [&](synth::Phasor2Tick& ticker, dsp::RandomShLane& lane, double time, int multiplier) {
            if (!transportQuarterNotes.has_value()) {
                return;  // transport not running / no clock plan: no tick, gate-closed-equivalent.
            }
            const synth::Phasor2Tick::Input input{time, multiplier};
            if (ticker.Process(input)) {
                lane.Increment();
            }
        };

        const double quarterNotes = transportQuarterNotes.value_or(0.0);
        // Design D8a's rate table: #1 quarter note (x1), #2 eighth (x2),
        // #3 eighth triplet (x3), #4 quarter note (x1, free-running
        // character differs via the lane's own construction, task 3.4), #5
        // once per bar (QN/4, x1 -- a DIVISION, so `time` is pre-scaled
        // rather than using `multiplier`, per Phasor2Tick's own
        // `multiplier <= 0` rejection, DspPhasor2Tick.hpp:17-22).
        tickLane(tick1_, randomShLanes_[0], quarterNotes, 1);
        tickLane(tick2_, randomShLanes_[1], quarterNotes, 2);
        tickLane(tick3_, randomShLanes_[2], quarterNotes, 3);
        tickLane(tick4_, randomShLanes_[3], quarterNotes, 1);
        tickLane(tick5_, randomShLanes_[4], quarterNotes / 4.0, 1);
    }

    // Task 8.3: per-lane visualizer colors, shared between RegisterSources()
    // (ModulatorMetadata::sourceColor) and the constructor's visualizer
    // initializer list -- kept as one formula so the two never drift apart.
    static synth::Color LaneColor(std::size_t i) {
        return synth::Color::FromHsvDegrees(180.0f + static_cast<float>(i) * 30.0f, 0.65f, 0.95f);
    }

    void RegisterSources() {
        // task 6.3: source colors. No v2 "modulation source palette" file
        // exists for this all-new 15-source set (v2's own mod sources are
        // largely superseded by D5 -- see design D5's "superseded by this
        // slate" note), so these are an implementer default: one hue family
        // per source category, spaced for visual distinction, flagged here
        // (and in the packet report) as not verified against a specific v2
        // palette table.
        // Task 8.3: each X-style lane's OWN new Visualizer (design D8a),
        // constructed once alongside this instance (see the constructor's
        // own comment) -- replaces the packet-6 placeholder `nullptr`.
        std::array<synth::ui::Visualizer*, kFroggersNumRandomShLanes> randomShLaneVisualizers{
            &randomShLaneVisualizer1_, &randomShLaneVisualizer2_, &randomShLaneVisualizer3_,
            &randomShLaneVisualizer4_, &randomShLaneVisualizer5_,
        };
        for (std::size_t i = 0; i < kFroggersNumRandomShLanes; ++i) {
            const std::array<float*, 1> src{&randomShLaneOutputs_[i]};
            group_->SetModulationSource(i, src,
                {kRandomShNames[i], kRandomShShortNames[i], LaneColor(i), randomShLaneVisualizers[i],
                 /*connected=*/true});
        }
        {
            // ITEM 2b: mirrors Sheaf's own StandardModulators::Init, which
            // calls SetVoiceColor on every GangedRandomLfoProcessor it owns
            // before registering it (StandardModulators.hpp:126-130).
            // gangedRandomLfo6_ is this app's own standalone instance (not
            // one of StandardModulators' processors, since D8 resolves
            // source #6 outside that shared machinery), so nothing else
            // ever calls this -- without it, its one voice's UiState color
            // (what GangedRandomLfoVisualizer actually draws,
            // GangedRandomLfoVisualizer.hpp's `voice.color`) stays at
            // GangedRandomLfoAtomicColor's own default, Color::Grey
            // (DspRandomLfo.hpp:165), even though the ModulatorMetadata
            // below already carries the correct LaneColor(5).
            gangedRandomLfo6_.SetVoiceColor(0, LaneColor(5));
            const std::array<float*, 1> src{&randomSh6Output_};
            group_->SetModulationSource(kModSlotRandomSh6, src,
                {"Random S&H 6", "RndSH6", LaneColor(5), &source6Visualizer_, /*connected=*/true});
        }

        {
            const std::array<float*, 1> src{&vco1AudioSource_};
            group_->SetModulationSource(kModSlotVco1Audio, src,
                {"VCO1 Audio", "V1Aud", synth::Color::Red, nullptr, true});
        }
        {
            const std::array<float*, 1> src{&vco2AudioSource_};
            group_->SetModulationSource(kModSlotVco2Audio, src,
                {"VCO2 Audio", "V2Aud", synth::Color::Orange, nullptr, true});
        }
        {
            const std::array<float*, 1> src{&vco3AudioSource_};
            group_->SetModulationSource(kModSlotVco3Audio, src,
                {"VCO3 Audio", "V3Aud", synth::Color::Yellow, nullptr, true});
        }

        {
            const std::array<float*, 1> src{&vco1EfSource_};
            group_->SetModulationSource(kModSlotVco1Ef, src,
                {"VCO1 EF", "V1EF", synth::Color::Red.AdjustBrightness(0.55f), nullptr, true});
        }
        {
            const std::array<float*, 1> src{&vco2EfSource_};
            group_->SetModulationSource(kModSlotVco2Ef, src,
                {"VCO2 EF", "V2EF", synth::Color::Orange.AdjustBrightness(0.55f), nullptr, true});
        }
        {
            const std::array<float*, 1> src{&vco3EfSource_};
            group_->SetModulationSource(kModSlotVco3Ef, src,
                {"VCO3 EF", "V3EF", synth::Color::Yellow.AdjustBrightness(0.55f), nullptr, true});
        }

        // task 6.1: reuse NoiseModulatorProcessor's own SourcePointers() span
        // directly -- it is already sized to its voiceCount (1), matching
        // this mono group's numVoices, so no manual pointer wiring is needed.
        group_->SetModulationSource(kModSlotNoise, noiseProcessor_.SourcePointers(),
            {"Noise", "Noise", synth::Color::White, &noiseVisualizer_, true});

        // task 6.5: external audio pair registered `connected = false`
        // initially. As of frogg3rs-external-audio-phantom-input's T1/T2/T6,
        // this is not merely an initial value that Step() later refreshes --
        // `Config()` requests ZERO audio input channels, and T6 removed the
        // per-sample call that used to re-derive this every sample (see
        // Step()'s own comment), so nothing in this class calls
        // SetExternalAudioConnected() again in production. `false` set here
        // is permanent for the object's lifetime until a future re-enable
        // writes a real derivation (FroggersAppCore.hpp's Config()/
        // ProcessBlock comments say what that requires).
        {
            const std::array<float*, 1> src{&externalAudioSource_};
            group_->SetModulationSource(kModSlotExternalAudio, src,
                {"External Audio", "ExtAud", synth::Color::Green, nullptr, false});
        }
        {
            const std::array<float*, 1> src{&externalAudioEfSource_};
            group_->SetModulationSource(kModSlotExternalAudioEf, src,
                {"External Audio EF", "ExtEF", synth::Color::Green.AdjustBrightness(0.55f), nullptr, false});
        }
    }

    static constexpr std::array<const char*, kFroggersNumRandomShLanes> kRandomShNames{
        "Random S&H 1", "Random S&H 2", "Random S&H 3", "Random S&H 4", "Random S&H 5",
    };
    static constexpr std::array<const char*, kFroggersNumRandomShLanes> kRandomShShortNames{
        "RndSH1", "RndSH2", "RndSH3", "RndSH4", "RndSH5",
    };
    // Distinct per-lane seeds (D8/task 3.4: each RGen is now per-instance, so
    // distinct seeds are what actually gives five independent streams).
    static constexpr std::array<uint32_t, kFroggersNumRandomShLanes> kRandomShSeeds{
        0x1a2b3c4du, 0x2b3c4d5eu, 0x3c4d5e6fu, 0x4d5e6f70u, 0x5e6f7081u,
    };

    synth::ParameterGroup* group_ = nullptr;
    double sampleRate_ = 48000.0;

    dsp::Vco vco1_;
    dsp::Vco vco2_;
    dsp::Vco vco3_;
    dsp::VcoEnvelopeFollowers vcoEnvelopeFollowers_;
    // T6: .Process() is no longer called (Step() no longer steps external
    // audio -- see that method's own comment); KEPT, along with its
    // SetSampleRate() call in Prepare() below, as re-enable infrastructure
    // rather than removed -- neither does per-sample work, so neither was
    // what the T6 finding was about, and deleting them would leave a future
    // re-enable to rediscover the correct attack/release coefficients
    // (SetSampleRate's coefficients are sample-rate-dependent; the struct's
    // own raw defaults are only valid near ~2kHz, not 48kHz) instead of
    // finding them already wired.
    dsp::SingleEnvelopeFollower externalAudioEf_;

    std::array<dsp::RandomShLane, kFroggersNumRandomShLanes> randomShLanes_;
    // Task 8.1: one Phasor2Tick per X-style lane (design D8a's rate table);
    // NOT one per bank/parameter -- each lane owns exactly its own ticker,
    // matching the "each source carries a fixed character" framing (D8).
    synth::Phasor2Tick tick1_;
    synth::Phasor2Tick tick2_;
    synth::Phasor2Tick tick3_;
    synth::Phasor2Tick tick4_;
    synth::Phasor2Tick tick5_;
    // Task 8.3: visualizer-facing published state for the five lanes above
    // (declared before the Visualizer members below, which each hold a
    // pointer into one element -- see this class's own "constructed once,
    // never moved" address-stability convention).
    std::array<dsp::RandomShLane::UiState, kFroggersNumRandomShLanes> randomShLaneUiStates_;

    synth::GangedRandomLfoProcessor<1> gangedRandomLfo6_;
    // Task 8.1: source #6's tempo-following input, recomputed once per block
    // by PrepareBlockClock(). Default-initialized to the same shape packet
    // 6's fixed placeholder used, so a Step() call before the first
    // PrepareBlockClock() (should not happen in practice -- FroggersApp
    // calls PrepareBlockClock() before its per-sample loop every block --
    // but is a defensive, always-valid starting point, not a live musical
    // rate choice) still never throws.
    synth::GangedRandomLfoInput currentGangedLfoInput_{
        .waiting = {.muSeconds = 3.0, .sigmaSeconds = 0.5, .internalSigmaHz = 0.1},
        .moving = {.muSeconds = 1.5, .sigmaSeconds = 0.3, .internalSigmaHz = 0.15},
        .targetInternalSigma = 0.15f,
    };
    synth::NoiseModulatorProcessor noiseProcessor_;
    synth::ui::NoiseWaveformVisualizer noiseVisualizer_;
    synth::ui::GangedRandomLfoVisualizer<1> source6Visualizer_;
    // Task 8.3: the five X-style lanes' own new Visualizer (design D8a) --
    // individually-named members, not a std::array, because
    // synth::ui::Visualizer deletes copy/move (see the constructor's own
    // comment).
    RandomShLaneVisualizer randomShLaneVisualizer1_;
    RandomShLaneVisualizer randomShLaneVisualizer2_;
    RandomShLaneVisualizer randomShLaneVisualizer3_;
    RandomShLaneVisualizer randomShLaneVisualizer4_;
    RandomShLaneVisualizer randomShLaneVisualizer5_;

    float randomShLaneOutputs_[kFroggersNumRandomShLanes]{0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    float randomSh6Output_ = 0.5f;
    float vco1AudioSource_ = 0.5f;
    float vco2AudioSource_ = 0.5f;
    float vco3AudioSource_ = 0.5f;
    float vco1EfSource_ = 0.0f;
    float vco2EfSource_ = 0.0f;
    float vco3EfSource_ = 0.0f;
    // T6: these two NSDMI values are now permanent, not merely initial --
    // Step() no longer writes either (see its own comment). Both values are
    // exactly what the removed per-sample computation always produced from
    // its permanently-0.0f input, so this is not an observable behavior
    // change.
    float externalAudioSource_ = 0.5f;
    float externalAudioEfSource_ = 0.0f;
};

// ============================================================================
// task 6.4/6.6 -- drill-in level cap
// ============================================================================
class FroggersModulationDrillIn {
public:
    // F5.1: the ONE definition site of the drill-in maximum. PUBLIC because
    // `RandomizeAll` below reads it to decide whether a deeper level exists to
    // descend into -- it must not re-declare the number (OMNI §8), and it is
    // the same fact this class already enforces in `PressEncoder`.
    static constexpr std::size_t kMaxDrillLevel = 3;   // F5.3: raised from 2, keeping the base-3 theme

    explicit FroggersModulationDrillIn(synth::Bank& bank) : bank_(&bank) {}

    std::size_t Level() const { return level_; }
    synth::Bank& BankRef() { return *bank_; }

    // task 6.4: the app-side level counter. Levels below the cap are Sheaf's
    // native behavior (no app code); the ONLY thing added is refusing to
    // dispatch a press that would open one level deeper than the cap.
    void PressEncoder(synth::PhysicalEncoderId encoderId) {
        if (level_ >= kMaxDrillLevel) {
            // Level cap: only forward a press on the Target/Back cell
            // (the current selection's own cell) -- anything else would
            // call OpenModulationView for THIS level's depth cell's own
            // depths, i.e. one level past the cap, so it is refused: no
            // dispatch at all.
            if (bank_->SelectedParameter() == nullptr ||
                bank_->VisibleParameter(encoderId) != bank_->SelectedParameter()) {
                return;
            }
        }
        synth::Parameter* const selectedBefore = bank_->SelectedParameter();
        // S5.1 (operator regression, 2026-08-07 -- "the drilldown back
        // button still doesn't go one back, it goes all the way back"):
        // capture, BEFORE dispatching the press, whether `encoderId` is the
        // Target/Back cell -- i.e. whether the pressed cell's own visible
        // parameter is the parameter already selected. This MUST be read
        // now, not after HandlePress() returns: a Target/Back press calls
        // Deselect() (below), which resets `visible_` to `topLevel_`, so
        // VisibleParameter(encoderId) would no longer reflect the
        // pre-press view afterward.
        //
        // This is the exact, and ONLY, distinguishing signal, verified by
        // reading Sheaf's complete Bank::HandlePress body (External/Sheaf/
        // projects/synth/src/ParameterModulation.cpp:2628-2650, pinned):
        // its one and only branch that sets `selected_` to nullptr is
        // `if (ShowingModulation() && cell->parameter == selected_) {
        // Deselect(); return; }` (:2643-2646) -- OpenModulationView
        // (:2813-2859) never nulls it, it either assigns a real parameter
        // or returns early on a storage shortfall leaving `selected_`
        // unchanged. `ShowingModulation()` is `selected_ != nullptr`
        // (:2710-2712), i.e. `selectedBefore != nullptr` here, and
        // `cell->parameter` for the pressed encoderId is exactly what
        // `bank_->VisibleParameter(encoderId)` reads (:2718-2721, same
        // FindVisibleCell lookup). So, given `selectedBefore != nullptr`,
        // HandlePress leaves `selectedAfter == nullptr` if and only if
        // `wasTargetBackPress` below is true -- an exact predicate, not a
        // heuristic. This also matches OpenModulationView's own
        // construction of the Target/Back cell (:2854-2857, the selected
        // parameter's own cell is always physicalLayout.back()) at every
        // drill level alike, since FullPhysicalLayout(*bank_) is the same
        // fixed 16-wide span on every call (this file's own comment above).
        const bool wasTargetBackPress =
            selectedBefore != nullptr && bank_->VisibleParameter(encoderId) == selectedBefore;
        bank_->HandlePress(encoderId, FullPhysicalLayout(*bank_));
        synth::Parameter* const selectedAfter = bank_->SelectedParameter();
        if (selectedAfter == nullptr) {
            if (wasTargetBackPress) {
                // The operator's actual Back gesture (every on-screen press,
                // Target/Back cell included, dispatches through here, never
                // through Back() directly -- see this class's own header
                // comment). Pop exactly ONE level by calling the existing
                // Back() mechanism -- reused as-is, not duplicated (OMNI
                // §8) -- rather than the full `level_ = 0` reset below.
                // Back() reads the CURRENT `level_` to compute its target
                // depth, so this must run before `level_` is touched here;
                // it leaves `level_` at that one-shallower depth itself.
                Back();
            } else {
                // Genuine full clear: `selectedBefore` was already nullptr
                // (level 0, e.g. an empty-cell press), so `wasTargetBackPress`
                // is false and this is a same-value no-op -- unchanged from
                // before this fix. No path reaches this branch with
                // `wasTargetBackPress` false and `selectedBefore` non-null:
                // that would require HandlePress to null out `selected_`
                // some OTHER way, and the trace above establishes there is
                // no other way.
                level_ = 0;
            }
        } else if (selectedAfter != selectedBefore) {
            // E.2 (design A7a, operator override 2026-07-29), generalised by
            // F5.1 from a single remembered level-1 encoder to one per level:
            // remember the encoder id that opened THIS descent, so Back() can
            // re-open the same path one level at a time (Sheaf's Bank has no
            // one-level pop of its own -- Deselect() is always a full exit --
            // so the app synthesizes one by Deselect()-then-replaying the
            // remembered presses; see Back() below). `levelEncoders_[i]` is
            // therefore the encoder id that opens level i+1, which is why
            // this is recorded at index `level_` BEFORE the increment below.
            levelEncoders_[level_] = encoderId;
            level_ += 1;
        }
        // else: selection identity unchanged -- either a held modifier was
        // applied to the pressed cell (Bank::HandlePress's modifier branch
        // returns before touching selected_/level state), or the press hit
        // an empty cell. Level stays exactly where it was either way.
    }

    // task 6.4, REVISED by E.2 (design A7a, operator override 2026-07-29):
    // "Back exits to the parameter grid from any level" is no longer true --
    // Back pops exactly ONE level, from any depth. Sheaf's Bank::Deselect()
    // is still a full, unconditional exit (no Sheaf change, no one-level pop
    // added there), so the one-level pop is synthesized app-side: Deselect()
    // all the way out, then replay the remembered presses (`levelEncoders_`,
    // set by PressEncoder above) that walk back down to exactly one level
    // short of where Back() was called. F5.1 generalised this from a single
    // remembered level-1 encoder plus a level-2 special case to one
    // mechanism at any depth. `target` is captured BEFORE `level_` is reset
    // to 0 below, and uses the `(level_ == 0) ? 0 : level_ - 1` form because
    // `level_ - 1` on an unsigned zero would wrap instead of staying at 0.
    void Back() {
        const std::size_t target = (level_ == 0) ? 0 : level_ - 1;
        bank_->Deselect();
        level_ = 0;
        for (std::size_t i = 0; i < target; ++i) {
            PressEncoder(levelEncoders_[i]);
        }
    }

private:
    synth::Bank* bank_;
    std::size_t level_ = 0;
    // E.2 (design A7a), generalised by F5.1: one remembered encoder id per
    // level of the current drill-in path -- `levelEncoders_[i]` is the
    // encoder id that opens level i+1. See PressEncoder's own comment for
    // when each entry is recorded, and Back() for how they are replayed.
    std::array<synth::PhysicalEncoderId, kMaxDrillLevel> levelEncoders_{};
};

// ============================================================================
// task 6.7-6.11 -- randomize (design D14)
// ============================================================================
// E.1 (design A6, 2026-07-29) -- REVISED from task 6.10's original note
// below, which documented a bug: every "randomize this parameter's depths"
// operation used to route through a held Modifier::RandomMod press into
// Bank::HandlePress, which calls the PRIVATE Bank::RandomizeModulationDepths
// (src/ParameterModulation.cpp:2881-2913). That function's own loop --
//     while (manager_->NextRandomCoin() < 0.5f) { ... one depth touched ... }
// (:2894) -- is a geometric distribution over the count of depths touched
// STARTING AT ZERO: P(k) = 0.5^(k+1) for k = 0, 1, 2, ..., mean 1.0. A single
// press typically changed only ~1 depth, and was a complete no-op exactly
// 50% of the time. That constant is Sheaf's, private, and out of scope to
// change upstream (src/ParameterModulation.cpp:2894).
//
// `detail::RandomizeParameterModulationDepths` below (used by all four call
// sites: RandomizeBankLevel1Depths, RandomizePage's drill-in branch, and
// both RandomizeAll drill-in branches) replaces this with an APP-SIDE count/
// source selection -- an app-owned weighted table, currently F1.2's mode-2
// shape (frogg3rs-blowout-and-drilldown-repair, 2026-08-07; supersedes A6's
// original median-3 10/30/30/20 -- see that function's own table comment for
// the exact numbers) -- while Sheaf
// still performs every actual write (`Parameter::EnsureModulationDepth` +
// `Parameter::RandomizeVisibleValue`, the same two calls
// `Bank::RandomizeModulationDepths` itself makes internally). This is design
// D14's "app chooses the target set, Sheaf does every write" split, not a
// violation of it -- see that function's own header comment for the full
// derivation. NEVER a no-op (at least 1 connected source is always touched
// when one exists), and never re-draws the same source twice (Sheaf's own
// loop could; this one uses a partial Fisher-Yates over the connected set).
//
// The OTHER knob this app owns, unchanged by E.1: Randomize All's aggregate
// reach also comes from how many PARAMETERS it presses (61 for the
// parameter-page case, task 6.8) -- a future maintainer who wants Randomize
// All to feel like "more" or "fewer" changes has two independent levers now
// (the per-parameter count table above, or the parameter set
// RandomizeAll/RandomizeBankLevel1Depths iterates over).
struct FroggersRandomizeResult {
    // task 6.8: EnsureModulationDepthParameter's CanAllocate() failure must
    // surface as a detectable partial randomize, not fail silently. True
    // when `ParameterGroup::CanAllocate()` (public) was observed false
    // immediately before a press that could have needed to materialize a
    // new depth parameter during this operation.
    bool partial = false;
};

namespace detail {

// The only reliable externally-observable signal that
// Bank::EnsureModulationDepthParameter's private CanAllocate() early return
// (ParameterModulation.cpp:2790-2792) is ABOUT to fire for the next press:
// `ParameterGroup::CanAllocate()` (public) is already false. Checking a
// per-parameter "does every connected modulator have a materialized depth"
// count instead would be wrong -- design D14's own bias table
// (P(k)=0.5^(k+1), mean 1.0) is Sheaf's geometric loop, REPLACED on the app
// side since E.1. The distribution actually in force is this file's own
// RandomizeParameterModulationDepths draw below (F1.2's mode-2 table as of
// 2026-08-07, mean 2.25 -- see that draw's own comment for the exact
// numbers) -- and that still means a HEALTHY randomize typically leaves
// most of a parameter's 15 possible depths untouched; that is normal, not a
// partial randomize. Only "no more storage was available to give" is.
inline bool CapacityExhausted(const synth::ParameterGroup& group) {
    return !group.CanAllocate();
}

// A3 (scene-pair semantics, tasks.md's A3 decision): Randomize All/Page is
// "agnostic to scene-slider position" -- every depth write below targets one
// of these two FIXED scene poles directly, never the live (possibly
// mid-blend) `manager.Scene()`. leftScene==rightScene on each, so
// `ApplySceneDistribution`'s own `&left==&right` special case
// (ParameterModulation.cpp:369-374) applies the write to exactly that one
// scene index regardless of blend -- the same pattern
// `ApplyFroggersDefaultPatch`'s own `kScene` constant already relies on for
// pole 0 (this file, further down). Matches `FroggersParameterModel::
// kNumScenes == 2` and the app's fixed `SetSceneEndpoints(0, 1)`
// (FroggersParameters.hpp; scene index 0 = "Scene 1", 1 = "Scene 2").
inline constexpr synth::SceneState kScenePole0{0, 0, 0.0f};
inline constexpr synth::SceneState kScenePole1{1, 1, 0.0f};

// F2: the single definition site for "how many scene poles exist." Both
// ZeroExistingModulationDepths and RandomizeParameterModulationDepths below
// used to hardcode kScenePole0/kScenePole1 as two separate consecutive
// statements; both now iterate this array instead, so a future change to
// `FroggersParameterModel::kNumScenes` trips the static_assert right below
// (a build break) instead of silently leaving both call sites still
// handling exactly two poles. Size is deduced from the initializer list
// (currently 2, matching kScenePole0/kScenePole1 above) and cross-checked
// against kNumScenes, the actual authority (FroggersParameters.hpp).
inline constexpr std::array kScenePoles{kScenePole0, kScenePole1};
static_assert(kScenePoles.size() == FroggersParameterModel::kNumScenes,
              "kScenePoles must enumerate exactly kNumScenes scene poles");

// F3: the bipolar-neutral commanded value, named once. Sheaf's own
// `kNeutralModulationDepthCenter` (ParameterModulation.cpp:258) is
// file-private and not reachable from here, so this is a legitimate
// separate definition -- but it must exist exactly once on this side, not
// as a bare `0.5f` repeated at every call site (W1.0: "depths are
// RangeKind::Bipolar, 0.5 IS zero"). Referenced by
// ZeroExistingModulationDepths below and by FroggersModulationTests.cpp.
inline constexpr float kNeutralModulationDepthCenter = 0.5f;
// Matches Sheaf's own `kModulationNeutralTolerance`
// (External/Sheaf/projects/synth/src/ParameterModulation.cpp:256), which is
// the tolerance `Parameter::HasNonZeroState()` -- and therefore the badge
// criterion `ModulatorsAffectingMask()` -- uses to decide neutrality. Pinned
// to the same value deliberately: `DepthIsModulating` below has to agree with
// what the UI will actually draw, and that constant is file-local to Sheaf's
// .cpp so it cannot be referenced directly. If Sheaf's value ever moves, this
// one moves with it.
inline constexpr float kModulationNeutralEpsilon = 0.000001f;

// A1 (non-additive randomize) + A3 (scene-pair semantics): zeroes
// `parameter`'s EXISTING (already-materialized) modulation depths at BOTH
// scene poles, immediately before RandomizeParameterModulationDepths below
// draws a fresh set. Only touches depths that are already materialized --
// `ModulationDepthParameter` (unlike `EnsureModulationDepth`) never
// allocates, so an unmaterialized source (never touched, or already at
// storage capacity) is left alone rather than burning a slot on a write that
// would be a no-op anyway (an unmaterialized depth has no Parameter to carry
// a nonzero commanded value in the first place). `kNeutralModulationDepthCenter`
// is the bipolar neutral commanded value (see its own comment above).
// A raw `SceneCenter` write (not `RandomizeVisibleValue`/`HandleSetAbsolute`)
// is deliberate: it is the exact commanded-value write W1.0 identifies
// `sceneCenters_` as being, with no dependency on the depth's own
// (currently stale) `targetCenter_`, and the single `ComputeAllParameters()`
// call A2 adds at the end of the whole randomize operation resyncs
// `currentCenter_`/`targetCenter_`/the UI display for every parameter this
// touches -- including these zeroed ones -- in one pass, so nothing here
// needs to re-converge anything itself.
// Is this depth parameter ACTUALLY modulating anything -- i.e. does it carry a
// non-neutral commanded value in either scene pole?
//
// Needed because "a depth parameter EXISTS" and "a source is modulating this
// parameter" are different facts, and drilling in conflates them: opening a
// modulation view eagerly materializes one depth per CONNECTED source (13 of
// 15 on the measured patch), regardless of whether randomize selected any of
// them. RandomizeAll below must descend only into the ones that are really
// modulating, for two reasons:
//
//  1. Semantics: a neutral depth modulates nothing, so sub-modulating IT is
//     meaningless work on a value that has no audible effect.
//  2. It is directly visible. Sheaf's badge criterion is
//     `Parameter::ModulatorsAffectingMask()` (External/Sheaf/projects/synth/
//     src/ParameterModulation.cpp:2356-2365), which sets a bit when the depth
//     is non-null AND `HasNonZeroState()`. `HasNonZeroState()` (:2367) returns
//     true if the depth's OWN `currentDepths_`/`targetDepths_` are non-zero --
//     so a depth counts as "affecting" merely because it has SUB-modulation,
//     even when its own value is dead neutral. Writing sub-depths to every
//     materialized depth therefore lit up all 13 as badges when only ~3
//     sources were modulating (measured 2026-08-07: badge 13, non-neutral 1).
//     The badge was reporting CONNECTEDNESS, not modulation.
//
// `HasNonZeroState()` itself is private in Sheaf and unreachable from here, so
// this reads the commanded values the app itself writes -- the same
// `kNeutralModulationDepthCenter` / `kScenePoles` idiom
// ZeroExistingModulationDepths below uses, so the two agree by construction.
// Per-scene form: is this depth non-neutral in ONE specific scene? Separate
// from the any-scene form below because two distinct properties are asserted
// across this codebase's tests and they must not be conflated:
//   - "is this source modulating at all" (any pole) -- what the badge shows,
//     and what RandomizeAll's descent gates on;
//   - "is this source modulating in scene N" (this one) -- used to compare the
//     two poles against each other, which is W1.0's badge/depth symmetry
//     property ("randomizing only scene 0 lights the badge in both scenes
//     while scene 1 reads zero"). Expressing THAT with the any-pole form would
//     make its own assertion trivially true and silently gut the test.
// Both share this one neutral-value/epsilon comparison, so the threshold that
// decides "neutral" has exactly one definition site (OMNI §8).
inline bool DepthIsModulatingInScene(const synth::Parameter& depth, std::size_t sceneIx) {
    return std::fabs(depth.SceneCenter(sceneIx) - kNeutralModulationDepthCenter) >
           kModulationNeutralEpsilon;
}

inline bool DepthIsModulating(const synth::Parameter& depth) {
    for (const synth::SceneState& pole : kScenePoles) {
        if (DepthIsModulatingInScene(depth, pole.leftScene)) {
            return true;
        }
    }
    return false;
}

inline void ZeroExistingModulationDepths(synth::Parameter& parameter) {
    for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
        synth::Parameter* depth = parameter.ModulationDepthParameter(modIx);
        if (depth == nullptr) {
            continue;
        }
        for (const synth::SceneState& pole : kScenePoles) {
            depth->SceneCenter(pole.leftScene) = kNeutralModulationDepthCenter;
        }
    }
}

// E.1 (design A6) -- the shared count/source-selection helper used by all
// four RandomMod dispatch sites in this file (RandomizeBankLevel1Depths,
// RandomizePage's drill-in branch, and both RandomizeAll drill-in branches).
// Chooses a COUNT of `parameter`'s connected modulation sources using the
// mode-2 table below (F1.2, 2026-08-07 -- supersedes A6's original median-3
// 10/30/30/20; see the table's own comment, right above the draw, for the
// exact numbers and their derivation), draws that many DISTINCT sources
// (partial Fisher-Yates -- Sheaf's
// own private Bank::RandomizeModulationDepths loop can independently redraw
// the same source twice, which this does not reproduce, per A6's "two
// properties Sheaf's loop has that the replacement should not inherit"), and
// for each chosen source calls the exact same two public calls Sheaf's own
// loop makes internally: `Parameter::EnsureModulationDepth` then
// `Parameter::RandomizeVisibleValue` (ParameterModulation.cpp:2896-2911).
// Sheaf performs every write; only the count and the source set are chosen
// here (design D14's split). Does NOT dispatch through Bank::HandlePress at
// all -- no press, no modifier-hold, no selection/level state touched -- so
// callers do not need `parameter` to be the bank's currently selected/open
// one.
//
// Returns true (a "partial" randomize, matching CapacityExhausted's meaning
// everywhere else in this file) when storage was already exhausted before
// this call, OR when `EnsureModulationDepth` returns null mid-loop (storage
// ran out while this call was materializing depths) -- the same "stop and
// report partial" convention `ApplyFroggersDefaultPatch`'s `applyDetent`
// lambda already uses for the identical null-return case.
inline bool RandomizeParameterModulationDepths(synth::ParameterManager& manager, synth::Parameter& parameter) {
    synth::ParameterGroup& group = parameter.Group();
    bool partial = CapacityExhausted(group);

    // A1 (non-additive, Option A -- operator 2026-08-05: "the randomization
    // should never have been additive"): zero this parameter's own existing
    // depths, BOTH scene poles (A3), before drawing the fresh set below.
    // Scope note: this function is the single call site every RandomizeAll/
    // RandomizePage branch below routes through per parameter, so "zero in
    // scope, then draw" naturally becomes "zero ALL depths this operation
    // touches" for Randomize All (RandomizeBankLevel1Depths calls this once
    // per top-level parameter, across every bank) and "zero only that page's
    // parameter's depths" for Randomize Page (RandomizePage's level-1/2
    // branch calls this exactly once, on the one selected parameter) -- no
    // separate wide "zero everything up front" pass is needed, since one
    // parameter's depths never affect another's.
    ZeroExistingModulationDepths(parameter);

    // Built once per call (not cached across calls: `connected` changes at
    // runtime, e.g. external-audio cabling) and reserved to the modulator
    // count -- O(15), trivial even at Randomize All's 54 calls.
    const std::span<const synth::ModulatorMetadata> metadata = group.GetModulators().Metadata();
    std::vector<std::size_t> eligible;
    eligible.reserve(metadata.size());
    for (std::size_t modIx = 0; modIx < metadata.size(); ++modIx) {
        if (metadata[modIx].connected) {
            eligible.push_back(modIx);
        }
    }
    if (eligible.empty()) {
        return partial;
    }

    // F1.2 (frogg3rs-blowout-and-drilldown-repair, 2026-08-07) -- mode-2
    // table. SUPERSEDES A6's 10/30/30/20: the operator's ruling -- "mode 2,
    // rarely above 4 is good enough" -- explicitly authorizes breaking A6's
    // deliberate 2/3 tie and tuning toward the resulting mean, both of which
    // A6's own directives (retracted by F0.3) used to forbid. Never-zero and
    // distinct-source draws are UNCHANGED from A6 -- see this function's own
    // header comment for why distinctness matters.
    //
    // count=1: 20% (u<0.20f)     count=2: 46% (u<0.66f) <- the mode
    // count=3: 26% (u<0.92f)     count=4:  6% (u<0.98f)
    // count=5+: 2%, geometric r=0.30 (else branch below)
    //
    // Derivation (operator-approved numbers; recorded so a future retune
    // starts from this arithmetic instead of re-deriving it):
    //   P(>=4)                   = 6% + 2%               = 8%     (was 30% under A6)
    //   P(count=7)               = 0.02 * 0.30^2 * 0.70   = 0.126%
    //   P(>=7)                   = 0.02 * 0.30^2          = 0.18%  (was 4.9% under A6)
    //   E[params at 7+ / press]  = 16 * 0.0018           = 0.029  (was 0.78, across 16
    //                               visible params -- roughly one press in 35, i.e.
    //                               "essentially never")
    //   mean = 0.20(1)+0.46(2)+0.26(3)+0.06(4)+0.02(5+0.3/0.7) = 2.25 (was ~3.1)
    //   mode = 2, strictly (46% > 26% > 20% > 6% > 2%); minimum is still 1, never 0
    const float u = manager.NextRandomCoin();
    std::size_t count;
    if (u < 0.20f) {
        count = 1;
    } else if (u < 0.66f) {
        count = 2;
    } else if (u < 0.92f) {
        count = 3;
    } else if (u < 0.98f) {
        count = 4;
    } else {
        count = 5;
        while (count < eligible.size() && manager.NextRandomCoin() < 0.30f) {
            ++count;
        }
    }
    count = std::min(count, eligible.size());

    // Partial Fisher-Yates over `eligible`: draws `count` DISTINCT source
    // indices (see this function's header comment on why "distinct" matters
    // here, unlike Sheaf's own loop).
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t remaining = eligible.size() - i;
        const std::size_t pick = i + manager.NextRandomIndex(remaining);
        std::swap(eligible[i], eligible[pick]);

        synth::Parameter* depth = parameter.EnsureModulationDepth(eligible[i]);
        if (depth == nullptr) {
            partial = true;
            break;  // storage exhausted mid-call -- partial, not a silent short-count.
        }
        // A3 (scene-pair semantics): the SAME chosen source (`eligible[i]`)
        // gets an INDEPENDENT random value in EACH scene pole -- identical
        // source membership (so the badge, true if ANY scene is nonzero,
        // agrees with what every scene actually holds), different values per
        // pole (so blending sweeps between two distinct modulation states).
        // Deliberately NOT `manager.Scene()` (the live, possibly mid-blend
        // scene) -- see kScenePole0/kScenePole1's own comment. F2: iterates
        // kScenePoles rather than naming each pole in its own statement --
        // see that array's own comment.
        for (const synth::SceneState& pole : kScenePoles) {
            depth->RandomizeVisibleValue(pole, manager.NextRandomValue());
        }
    }
    return partial;
}

// Presses `encoderId` directly on `bank` (bypassing any BankSlot -- confirmed
// safe: Bank::HandlePress/FindVisibleCell/topLevel_/visible_ are entirely
// Bank-owned state, with no dependency on whether this Bank is the slot's
// currently *selected* one) while `Modifier::Random` is held, then clears
// it -- i.e. a single-cell VALUE randomize. Narrowed from a generic
// `Modifier` parameter (E.1, design A6): depth-randomize
// (`Modifier::RandomMod`) no longer routes through a press at all --
// `RandomizeParameterModulationDepths` above is called directly on the
// target `Parameter&` instead -- so a parameter accepting a modifier this
// function never receives would itself have been misleading. Calling this
// directly on a `Bank&` (rather than through a level-tracking
// FroggersModulationDrillIn) is safe here specifically because a held
// modifier makes Bank::HandlePress's modifier branch
// (ParameterModulation.cpp:2633-2642) return before ever touching
// `selected_`/showing-modulation state, so no level bookkeeping can be
// disturbed by these presses.
inline void PressBankWithRandomValue(synth::ParameterManager& manager, synth::Bank& bank,
                                     synth::PhysicalEncoderId encoderId) {
    manager.SetRandomHeld(true);
    bank.HandlePress(encoderId, FullPhysicalLayout(bank));
    manager.SetRandomHeld(false);
}

// Randomizes one bank's 9 page-parameter values plus its Crispy (encoder 14)
// -- NEVER Crunchy (encoder 15) -- matching D14's "include per-bank Crispy,
// exclude global Crunchy" rule shared by Randomize All and Randomize Page's
// parameter-page cases. Presses `bank` directly (see PressBankWithRandomValue);
// no level state is touched or required.
// `includeCrispy` (operator decision 2026-07-29) exists because the two
// callers must differ on it:
//
//   Randomize Page -> TRUE.  One page's local Crispy is that page's business.
//   Randomize All  -> FALSE. Randomizing local Crispy on all six pages at once
//                     is effectively randomizing global Crunchy, which this
//                     app deliberately never randomizes (see this file's own
//                     "exclude global Crunchy" rule). Doing it six times over
//                     reaches the same place by another route.
inline void RandomizeBankValues(synth::ParameterManager& manager, synth::Bank& bank,
                                bool includeCrispy) {
    for (synth::PhysicalEncoderId e = 0; e < kFroggersParamsPerBank; ++e) {
        PressBankWithRandomValue(manager, bank, e);
    }
    if (includeCrispy) {
        PressBankWithRandomValue(manager, bank, kFroggersCrispySlot);
    }
}

// Randomizes one bank's 9 page parameters' LEVEL-1 depths -- and only those.
// Crispy (encoder 14) is deliberately EXCLUDED here, as is Crunchy (encoder
// 15), so unlike RandomizeBankValues above there is no `includeCrispy` knob:
// that function randomizes VALUES and its two callers differ on Crispy, while
// this one randomizes DEPTHS and its sole caller (RandomizeAll's level-0
// branch, which runs it once per bank) always wants Crispy out. The why is
// spelled out at this function's tail (operator 2026-07-29).
// E.1 (design A6): no longer presses through Bank::HandlePress at all --
// `bank.VisibleParameter(paramIx)` reads the top-level parameter directly
// (this bank is never drilled into here, so `visible_ == topLevel_`, per
// PressBankWithRandomValue's own header comment on Bank-owned state), and
// RandomizeParameterModulationDepths is called on it directly (it derives
// the parameter's group itself via `parameter.Group()`, which is every page
// parameter's SAME group -- FroggersParameterModel's one mono
// ParameterGroup -- so no separate group parameter is needed here anymore).
inline bool RandomizeBankLevel1Depths(synth::ParameterManager& manager, synth::Bank& bank) {
    bool partial = false;
    // F0.1: each call's result is hoisted into its own named local BEFORE
    // combining (mirrors FroggersAppCore.hpp:482-488's F4 fix), so every
    // iteration of this loop always runs its randomize call -- `partial =
    // RandomizeParameterModulationDepths(...) || partial` reads fine but is
    // only correct because the call sits on the left of `||`; swapping the
    // combine order (or a future edit that does) would short-circuit once
    // `partial` went true and skip randomizing every remaining page parameter.
    for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
        synth::Parameter* param = bank.VisibleParameter(static_cast<synth::PhysicalEncoderId>(paramIx));
        if (param == nullptr) {
            continue;  // defensive: every page-parameter slot is always registered in practice.
        }
        const bool paramPartial = RandomizeParameterModulationDepths(manager, *param);
        partial = partial || paramPartial;
    }
    // Crispy's DEPTHS are excluded here for the same reason its value is
    // (operator 2026-07-29): this function runs once per bank from Randomize
    // All, so modulating local Crispy on all six pages lands in the same place
    // as randomizing global Crunchy. Randomize Page reaches a single page's
    // Crispy depths through the ordinary drill-in path, which is unaffected.
    return partial;
}

}  // namespace detail

// task 6.9: Randomize Page -- "randomize exactly what is displayed."
//   - parameter page (drillIn.Level()==0): this bank's 9 values + Crispy,
//     excluding Crunchy; no depths.
//   - level-1 grid (Level()==1): one RandomizeParameterModulationDepths call
//     on the selected parameter (the L1 parameter's own 15 depths).
//   - level-2 grid (Level()==2): one RandomizeParameterModulationDepths call
//     on the selected (depth) parameter's own 15 depths.
inline FroggersRandomizeResult RandomizePage(synth::ParameterManager& manager, FroggersModulationDrillIn& drillIn) {
    if (drillIn.Level() == 0) {
        // Randomize Page: this page's own Crispy IS included -- see the flag's
        // comment on RandomizeBankValues for why the two callers differ.
        detail::RandomizeBankValues(manager, drillIn.BankRef(), /*includeCrispy=*/true);
        return {};
    }
    // Level 1 or 2: one RandomizeParameterModulationDepths call (design A6's
    // count/source-selection helper) on whichever parameter is currently
    // selected. E.1: no longer a Target/Back-cell press -- the helper is
    // called on the selected parameter directly.
    synth::Parameter& selected = *drillIn.BankRef().SelectedParameter();
    const bool partial = detail::RandomizeParameterModulationDepths(manager, selected);
    return {partial};
}

// task 6.8: Randomize All -- context-sensitive by view, "wider and deeper."
//   - parameter page (Level()==0): every top-level parameter ACROSS ALL SIX
//     BANKS -- value + level-1 depths, Crispy included, Crunchy excluded.
//     Never descends to level 2. Each bank is pressed directly via its own
//     `Bank&` (PressBankWithRandomValue), independent of which bank the
//     BankSlot currently displays, so the active/displayed bank is left
//     completely undisturbed by this global operation.
//   - ANY drilled-in grid (Level() >= 1): the ONE selected parameter's own
//     depths, PLUS one more RandomizeParameterModulationDepths call on each of
//     those depths that is ACTUALLY MODULATING (detail::DepthIsModulating) --
//     i.e. randomizing at level N also randomizes level N+1, at every N, gated
//     only by kMaxDrillLevel. Operator 2026-08-07: "level 1 randomize all
//     should affect level 2 randomization. level 2 randomize all should affect
//     level 3." There is no per-level branch: the old shape descended at
//     level 1 only and let levels 2/3 fall through to RandomizePage, which was
//     a hardcoded special case of exactly the kind F5.1 deleted from Back().
//
//     Two things this deliberately does NOT do. It never opens a view --
//     RandomizeParameterModulationDepths takes `Parameter&` and reads no view
//     state -- so the level counter never moves (F4, the ejection fix). And it
//     skips neutral depths rather than descending into every materialized one:
//     a neutral depth modulates nothing, and sub-modulating it made the
//     drilled parameter report 13 badges against 1 live source, because
//     Sheaf's badge criterion counts a depth that merely HAS sub-modulation.
//     See detail::DepthIsModulating's own comment.
inline FroggersRandomizeResult RandomizeAll(synth::ParameterManager& manager, FroggersModulationDrillIn& drillIn,
                                            FroggersParameterModel& model) {
    if (drillIn.Level() == 0) {
        bool partial = false;
        // F0.1: hoisted for the same reason as RandomizeBankLevel1Depths's own
        // loop above (mirrors FroggersAppCore.hpp:482-488) -- this loop is
        // ACROSS ALL SIX BANKS, so a short-circuited `||` here would skip
        // randomizing every remaining bank once one had already gone partial.
        for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
            const auto bankId = static_cast<FroggersBankId>(bankIx);
            synth::Bank& bank = model.BankAt(bankId);
            // Randomize All: Crispy EXCLUDED on every bank (operator 2026-07-29).
            detail::RandomizeBankValues(manager, bank, /*includeCrispy=*/false);
            const bool bankPartial = detail::RandomizeBankLevel1Depths(manager, bank);
            partial = partial || bankPartial;
        }
        return {partial};
    }

    // ONE rule for every drilled-in level (operator, 2026-08-07: "level 1
    // randomize all should affect level 2 randomization. level 2 randomize all
    // should affect level 3"). This used to be a `Level() == 1` branch with
    // levels 2 and 3 falling through to RandomizePage, i.e. the descent was a
    // hardcoded level-1 special case -- the same §5/§8 shape F5.1 deleted from
    // Back()'s `wasLevelTwo`. There is no per-level branching now: at ANY
    // drilled-in level, randomize the selected parameter's own depths, then
    // descend exactly one level, guarded by kMaxDrillLevel. Raising the cap
    // again needs no edit here.
    //
    // Level 0 already returned above, so this is every drilled-in level. No
    // guard condition and no enclosing block: a redundant `Level() >= 1` test
    // would only add an unreachable tail the compiler still demands a return
    // for.
    //
    // F4 (operator: "randomize all in level 1 shouldn't navigate me out wtf"):
    // this used to Back() out to level 0 to look up the selected parameter's
    // encoder id, PressEncoder back in, then PressEncoder/Back once per depth
    // parameter to reach the next level -- round trips whose only permanent
    // effect was driving the level counter to 0, which was the ejection. None
    // of it was needed: RandomizeParameterModulationDepths takes `Parameter&`
    // directly, reads eligibility from `group.GetModulators().Metadata()`, and
    // calls EnsureModulationDepth itself -- nothing in it reads view state. So
    // this only ever calls that helper, never the view, and the level never
    // moves. (F4.3: Bank::Deselect(), which Back() called, had always pruned
    // the transient eager materialization back to whatever was actually
    // written, so removing the round trips left the steady-state count
    // unchanged; what it dropped was the wasted allocate-then-prune churn.)
    synth::Parameter& selectedParam = *drillIn.BankRef().SelectedParameter();
    bool partial = detail::RandomizeParameterModulationDepths(manager, selectedParam);
    // Descend one level -- but ONLY if a deeper level exists to descend into.
    // Read from the drill-in's own single definition site rather than
    // re-testing a literal (OMNI §8).
    if (drillIn.Level() < FroggersModulationDrillIn::kMaxDrillLevel) {
        for (std::size_t modIx = 0; modIx < FroggersParameterModel::kNumModulators; ++modIx) {
            synth::Parameter* depthParam = selectedParam.ModulationDepthParameter(modIx);
            if (depthParam == nullptr) {
                continue;  // not connected, or the draw above hit CanAllocate()==false
            }
            // Descend only into depths that are ACTUALLY modulating. A
            // materialized-but-neutral depth modulates nothing, so
            // sub-modulating it is meaningless -- and it is what made the
            // drilled parameter show 13 badges when 1 source was live. See
            // DepthIsModulating's own comment for the badge mechanism.
            if (!detail::DepthIsModulating(*depthParam)) {
                // CLEAR it rather than merely skipping. Randomize is
                // non-additive (A1): each press zeroes the selected
                // parameter's own depths before drawing, so a source picked
                // last press is neutral this press. But
                // ZeroExistingModulationDepths DOES NOT RECURSE -- it clears
                // this parameter's depths, not those depths' own sub-depths.
                // Skipping alone would therefore leave the sub-depths a
                // PREVIOUS press wrote sitting on a now-neutral depth, and
                // Sheaf's badge criterion counts a depth that merely HAS
                // sub-modulation -- so the badge would stay lit for a source
                // that is modulating nothing, which is the very defect this
                // rule exists to remove. Found by this task's own test.
                detail::ZeroExistingModulationDepths(*depthParam);
                continue;
            }
            // F0.1: hoisted for the same reason (mirrors FroggersAppCore.hpp:482-488)
            // -- this loop is the depth-parameter sweep, so a short-circuited
            // `||` here would skip randomizing every remaining depth parameter.
            const bool depthPartial = detail::RandomizeParameterModulationDepths(manager, *depthParam);
            partial = partial || depthPartial;
        }
    }
    return {partial};

}

// ============================================================================
// task 6.12 -- default patch (design D16)
// ============================================================================
// Requires slate indices 6-8 (VCO audio sources) to already be registered,
// hence this function takes the slate too, and must run AFTER
// FroggersModulationSlate::Init.
inline void ApplyFroggersDefaultPatch(FroggersParameterModel& model) {
    // "One encoder detent" (design D16): the surface's real encoder
    // resolution is not established until packet 10, so this is an explicit
    // placeholder quantum, flagged per D16's own "ties the default to the
    // encoder resolution" caveat -- NOT a value derived from any cited
    // Sheaf/Froggers source. 1/100 approximates a common relative-encoder
    // resolution; if packet 10 establishes a different one, this constant
    // (and therefore this default) must be revisited.
    constexpr float kPlaceholderEncoderDetent = 1.0f / 100.0f;

    // C1a (tasks.md CONSOLIDATED PUSH, item C1 -- operator: "the default VCO
    // shapes in scene 2 should be the opposite of what they are in scene
    // 1"): scene 1 (pole 0, `detail::kScenePole0`) keeps sine/saw/square =
    // 0.0/0.5/1.0 exactly as before -- Audio bank slots 3-5 (design D5a),
    // confirmed against EvalWaveMorph's sine->saw (0-0.5) / saw->square
    // (0.5-1.0) crossfade (app/dsp/Vco.hpp's EvalWaveMorph, ported from
    // src/core/VcoWaveEval.hpp:7-23). Scene 2 (pole 1, `detail::
    // kScenePole1`) gets the MIRROR of that same value, `1.0 - shape` --
    // written as an expression of the scene-1 value, not as three more
    // hardcoded literals, so the mirror relationship is what the reader
    // sees. VCO2's 0.5 (saw) is its own mirror and is therefore unchanged
    // in scene 2 too, without needing a special case.
    constexpr std::array<float, 3> kScene1VcoShapes{0.0f, 0.5f, 1.0f};
    for (std::size_t vcoIx = 0; vcoIx < kScene1VcoShapes.size(); ++vcoIx) {
        const float scene1Shape = kScene1VcoShapes[vcoIx];
        synth::Parameter& shapeParam = model.PageParameter(FroggersBankId::Audio, 3 + vcoIx);
        shapeParam.HandleSetAbsolute(detail::kScenePole0, scene1Shape);
        shapeParam.HandleSetAbsolute(detail::kScenePole1, 1.0f - scene1Shape);
    }

    // C1b: the same light cross-VCO pitch modulation lands in BOTH scene
    // poles, identically -- same sources, same signs, same magnitude. This
    // matches A3's rule that source membership (and, for this fixed
    // non-randomized default, depth too) is identical across poles.
    // Sourced from the VCO audio-rate slate entries (indices 6-8), targeting
    // Audio bank pitch parameters (slots 0-2):
    //   VCO1 pitch (slot 0) <- +1 detent from VCO2 audio (7), VCO3 audio (8)
    //   VCO2 pitch (slot 1) <- -1 detent from VCO1 audio (6), VCO3 audio (8)
    //   VCO3 pitch (slot 2) <- +1 detent from VCO1 audio (6), VCO2 audio (7)
    auto applyDetent = [&](std::size_t targetParamIx, std::size_t modIx, float sign) {
        synth::Parameter& target = model.PageParameter(FroggersBankId::Audio, targetParamIx);
        synth::Parameter* depth = target.EnsureModulationDepth(modIx);
        if (depth == nullptr) {
            return;  // storage exhausted -- would be a partial default patch; flagged, not silently ignored
        }
        // F2: iterates kScenePoles rather than naming each pole in its own
        // statement -- see that array's own comment. Same delta applied
        // independently to each pole's own scene center, from the same
        // freshly-materialized neutral base, so both poles land on the same
        // final value.
        for (const synth::SceneState& pole : detail::kScenePoles) {
            depth->HandleIncDec(pole, sign * kPlaceholderEncoderDetent);
        }
    };
    applyDetent(0, kModSlotVco2Audio, +1.0f);
    applyDetent(0, kModSlotVco3Audio, +1.0f);
    applyDetent(1, kModSlotVco1Audio, -1.0f);
    applyDetent(1, kModSlotVco3Audio, -1.0f);
    applyDetent(2, kModSlotVco1Audio, +1.0f);
    applyDetent(2, kModSlotVco2Audio, +1.0f);

    // C1c: Drive (Drive bank, slot 0) = 20% of its range (D16, flagged
    // provisional -- "GAIN on the distortion page" maps to the Drive
    // parameter, no parameter is literally named Gain, D5a's Drive-bank
    // layout), identical in both scene poles -- only the VCO shapes differ
    // between scene 1 and scene 2.
    for (const synth::SceneState& pole : detail::kScenePoles) {
        model.PageParameter(FroggersBankId::Drive, 0).HandleSetAbsolute(pole, 0.2f);
    }
}

}  // namespace synth_froggers
