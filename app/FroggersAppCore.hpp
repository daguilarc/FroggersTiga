#pragma once

// synth_froggers::FroggersAppCore -- packet 10 of the froggers-sheaf-app
// change (openspec/changes/froggers-sheaf-app/tasks.md, section "10. Surface
// layout (ported v2 design)"; design D11, following Braid 4's own
// Core/UI/outer-composition split, apps/braid-4/Braid4Core.hpp +
// Braid4UI.hpp + Braid4.hpp).
//
// This is packets 1-9's `FroggersApp` class (Config/Init/PrepareToPlay/
// ProcessBlock/RouteAudioSample and every existing accessor), renamed and
// UNCHANGED in substance, PLUS the packet-10 UI-thread -> audio-thread
// request bridge described below. The composed `synth_froggers::FroggersApp`
// (still that exact name, so every existing test TU's
// `#include "Froggers.hpp"` / `synth_froggers::FroggersApp` keeps working
// unchanged) now lives in Froggers.hpp as a thin wrapper: `class FroggersApp
// : public FroggersAppCore` that adds only the `FroggersUiSurface` member and
// `PortableSurface()` -- exactly Braid4.hpp's own shape.
//
// ============================================================================
// Why a request bridge exists (design D11/D14, task 10.1-10.7)
// ============================================================================
// `synth::AppContext` documents `parameterManager`/`masterClock` as
// audio-thread-owned once the engine is running (AppContext.hpp's own
// thread-role comments); the sanctioned way for UI code (message thread) to
// influence them is the existing `MessageInBus* uiBus` (message thread
// produces, audio thread consumes inside `Engine::ProcessBlock`'s
// `DrainMessageBus` call, itself before `app_.ProcessBlock()`).
//
// That generic bus is exactly right for encoder DRAG (`MessageIn::
// ParamIncDec`/`ParamSetAbsolute`), scene select/blend, and transport Start/
// Stop -- `Bank::HandleTick`/`HandleSetAbsolute` only look up the currently
// visible cell and never touch `selected_`/level state (verified by reading
// `src/ParameterModulation.cpp`'s `Bank::HandleTick`/`HandleSetAbsolute`),
// so this surface pushes those four kinds of action straight onto
// `context_->uiBus`, the same way `apps/braid-4/Braid4UI.hpp` does.
//
// It is NOT right for an encoder PRESS (drill-in), Randomize All/Page, or
// the BPM slider, for two different reasons:
//   - Encoder press MUST go through `FroggersModulationDrillIn::PressEncoder`
//     (FroggersModulation.hpp, packet 6) rather than a generic
//     `MessageIn::ParamPush`, because that class is the ONLY thing enforcing
//     this app's 2-level drill-in cap -- Sheaf's own `Bank` has no level
//     concept at all (FroggersModulation.hpp's own header comment) and would
//     happily let a generic press descend to a third, fourth, ... level.
//   - Randomize All/Page (`FroggersModulation.hpp`'s `RandomizeAll`/
//     `RandomizePage`) and the BPM slider (`MasterClock::SetTempoBpm`/
//     `TempoBpm`, both audio-thread-owned per `AppContext.hpp`) mutate
//     audio-thread-owned state directly, with no existing generic
//     `MessageIn` shape for either of them.
//
// Crunchy no longer routes through this bridge (operator 2026-07-27): the
// chrome-band Crunchy slider that used to call `RequestCrunchy()` here is
// removed (it duplicated the real control at bank slot 15 -- see
// FroggersUiSurface.hpp's own header comment/tasks.md 10.2). Crunchy is a
// `Parameter` shared across all six banks, but at slot 15 it is addressed
// exactly like any other bank parameter (generic encoder press/drag over
// `context_->uiBus`), so it never needed a pending-atomic request of its
// own -- the chrome slider was the only thing that did, purely because it
// bypassed the bank/slot addressing scheme entirely.
//
// The fix used throughout this file is the same one packet 8's clock-driven
// Marbles already established for "audio-thread state written once per
// block, safely observed cross-thread": small, single-slot pending-request
// atomics that the UI/message thread WRITES (`Request*` methods below) and
// that `ProcessFrame()` (detected via `AppConcepts.hpp`'s `HasProcessFrame`
// concept, invoked by `synth::Engine` once per block, after message drains
// and before `ProcessBlock()` -- i.e. on the audio thread) DRAINS and
// applies. Display-direction state (the master clock's active tempo,
// whether it is externally slaved) is published the same way in reverse,
// once per block from `ProcessBlock()`'s existing end-of-block publish
// section.

#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "FroggersTransferFunctionVisualizer.hpp"

#include "dsp/Delay.hpp"
#include "dsp/Drive.hpp"
#include "dsp/DspMath.hpp"
#include "dsp/FilterFx.hpp"
#include "dsp/Limiter.hpp"
#include "dsp/Reverb.hpp"
#include "dsp/Vco.hpp"
#include "dsp/VoiceEnvelope.hpp"

#include "synth/AppConcepts.hpp"
#include "synth/AppContext.hpp"
#include "synth/Color.hpp"
#include "synth/DspScope.hpp"
#include "synth/MasterClock.hpp"
#include "synth/PortableScopeVisualizer.hpp"
#include "synth/PortableUI.hpp"
#include "synth/PortableUIBuilders.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace synth_froggers {

class FroggersAppCore {
public:
    // Tasks 7.2/9.2 (design D7/D10): the ONLY hand-written constructor work
    // this class needs -- everything else is default member initializers.
    // Reserves the VCO scope's three channels (one per VCO) and wires each
    // VCO's ScopeWriterHolder/color, then constructs the two transfer-function
    // Visualizers over this instance's own peakUiState_/combUiState_ (task
    // 9.1's UIState members, populated once per block in ProcessBlock()).
    // `synth::ui::ScopeVisualizer`/`synth_froggers::TransferFunctionVisualizer`
    // have no default constructor, so this class cannot rely on implicit
    // default construction the way it did before packets 7/9.
    FroggersAppCore()
        : vco1ScopeHolder_(vcoScopeWriter_.ReserveChans(1)),
          vco2ScopeHolder_(vcoScopeWriter_.ReserveChans(1)),
          vco3ScopeHolder_(vcoScopeWriter_.ReserveChans(1)),
          vcoScopeVisualizer_(
              std::array<dsp::Vco::UIState*, 3>{&vco1ScopeUiState_, &vco2ScopeUiState_, &vco3ScopeUiState_},
              // D.2 sizing decision, recorded 2026-07-29 (open since the
              // predecessor's task 1.2 and never written down). Verdict:
              // the defaults are correct for this app; no change.
              //   - Read window 512 samples across a 340px-wide panel
              //     (FroggersUiSurface's kScopeWidth) is ~1.5 samples per
              //     pixel -- slightly oversampled, which is the right side to
              //     err on: the polyline builder decimates, it cannot invent
              //     detail. At 48kHz, 512 samples = 10.7ms, so the lowest
              //     default voice (110Hz, 9.1ms period) shows ~1.2 cycles and
              //     the highest (330Hz) ~3.5. That is a readable window.
              //   - ScopeWriter's default ring is maxFrames=4096 (DspScope.hpp),
              //     8x the read window and 85ms of history, with 3 of its 16
              //     channels reserved. Ample.
              //   - Braid 4's kScopeFrames = 6'553'600 is NOT a target to
              //     match: that sizes a marker-aligned long-window display it
              //     hand-builds. Copying the number without the display it
              //     serves would just allocate ~400MB for nothing.
              /*minY=*/-1.0f, /*maxY=*/1.0f, /*numSamples=*/512, /*drawMarkers=*/true),
          peakVisualizer_(peakUiState_, synth::Color::Blue),
          combVisualizer_(combUiState_, synth::Color::Blue.AdjustBrightness(0.7f)) {
        audioVco1_.SetScopeWriterHolder(&vco1ScopeHolder_);
        audioVco2_.SetScopeWriterHolder(&vco2ScopeHolder_);
        audioVco3_.SetScopeWriterHolder(&vco3ScopeHolder_);
        // UI-rework ITEM 2 (design.md A3b, tasks.md B.2, 2026-07-29): was
        // Red/Orange/Yellow -- three hues that collapse together under
        // red-green colour blindness (protanopia/deuteranopia), an operator
        // report from the running build. `synth::Color` has `Cyan` and
        // `Yellow` (External/Sheaf/projects/synth/include/synth/Color.hpp)
        // but no `Pink`/`Magenta` (verified by grep of that file), so pink
        // is `Color::Rgb(255, 105, 180)` -- bright against the panel's dark
        // background and separated from cyan/yellow in both blue channel
        // and luminance. Do NOT restore Red/Orange/Yellow.
        audioVco1_.SetScopeColor(synth::Color::Cyan);
        audioVco2_.SetScopeColor(synth::Color::Rgb(255, 105, 180));  // pink -- no synth::Color::Pink/Magenta exists.
        audioVco3_.SetScopeColor(synth::Color::Yellow);
    }

    static synth::RuntimeConfig Config() {
        synth::RuntimeConfig config;
        config.appName = "Frogg3rs Synth";
        // Task 2.6 (tasks.md section 2; design D5 slate slots 13-14). This
        // was 0, which made the modulation slate's two external-audio
        // sources (audio rate + its envelope follower) permanently
        // `.connected = false` by *construction* -- the app never asked the
        // host for an input channel at all, so no cable could ever change
        // that. Both slate slots are derived from the same external signal
        // (design D5's slate table has one external-audio source, tapped
        // twice), so exactly one input channel covers both -- confirmed
        // against the host's actual contract, not assumed: `numAudioInputs`
        // is a request forwarded verbatim to
        // `juce::AudioDeviceManager::initialiseWithDefaultDevices(config.numAudioInputs, ...)`
        // (External/Sheaf/projects/synth/runtime/Runtime.hpp:237, gated at
        // `:260-261`), and the headless test rig honors it 1:1, allocating
        // exactly `numAudioInputs` input buffers/pointers
        // (tests/support/SynthRig.hpp:62,72,76,454,456). Raising this to 1
        // makes the flag track actual cabling (host-reported
        // `AudioBlock::numInputChannels`/`inputs`, packet 6 task 6.5) rather
        // than a permanently-zero request.
        config.numAudioInputs = 1;
        config.numAudioOutputs = 2;
        config.preferredSampleRate = 48000.0;
        config.preferredBlockSize = 256;
        config.uiWidth = 900;
        // DEMOTED by task F.3 (openspec/changes/frogg3rs-audio-safety-and-
        // ui-rework/tasks.md, 2026-08-04/05): this literal is now just the
        // window's INITIAL size, not a derived cross-check. Before F.3 it
        // was required to equal `FroggersPageLayout::RequiredHeight()`
        // (hand-maintained in sync, verified by a dedicated test) because
        // `FroggersUiSurface.hpp`'s hand-rolled auto-flow model
        // (`FroggersAutoFlowedChromeModel`) needed the app to reserve
        // exactly the right amount of vertical space for its own chrome
        // band ahead of time. F.3 replaced that band with one declarative
        // grid whose regions are resolved by Sheaf's own layout engine
        // against whatever root extent it is given (`FroggersPageLayout::
        // RootBounds()`), so there is nothing left for this literal to be
        // derived FROM, and the circular-include workaround that comment
        // used to describe (`FroggersUiSurface.hpp` cannot call back into
        // this file) no longer applies because there is no call to make.
        // 632 itself is unchanged -- it is simply a plain, hand-picked
        // initial window size now, matching
        // `synth_froggers::FroggersPageLayout::kDefaultHeight`
        // (FroggersUiSurface.hpp). `FroggersSurfaceTests.cpp`'s fit-guard
        // tests assert the surface actually fits its declared grid at this
        // size (and at a smaller and a larger one), which is a stronger
        // guarantee than the old cross-check ever was (see task F.3's
        // report for why the old test could never fail even when wrong).
        config.uiHeight = 632;
        config.uiFrameHz = 30;
        return config;
    }

    void Init(synth::AppContext* context) {
        if (context == nullptr || context->parameterManager == nullptr) {
            throw std::invalid_argument("FroggersApp requires a valid app context");
        }
        context_ = context;
        // Packet 4 (tasks.md section 4): build the six 16-slot parameter
        // banks (Crispy@14/Crunchy@15 in every bank), the shared BankSlot,
        // and the mono ParameterGroup + scenes. See FroggersParameters.hpp.
        // Task 9.3 (design D10): peakVisualizer_/combVisualizer_ are
        // constructed by this class's own constructor (see its comment),
        // ahead of Init() always running -- passed in so
        // FroggersParameterModel can attach them as the Filter bank's
        // Peak/Comb parameter underlays.
        parameters_.Init(*context->parameterManager, &peakVisualizer_, &combVisualizer_);

        // Packet 6 (tasks.md section 6, tasks 6.1/6.2): register all 15
        // modulation sources, in design D5's order. See FroggersModulation.hpp.
        modulation_.Init(parameters_.Group());

        // Task 6.12 (design D16): apply the default patch once, on first
        // start, now that slate indices 6-8 (VCO audio sources) exist.
        ApplyFroggersDefaultPatch(parameters_);

        // Packet 10 (design D11): the single active-bank authority (D14's
        // "single selection authority" requirement) plus its own drill-in
        // level tracker, constructed against bank 0 -- the same default
        // active selection FroggersParameterModel::Init() already made via
        // `slot_->SelectBank(banks_[0])`.
        drillIn_.emplace(parameters_.BankAt(activeBankIx_));
    }

    // Sample-rate-dependent modulation-slate setup (packet 6): detected and
    // called automatically by synth::Engine via the optional HasPrepareToPlay
    // hook (AppConcepts.hpp:26-28) once the host negotiates a real sample
    // rate. Everything else in this class is sample-rate-independent at
    // Init() time.
    void PrepareToPlay(double sampleRate, int /*blockSize*/) {
        modulation_.Prepare(sampleRate);

        // Task 6a.1 (design D15): the audio-path DSP units below are a
        // SEPARATE set of instances from modulation_'s own private VCOs
        // (FroggersModulation.hpp's vco1_/vco2_/vco3_, which exist solely to
        // produce the six VCO-audio-rate modulation-source values and are
        // never summed to the output bus -- see that header's own scope
        // note). These need their own sample-rate-dependent setup.
        sampleRate_ = static_cast<float>(sampleRate);
        audioAdsr_.init(sampleRate_);

        // ITEM 2a: Sheaf's parameter-smoothing constants
        // (kDefaultProcessLiteAlpha/kDefaultTargetComputeIntervalSamples/
        // kDefaultUiDisplayCenterAlpha/kDefaultUiDisplaySpreadAlpha,
        // ParameterModulation.hpp:170-174) are defined at a 48 kHz
        // reference and ParameterGroupConfig starts out holding exactly
        // those raw values (ParameterModulation.hpp:199-203) until
        // ConfigureProcessingTiming replaces them
        // (ParameterModulation.cpp:859-865) -- otherwise knob glide,
        // modulation-depth smoothing, and UI-display slew all run at the
        // wrong real-time rate at any host rate other than 48 kHz. Mirrors
        // Braid 4's own PrepareToPlay (Braid4Core.hpp:207-220), which
        // converts against internalSampleRate_ (its oversampled internal
        // parameter-tier rate); this app has no such oversampling at the
        // parameter tier (parameters_/modulation_ share the single mono
        // ParameterGroup below, driven once per sample at the host rate --
        // see modulation_.Init(parameters_.Group()) in Init()), so this
        // converts against the host `sampleRate` directly instead. One
        // call covers modulation_ too: modulation_.Init(parameters_.Group())
        // (Init(), above) hands it the SAME ParameterGroup instance, not a
        // separate one.
        parameters_.Group().ConfigureProcessingTiming(synth::ParameterProcessingTiming{
            .processLiteAlpha =
                synth::ConvertOnePoleAlpha(synth::kDefaultProcessLiteAlpha, 48000.0, sampleRate),
            .targetComputeIntervalSamples = synth::ConvertSampleInterval(
                synth::kDefaultTargetComputeIntervalSamples, 48000.0, sampleRate),
            .uiDisplayCenterAlpha =
                synth::ConvertOnePoleAlpha(synth::kDefaultUiDisplayCenterAlpha, 48000.0, sampleRate),
            .uiDisplaySpreadAlpha =
                synth::ConvertOnePoleAlpha(synth::kDefaultUiDisplaySpreadAlpha, 48000.0, sampleRate),
        });
        // Task 6b (design D17, revised 2026-07-27): the ASR gate is no
        // longer forced permanently on -- it is driven per-sample in
        // ProcessBlock() from the master clock's transport quarter-note
        // pulse (see TransportQuarterNotesAt()/the gate computation there).
        // audioAdsr_.init() above already leaves the gate low
        // (VcoAdsrState::init() sets m_gateHigh = false, Stage::Idle), which
        // is the correct starting state for "silent until the transport
        // runs."
        delay_.SetSampleRate(sampleRate_);  // B6a: also (re)configures wetLimiterL/R's coeffs, see dsp/Delay.hpp.
        outputLimiter_.Configure(sampleRate_);  // item 3: attack/release coeffs are sample-rate-dependent.
        filterChain_.Configure(sampleRate_);  // B5: peak-branch limiter's own coeffs, same reason.
        reverb_.Configure(sampleRate_);  // B6b: reverb's own wetLimiter coeffs, same reason.
        driveBlendPhase_.Configure(sampleRate_);  // B7.2: this stage's own outputLimiter coeffs, same reason.

        // Root-cause fix (D17 robustness gap, found while diagnosing "Play
        // produces no audio in the real Runtime"): `synth::Engine::Prepare()`
        // calls `MasterClock::Prepare()` UNCONDITIONALLY before this hook
        // runs (Engine.hpp's own Prepare(), which calls
        // `masterClock_.Prepare(sampleRate, blockSize)` first, then this
        // method) -- and `MasterClock::Prepare()` unconditionally resets
        // `transportState_` to `Stopped`
        // (External/Sheaf/projects/synth/src/MasterClock.cpp:929), with no
        // regard for whether the transport was already `Running`.
        // `synth::Engine::Prepare()` is not a one-time startup call: JUCE's
        // `synth_runtime::Runtime<App>::audioDeviceAboutToStart` (Runtime.hpp)
        // re-invokes it on EVERY audio-device renegotiation -- not just user
        // device switches (`ApplyAudioDeviceSelection`/
        // `ApplyAudioDeviceInputSelection`), but also spontaneous ones during
        // ordinary startup (verified against a real session's own log,
        // ~/Library/Sheaf/synth/sheaf-patch/logs/: three separate "Audio
        // prepared" lines fired before any user interaction at all, one of
        // them renegotiating the block size from 256 to 512 frames). Each
        // one silently re-closes the D17 transport-gated ASR with no visual
        // indication and no automatic recovery -- a user who pressed Play
        // and then hit (or caused, e.g. by switching output devices while
        // troubleshooting) any such renegotiation gets permanent silence
        // with no way to know Play needs pressing again. Verified
        // reproducible headlessly: `synth::Engine::Prepare()` called a
        // second time after a `MessageIn::Start` mid-session drops
        // `TransportState()` back to `Stopped` and output back to silence,
        // exactly matching the reported symptom.
        //
        // Fix: track the surface's last explicit Play/Stop request
        // independently of `MasterClock::TransportState()` (which this same
        // Prepare() call may have just clobbered), and re-push the SAME
        // `MessageIn::Start` the Play button itself pushes
        // (`FroggersUiSurface::HandleAction`, via the sanctioned
        // message-thread-producer/audio-thread-consumer `uiBus`) so the very
        // next `ProcessBlock` (audio thread, which always drains `uiBus`
        // before touching the clock plan) restores `Running` before another
        // sample is produced. `PrepareToPlay()` runs on the thread that
        // called `Engine::Prepare()` -- the message thread for every runtime
        // device-renegotiation path -- so pushing onto `uiBus` here is the
        // same safe, already-relied-upon cross-thread path `FroggersUiSurface`
        // uses, not a new one.
        if (desiredTransportRunning_.load(std::memory_order_acquire) && context_ != nullptr &&
            context_->uiBus != nullptr) {
            const std::uint64_t timestamp = context_->now ? context_->now() : 0;
            context_->uiBus->Push(synth::MessageIn::Start(timestamp));
        }
    }

    // Packet 10 follow-up (D17 robustness fix, see PrepareToPlay()'s own
    // comment): records the surface's last explicit Play/Stop request so
    // PrepareToPlay() can re-assert it across a `MasterClock::Prepare()`
    // reset triggered by an audio-device renegotiation. Called from
    // `FroggersUiSurface::HandleAction` alongside its existing `uiBus` push
    // -- this does not change Play/Stop's own behavior, it only lets a LATER
    // renegotiation event recover it.
    void SetDesiredTransportRunning(bool running) {
        desiredTransportRunning_.store(running, std::memory_order_release);
    }

    // Packet 10 (design D11/D14, tasks 10.2-10.7): the surface's request API
    // -- called from FroggersUiSurface::DispatchAction (UI/message thread).
    // Each is a single-slot pending request; a later write before the audio
    // thread drains the previous one simply coalesces (acceptable: these are
    // all control-rate, human-paced actions, never a data stream). See this
    // file's header comment for why each one needs to be a request rather
    // than a direct call.
    void RequestBankSelect(std::size_t bankIx) {
        pendingBankSelect_.store(static_cast<int>(bankIx), std::memory_order_release);
    }
    void RequestEncoderPress(std::size_t encoderId) {
        pendingEncoderPress_.store(static_cast<int>(encoderId), std::memory_order_release);
    }
    void RequestRandomizeAll() { pendingRandomizeAll_.store(true, std::memory_order_release); }
    void RequestRandomizePage() { pendingRandomizePage_.store(true, std::memory_order_release); }
    // Task 10.6 (design D17's citation chain): a negative sentinel means "no
    // pending request." `MasterClock::SetTempoBpm` itself already no-ops
    // (returns false) while slaved to external MIDI clock
    // (src/MasterClock.cpp:963-965) -- ProcessFrame() below still calls it
    // unconditionally when a request is pending; the surface's own
    // DispatchAction additionally never enqueues a request while slaved (see
    // FroggersUiSurface.hpp), so this is a belt-and-suspenders no-op, not the
    // sole guard.
    void RequestTempoBpm(double bpm) { pendingTempoBpmRequest_.store(bpm, std::memory_order_release); }

    // Packet 10: display-direction reads for the surface (UI/message
    // thread), published once per block from ProcessBlock()'s existing
    // end-of-block section below.
    double DisplayTempoBpm() const { return tempoDisplayBpm_.load(std::memory_order_acquire); }
    bool TempoExternallyClocked() const { return tempoExternallyClocked_.load(std::memory_order_acquire); }
    // Task 3.6 (design E3e): published alongside tempoDisplayBpm_/
    // tempoExternallyClocked_ below, same cross-thread contract -- lets the
    // surface (message thread) know whether the transport is currently
    // running without reading `context_->masterClock` directly (audio-
    // thread-owned per AppContext.hpp). Used only to annotate the BPM
    // control, which has zero audible effect while the gate is closed
    // outright (ProcessBlock()'s own comment on `gateOpen`) -- no wiring
    // change, this is a read-only discoverability aid.
    bool TransportRunning() const { return transportRunningDisplay_.load(std::memory_order_acquire); }

    // A4 (tasks.md CONSOLIDATED PUSH, "surface partial randomize"): true when
    // the MOST RECENT Randomize All/Page operation left
    // `FroggersRandomizeResult.partial` true -- i.e. `EnsureModulationDepth`
    // hit `!group_.CanAllocate()` (Sheaf, ParameterModulation.cpp:2790-2792)
    // and stopped that operation short of drawing its full chosen set.
    // Published from ProcessFrame() (audio thread) alongside the
    // ComputeAllParameters() reseed below, same cross-thread contract as
    // TransportRunning() above -- makes a silent partial randomize
    // observable to tests, without inventing any new UI element the operator
    // did not ask for. Any operator-visible logging must read this atomic
    // from the UI thread -- ProcessFrame() itself never logs (F1: fprintf on
    // the audio thread can allocate/lock/block, which is a dropout risk).
    bool LastRandomizePartial() const { return lastRandomizePartial_.load(std::memory_order_acquire); }

    // Packet 10 (design D11/D14): detected via AppConcepts.hpp's
    // HasProcessFrame concept; synth::Engine invokes this once per block,
    // after message drains and before ProcessBlock() (AppConcepts.hpp's own
    // comment on the hook's placement) -- exactly the audio-thread window
    // the pending-request atomics above need to be applied in.
    void ProcessFrame() {
        if (!drillIn_.has_value()) {
            drillIn_.emplace(parameters_.BankAt(activeBankIx_));
        }

        const int bankRequest = pendingBankSelect_.exchange(-1, std::memory_order_acq_rel);
        if (bankRequest >= 0 && static_cast<std::size_t>(bankRequest) < kFroggersBankCount) {
            if (static_cast<std::size_t>(bankRequest) != activeBankIx_) {
                activeBankIx_ = static_cast<std::size_t>(bankRequest);
                parameters_.Slot().SelectBank(&parameters_.BankAt(activeBankIx_));
                // `BankSlot::SelectBank` Deselect()s the OUTGOING bank
                // (src/ParameterModulation.cpp:2924-2932 in External/Sheaf), so
                // a freshly-constructed drillIn_ (level_ starts at 0) for the
                // INCOMING bank is always consistent with that bank's real
                // state: either it was never drilled into, or it was
                // Deselect()ed the last time it was left active -- both are
                // real level 0. This is why exactly one drillIn_ instance,
                // reconstructed on every switch, never desyncs from six
                // persistent per-bank instances would risk.
                drillIn_.emplace(parameters_.BankAt(activeBankIx_));
            } else if (drillIn_->Level() > 0) {
                // E.3 (design A7b, operator override 2026-07-29): clicking
                // the bank you are ALREADY viewing must still be able to
                // back a modulation drilldown all the way out to that bank's
                // top-level parameter grid -- "clicking on the page bank for
                // the page we are on is the way the user should always be
                // able to get to that page, even when they are in a
                // modulation drilldown for a parameter on that page."
                // Back()-until-zero reaches the same "full Deselect(),
                // level 0" state a genuine bank switch produces above,
                // without reconstructing drillIn_ (same Bank&, no need) --
                // bounded to at most 2 iterations (design D5's level cap).
                // The `Level() > 0` guard is what keeps the pre-existing
                // no-op preserved for a same-bank click that is ALREADY at
                // level 0: nothing in this branch runs, so activeBankIx_/
                // drillIn_ are left completely undisturbed, same as before
                // this fix (rebuilding identical state would be wasted work
                // for no behavior change).
                while (drillIn_->Level() > 0) {
                    drillIn_->Back();
                }
            }
        }

        const int pressRequest = pendingEncoderPress_.exchange(-1, std::memory_order_acq_rel);
        if (pressRequest >= 0 && pressRequest < static_cast<int>(kFroggersSlotsPerBank)) {
            drillIn_->PressEncoder(static_cast<synth::PhysicalEncoderId>(pressRequest));
        }

        if (context_ != nullptr && context_->parameterManager != nullptr) {
            // A1/A2/A4 (tasks.md CONSOLIDATED PUSH): both branches below now
            // return a `FroggersRandomizeResult` whose `.partial` flag used
            // to be discarded entirely (W1.0 S3's "left un-surfaced" citation
            // of this exact call site) -- captured into `anyPartial` and
            // published below instead.
            bool randomizeRan = false;
            bool anyPartial = false;
            // F4: each call's result is hoisted into its own named local
            // BEFORE combining, so both RandomizeAll/RandomizePage always run
            // when their request is pending -- `anyPartial = a.partial ||
            // anyPartial` reads fine but is only correct because the call
            // sits on the left of `||`; swapping the combine order (or a
            // future edit that does) would short-circuit and skip the second
            // call whenever the first's `.partial` was already true.
            if (pendingRandomizeAll_.exchange(false, std::memory_order_acq_rel)) {
                const bool allPartial = RandomizeAll(*context_->parameterManager, *drillIn_, parameters_).partial;
                anyPartial = anyPartial || allPartial;
                randomizeRan = true;
            }
            if (pendingRandomizePage_.exchange(false, std::memory_order_acq_rel)) {
                const bool pagePartial = RandomizePage(*context_->parameterManager, *drillIn_).partial;
                anyPartial = anyPartial || pagePartial;
                randomizeRan = true;
            }
            if (randomizeRan) {
                // A4: surface a partial randomize -- observable to tests via
                // LastRandomizePartial(). Not a per-parameter/per-press
                // signal (design D14's own randomize helper can be called
                // dozens of times per operation); one publish per Randomize
                // All/Page press that actually ran short. F1: no log is
                // emitted here -- ProcessFrame() runs on the audio thread
                // (this method's own header comment), and any operator-
                // visible logging must instead read this atomic from the UI
                // thread.
                lastRandomizePartial_.store(anyPartial, std::memory_order_release);
                // A2 (W1.1a/W1.1d): the display-staleness fix. `Parameter::
                // RandomizeVisibleValue` writes `sceneCenters_` (the commanded
                // value) directly and immediately, but the drill-in knob
                // reads `uiDisplayCenters_`, which a depth parameter only
                // ever gets seeded into via a smoothed, one-shot nudge inside
                // RandomizeVisibleValue itself (Sheaf, pinned; see tasks.md
                // W1.0/W1.1a for the full trace) -- it is never touched again
                // by the per-sample loop, because depth parameters are not in
                // `topLevelParameters_`. `ComputeAllParameters()` (public,
                // ParameterModulation.hpp:796) is the one call that reseeds
                // it exactly, for every parameter including depth children
                // (ComputeAtDepth's recursionDepth_>0 branch takes the
                // instant snap-and-seed path, not the smoothed one -- see
                // W1.1a's derivation). Called ONCE here, after both branches
                // above have made every write for this frame's request(s),
                // never per-parameter and never from the UI thread:
                // ProcessFrame() itself only ever runs on the audio thread
                // (this method's own header comment; `synth::Engine` invokes
                // it once per block, after message drains and before
                // ProcessBlock()), and `ComputeAllParameters()` is a full,
                // non-lock-free graph traversal that `ParameterManager`
                // requires to run there (ParameterModulation.hpp:484-485).
                context_->parameterManager->ComputeAllParameters();
            }
        }

        const double tempoRequest = pendingTempoBpmRequest_.exchange(-1.0, std::memory_order_acq_rel);
        if (tempoRequest >= 0.0 && context_ != nullptr && context_->masterClock != nullptr) {
            context_->masterClock->SetTempoBpm(tempoRequest);
        }
    }

    // Packet 4 drives the per-sample parameter-model (scene-blend, fuego).
    // Packet 6 (this class's modulation_ member) steps the DSP behind the 15
    // modulation sources every sample, BEFORE parameters_.ProcessSample() so
    // its group_->UpdateModValues() call reads freshly-updated values through
    // the pointers registered in FroggersModulationSlate::Init. Task 6a
    // (design D15) then routes the now-resolved (post-fuego, post-modulation)
    // parameter values into the real DSP chain and sums it to the stereo
    // output bus, replacing task 2.1's silent placeholder.
    void ProcessBlock(synth::AudioBlock& block) {
        // Operator decision 2026-07-29: external audio defaults to NOT
        // connected.
        //
        // This used to ask only "did the device hand us an input channel?",
        // which on a laptop is ALWAYS true -- the built-in mic presents an
        // input channel whether or not anything is plugged in (this machine's
        // own startup log reads "1 in / 2 out" with nothing attached). That
        // marked BOTH external modulation sources -- slots 13/14, external
        // audio and its envelope follower -- permanently connected, so
        // Randomize kept assigning depths to sources carrying nothing but mic
        // noise. Sheaf's randomizer is not at fault: it correctly picks only
        // among sources whose metadata says `connected`
        // (src/ParameterModulation.cpp:2886-2895) -- the app was lying to it.
        //
        // "A channel exists" and "the operator routed something in" are
        // different questions, and the app cannot currently tell them apart:
        // the selected input device name lives runtime-side
        // (Runtime.hpp's AudioDeviceSnapshot) and `AppContext` exposes no
        // audio-device state at all. Until Sheaf surfaces that (see
        // UPSTREAM-SHEAF-ASK.md), the honest default is OFF -- an unusable
        // external source beats a phantom one that steals randomization.
        //
        // Flip `kExternalAudioOptedIn` when there is a real opt-in signal to
        // gate on; the channel plumbing below is left intact so that is a
        // one-line change rather than a rewrite.
        constexpr bool kExternalAudioOptedIn = false;
        const bool externalInputHasChannel =
            block.inputs != nullptr && block.numInputChannels > 0 && block.inputs[0] != nullptr;
        const bool externalInputConnected = kExternalAudioOptedIn && externalInputHasChannel;

        // Task 8.1 (design D8/D8a): source #6's tempo-following recompute
        // happens ONCE PER BLOCK, not per sample (design D8's own wording),
        // from the block's clock-plan rate (independent of transport
        // running/stopped -- a rate, not a position). A null clock plan
        // falls back to a safe default inside PrepareBlockClock() itself.
        std::optional<double> quarterNotesPerSample;
        if (block.clockPlan != nullptr) {
            quarterNotesPerSample = block.clockPlan->QuarterNotesPerSample();
        }
        modulation_.PrepareBlockClock(quarterNotesPerSample);

        for (std::size_t frame = 0; frame < block.numFrames; ++frame) {
            const std::uint64_t absoluteOutputSample = block.startSample + frame;
            const float externalAudioSample =
                externalInputConnected ? block.inputs[0][frame] : 0.0f;

            // Task 6b (design D17, revised 2026-07-27): the ASR gate follows
            // the master clock's transport quarter-note pulse -- see
            // TransportQuarterNotesAt()'s own comment for the citation chain
            // (apps/miniapp/MiniAppCore.hpp's gate idiom, adapted to the
            // containment-safe Try accessor). Gated open for the first half
            // of every quarter note, closed for the second half; closed
            // outright whenever the transport isn't running or the block has
            // no clock plan.
            const std::optional<double> transportQuarterNotes =
                TransportQuarterNotesAt(block, absoluteOutputSample);
            bool gateOpen = false;
            if (transportQuarterNotes.has_value()) {
                const double phase = *transportQuarterNotes - std::floor(*transportQuarterNotes);
                gateOpen = phase >= 0.0 && phase < 0.5;
            }
            audioAdsr_.setGate(gateOpen);

            // Stop-transport reset (see wasTransportRunning_'s own comment):
            // `transportQuarterNotes.has_value()` above is already exactly
            // "transport running AND this sample is contained in a committed
            // plan" (TransportQuarterNotesAt()'s own comment), so the
            // running->stopped edge is read off that same already-computed
            // value rather than a second clock query. This runs on the
            // audio thread, inside the same per-sample loop that owns
            // delay_/reverb_ (RouteAudioSample(), below, is the one and only
            // caller of both), so there is no data race with the UI thread's
            // kStop handler (FroggersUiSurface.hpp's HandleAction), which
            // only pushes a MessageIn::Stop and never touches DSP state
            // directly. Resets only delay_/reverb_ -- the two feedback
            // structures that self-sustain on their own; VCOs/filters/drive
            // do not, so resetting them here would be unrequested work.
            const bool transportRunningNow = transportQuarterNotes.has_value();
            if (wasTransportRunning_ && !transportRunningNow) {
                // ITEM 1 (revised, superseding the old "clear every block
                // while releasing" policy): a releasing voice is still
                // musically live and still feeding delay_/reverb_
                // (RouteAudioSample(), below, runs every sample regardless
                // of transport) -- it is supposed to keep ringing through
                // them wet, exactly as it would if the transport were still
                // running. So this edge does NOT clear unconditionally.
                // `audioAdsr_.setGate(gateOpen)` above already ran for this
                // sample with the transport-forced-closed gate, so
                // AllIdle() here already reflects the post-edge stage:
                // false if that gate transition just moved a voice into
                // Stage::Release (setGate() reassigns every voice's stage on
                // a high->low transition, VoiceEnvelope.hpp:62-73 -- none of
                // them can already be Idle in that case), true only if the
                // gate was already closed *before* this edge (e.g. the
                // transport stopped during the closed half of the ASR gate
                // cycle) and every voice had already finished its Release
                // naturally.
                if (audioAdsr_.AllIdle()) {
                    // Nothing is left to inject further energy into
                    // delay_/reverb_ -- gate closed, every voice Idle -- so
                    // one clear right here is permanent. This is the
                    // "already idle at the stop edge" case: with no future
                    // Release to wait for, the pending-clear-on-AllIdle
                    // logic below would never get a false->true transition
                    // to fire on, so the clear has to happen here instead.
                    delay_.ClearBuffers();
                    reverb_.Reset();
                    delayReverbClearPending_ = false;
                } else {
                    // Still releasing: defer. Arms the sample-accurate watch
                    // below for the one clear this policy owes, fired the
                    // instant AllIdle() first turns true while still
                    // stopped.
                    delayReverbClearPending_ = true;
                }
            } else if (delayReverbClearPending_) {
                if (transportRunningNow) {
                    // Transport resumed mid-release: this is now a
                    // legitimately playing delay/reverb, not a stale
                    // release tail -- cancel instead of wiping it.
                    delayReverbClearPending_ = false;
                } else if (audioAdsr_.AllIdle()) {
                    // The moment every voice reaches Idle while still
                    // stopped: the release can no longer inject anything
                    // into delay_/reverb_, so this single clear is
                    // permanent. Checked per-sample (cheap: AllIdle() is
                    // just three stage comparisons) so the expensive part
                    // (ClearBuffers()/Reset(), O(capacity) fills,
                    // dsp/Delay.hpp:77-84) still runs exactly once for the
                    // whole release, not once per block for up to
                    // kMaxReleaseSeconds (10s, VoiceEnvelope.hpp:36) the way
                    // the old policy did.
                    delay_.ClearBuffers();
                    reverb_.Reset();
                    delayReverbClearPending_ = false;
                }
            }
            wasTransportRunning_ = transportRunningNow;

            // VCO audio-rate modulation sources (design D5 slots 6-8) are
            // driven from each VCO's own Audio-bank pitch/shape/PM knobs --
            // the PREVIOUS sample's post-fuego cached value (this Step()
            // call runs before this sample's own fuego/modulation resolve;
            // see FroggersModulationSlate::Step's comment on the resulting
            // one-sample latency, standard for any modulation graph with a
            // cycle -- e.g. task 6.12's cross-VCO pitch default).
            const auto vcoDrive = [this](std::size_t paramIx) {
                return FroggersModulationSlate::VcoDrive{
                    parameters_.PageParameter(FroggersBankId::Audio, paramIx).CachedKnobValue(0),
                    parameters_.PageParameter(FroggersBankId::Audio, paramIx + 3).CachedKnobValue(0),
                    parameters_.PageParameter(FroggersBankId::Audio, paramIx + 6).CachedKnobValue(0),
                };
            };
            // Task 8.1/6b.3: the SAME already-computed transportQuarterNotes
            // (above, for the ASR gate) is reused here rather than a second
            // null-check/guard/Try-call sequence -- see Step()'s own comment.
            modulation_.Step(vcoDrive(0), vcoDrive(1), vcoDrive(2), externalAudioSample, externalInputConnected,
                              transportQuarterNotes);

            parameters_.ProcessSample(absoluteOutputSample);

            // Task 6a.1/6a.2: route this sample's post-fuego, post-modulation
            // parameter values into the ported DSP chain and sum the (mono,
            // per D4's kNumVoices==1 model) result to every output channel --
            // matching FroggersEngine::ProcessSample's own float-in/float-out
            // shape (FroggersEngine.hpp:850-874), which this app's own stereo
            // bus duplicates identically on both channels.
            const float sample = RouteAudioSample();

            if (block.outputs != nullptr) {
                for (int channelIx = 0; channelIx < block.numOutputChannels; ++channelIx) {
                    float* const channel = block.outputs[static_cast<std::size_t>(channelIx)];
                    if (channel != nullptr) {
                        channel[frame] = sample;
                    }
                }
            }

            // Task 1.1 (design E1): move the scope ring-buffer cursor once
            // per sample -- Sheaf's ScopeWriter needs both Write() (called
            // per-sample inside RouteAudioSample(), above, on the POST-gate
            // values -- UI-rework ITEM 3, design.md A3d, moved this off
            // dsp::Vco::Process() itself, see that struct's own comment) and
            // AdvanceIndex() (index_ += amount, DspScope.hpp:126-128).
            // Mirrors Braid 4's own placement: AdvanceIndex() runs at the
            // end of its per-sample work, after that sample's audio/matrix
            // outputs are computed and published but before the per-sample
            // function returns (Braid4Core.hpp:487, immediately preceding
            // RecordInternalIndex()+return). Here the equivalent slot is
            // the end of this per-frame loop's body, after this sample's
            // output has been computed and written.
            vcoScopeWriter_.AdvanceIndex();
        }

        // ITEM 1: no once-per-block clearing step here anymore -- the single
        // clear this policy owes (fired either at the running->stopped edge
        // if voices were already Idle, or the instant AllIdle() first turns
        // true afterward) is now detected and performed sample-accurately
        // inside the per-sample loop above, at the same place the
        // running->stopped edge itself is detected. See that block's own
        // comment.

        // Tasks 2.2-2.5 (Tier 1/Tier 2 per-unit recovery): see
        // RecoverPoisonedUnitState()'s own header comment for the full
        // design/trace. Runs once per block, after the per-sample loop, over
        // every unit Item 2 gave a Reset() to -- this IS the mechanism that
        // fixes "audio never comes back": before this call existed, nothing
        // anywhere in this file ever reset audioVco1_/audioVco2_/audioVco3_/
        // driveBlendPhase_/drive_'s sub-units/filterChain_'s sub-units, so a
        // poisoned recursive state in any of them (e.g. a biquad whose y1/y2
        // went non-finite, or diverged to an extreme finite magnitude) would
        // keep feeding forward into every future sample forever, regardless
        // of what the operator changed the knobs to afterward -- only
        // SanitizeOutputSample's clamp/zero at the very end of the chain
        // masked the symptom, it never cleared the cause.
        RecoverPoisonedUnitState(block.numFrames);

        // Task 7.2/8.3/9.3: publish this block's UI-facing state, once per
        // block after the per-sample loop -- the same end-of-ProcessBlock
        // placement apps/braid-4's own ProcessBlock uses for its
        // scopeWriter_.Publish()/PopulateUIState()/PublishUiState() sequence
        // (Braid4Core.hpp:252-262).
        vcoScopeWriter_.Publish();
        audioVco1_.PopulateUIState(vco1ScopeUiState_);
        audioVco2_.PopulateUIState(vco2ScopeUiState_);
        audioVco3_.PopulateUIState(vco3ScopeUiState_);
        filterChain_.peak.PopulateUIState(peakUiState_);
        filterChain_.comb.PopulateUIState(combUiState_);
        modulation_.PublishUiState();

        // Packet 10: publish the master clock's active tempo/external-slave
        // state for the surface to read cross-thread (see this file's
        // header comment).
        if (context_ != nullptr && context_->masterClock != nullptr) {
            tempoDisplayBpm_.store(context_->masterClock->TempoBpm(), std::memory_order_release);
            tempoExternallyClocked_.store(context_->masterClock->SyncConfiguration().receiveClock,
                                          std::memory_order_release);
            // Task 3.6 (design E3e): same publish-once-per-block pattern as
            // the two stores above, for TransportRunning()'s cross-thread read.
            transportRunningDisplay_.store(
                context_->masterClock->TransportState() == synth::ClockTransportState::Running,
                std::memory_order_release);
        }
    }

    // Test/inspection access to the packet-7 VCO scope plumbing (task 7.4).
    synth::ui::Visualizer& VcoScopeVisualizer() { return vcoScopeVisualizer_; }
    const dsp::Vco::UIState& VcoScopeUiState(std::size_t vcoIx) const {
        switch (vcoIx) {
            case 0: return vco1ScopeUiState_;
            case 1: return vco2ScopeUiState_;
            default: return vco3ScopeUiState_;
        }
    }

    // Test/inspection access to the packet-9 transfer-function visualizers.
    synth::ui::Visualizer& PeakVisualizer() { return peakVisualizer_; }
    synth::ui::Visualizer& CombVisualizer() { return combVisualizer_; }

    // Test/inspection access to the packet-4 parameter/bank model (also used
    // by later packets to wire fuego/modulation/UI).
    FroggersParameterModel& Parameters() { return parameters_; }

    // Test/inspection access to the packet-6 modulation slate.
    FroggersModulationSlate& Modulation() { return modulation_; }

    // Test/inspection access to packet 10's own bookkeeping.
    std::size_t ActiveBankIndex() const { return activeBankIx_; }
    FroggersModulationDrillIn& ActiveDrillIn() { return *drillIn_; }

    // Test/inspection access to tasks 2.2-2.5's per-unit recovery targets --
    // the exact dsp:: unit instances RecoverPoisonedUnitState() watches, so
    // tests can inject a poisoned (non-finite or over-ceiling) state
    // directly into ONE unit and observe (a) that unit's own recovery
    // firing through the real ProcessBlock()/RunBlocks() path and (b) every
    // OTHER unit's state staying untouched -- same "Test/inspection access"
    // convention as VcoScopeUiState()/PeakVisualizer() above.
    dsp::Vco& TestAudioVco(std::size_t ix) {
        switch (ix) {
            case 0: return audioVco1_;
            case 1: return audioVco2_;
            default: return audioVco3_;
        }
    }
    dsp::DriveBlendPhase& TestDriveBlendPhase() { return driveBlendPhase_; }
    dsp::Oversampler2x& TestDriveOversampler() { return drive_.oversampler; }
    dsp::SampleRateReducer& TestSampleRateReducer(std::size_t ix) {
        return ix == 0 ? drive_.sampleRateReducer1 : drive_.sampleRateReducer2;
    }
    dsp::ResonantBump& TestFilterPeak() { return filterChain_.peak; }
    dsp::ResonantBump& TestFilterScoopNotch() { return filterChain_.scoopNotch; }
    dsp::Comb& TestFilterComb() { return filterChain_.comb; }
    // B5: test/inspection access to the peak branch's OWN limiter instance
    // (`FilterFxChain::peakLimiter`, `dsp/FilterFx.hpp`) -- same convention
    // as `TestOutputLimiter()` above, but for the independently-tuned
    // second instance rather than the master.
    dsp::OutputLimiter& TestFilterPeakLimiter() { return filterChain_.peakLimiter; }
    // Item 3: test/inspection access to the MASTER output limiter, same
    // convention as the accessors above -- lets tests call Process()/
    // Reset() directly and read `envelope` without going through the full
    // RouteAudioSample chain, e.g. to prove the bit-identical-passthrough
    // acceptance test. `auto&` kept as-is post-B5 (`dsp::OutputLimiter` is
    // now a public type, so `dsp::OutputLimiter&` would spell fine too, but
    // every other TestXxx() accessor above already returns `dsp::Xxx&` by
    // deduction-free convention -- `auto&` here is simply consistent with
    // those, not a workaround for privacy anymore). Distinct from the peak
    // branch's OWN limiter instance, which has its own accessor
    // (`TestFilterPeakLimiter()`, mirroring `TestFilterPeak()` above).
    auto& TestOutputLimiter() { return outputLimiter_; }

    // F3.1 (frogg3rs-blowout-and-drilldown-repair): test/inspection access
    // to `delay_`/`reverb_`, same convention as the accessors above --
    // added because the Stop-flush measurement harness needs to read their
    // internal state magnitude directly (their delay lines/tanks, not just
    // the finiteness `RecoverIfNonFinite` already checks) to tell "cleared
    // once and stayed clear" apart from "cleared once, then refilled by a
    // still-ringing upstream unit." Read-only; does not change either
    // unit's behaviour.
    dsp::StereoDelay& TestDelay() { return delay_; }
    dsp::Reverb& TestReverb() { return reverb_; }

    // B7.5: test/inspection access to the live synth::ParameterManager, same
    // convention as TestOutputLimiter() above. `context_` (below) is
    // private and there is no other public route to it; this is the
    // narrowest accessor that reaches ComputeAllParameters(), which
    // FroggersAudioRoutingTests.cpp's ApplyPatchNow() needs to make a
    // SceneCenter write converge exactly before the first RunBlocks()
    // (B7.5.0's settling rule).
    synth::ParameterManager& TestParameterManager() { return *context_->parameterManager; }

private:
    synth::AppContext* context_ = nullptr;

    // Task 6b (design D17, revised 2026-07-27): the single clock-read call
    // site for this class. Returns the transport quarter-note position at
    // `absoluteOutputSample`, or nullopt when the transport isn't running or
    // the committed plan doesn't contain the sample. Null-checking
    // `block.clockPlan` (`AppContext.hpp:84`) is necessary but not
    // sufficient for containment, so this calls the containment-safe
    // `TryTransportQuarterNotesAt` (`MasterClock.hpp:200`) rather than the
    // precondition-carrying `TransportQuarterNotesAt` (`:198`, precondition
    // `Contains(...)`, `:192-198`) -- the same shape
    // `apps/miniapp/MiniAppCore.hpp`'s own ADSR-gate idiom follows (guard
    // `:323-324`, phase derivation `:325-327`, duty-cycle assignment `:328`),
    // substituting the Try accessor for miniapp's unchecked one. Factored
    // into its own method (rather than inlined at the gate's one call site in
    // ProcessBlock) so packet 8's master-clock-driven Marbles advance (design
    // D8/D8a) can share this exact read instead of re-deriving the
    // null-check/guard/Try-call sequence a second time.
    static std::optional<double> TransportQuarterNotesAt(const synth::AudioBlock& block,
                                                          std::uint64_t absoluteOutputSample) {
        if (block.clockPlan == nullptr ||
            block.clockPlan->TransportState() != synth::ClockTransportState::Running) {
            return std::nullopt;
        }
        return block.clockPlan->TryTransportQuarterNotesAt(static_cast<double>(absoluteOutputSample));
    }

    // Task 6a.1-6a.3 (design D15): the real audio path -- Audio/Envelope
    // banks -> 3x dsp::Vco + dsp::MixOscVoices (packet 3 tasks 3.1/3.2) ->
    // Drive bank -> dsp::FrogBlock + dsp::DriveBlendPhase (task 3.9) ->
    // Filter bank -> dsp::FilterFxChain (task 3.6) -> Delay bank ->
    // dsp::StereoDelay (task 3.10) -> Reverb bank -> dsp::Reverb (task 3.8).
    //
    // Ordering proof (verified by reading the cited source, not assumed):
    // `Parameter::GetRaw()` (External/Sheaf's
    // projects/synth/src/ParameterModulation.cpp:1207-1215) sums the
    // scene-blended center with `Modulators::ApplyActive()` -- i.e.
    // modulation-depth routing is already baked in there. `Parameter::
    // ProcessLitePhase1()` (:1459-1461) writes `currentKnobValues_[v] =
    // GetRaw(v)`, and `ParameterGroup::ProcessSamplePhase1()` calls that for
    // every parameter (:867-870) -- so by the time `FroggersParameterModel::
    // ApplyFuegoSeam()` runs (between Phase1 and Phase2, FroggersParameters.
    // hpp), `Parameter::CachedKnobValue()` already reflects modulation, and
    // ApplyFuegoSeam() then overwrites it with the fuegoized value via
    // `ReplaceCachedKnobValue()`. `ProcessSamplePhase2()` ->
    // `ProcessLitePhase2()` (:1471-1479) only slews `uiDisplayCenters_` from
    // `currentKnobValues_` and never rewrites the latter, so the cache is
    // unaffected by Phase2. Every `CachedKnobValue()` read below -- taken
    // after `parameters_.ProcessSample()` has returned for this sample -- is
    // therefore post-modulation AND post-fuego, exactly as design D15
    // requires; if this were not achievable as structured, this packet's
    // brief called for stopping and reporting rather than reading raw
    // values, which is why this citation chain exists.
    float RouteAudioSample() {
        auto knob = [this](FroggersBankId bank, std::size_t ix) -> float {
            return parameters_.PageParameter(bank, ix).CachedKnobValue(0);
        };

        // -- Audio bank -> 3x dsp::Vco (task 3.1) ---------------------------
        // Slots: VCO pitch 0-2, Shape (morph) 3-5, Phase mod 6-8 -- same
        // (paramIx, +3, +6) grouping ProcessBlock's own vcoDrive lambda uses
        // above for the modulation slate's separate VCO instances.
        const float v1 = audioVco1_.Process(knob(FroggersBankId::Audio, 0),
                                             knob(FroggersBankId::Audio, 3),
                                             knob(FroggersBankId::Audio, 6),
                                             sampleRate_);
        const float v2 = audioVco2_.Process(knob(FroggersBankId::Audio, 1),
                                             knob(FroggersBankId::Audio, 4),
                                             knob(FroggersBankId::Audio, 7),
                                             sampleRate_);
        const float v3 = audioVco3_.Process(knob(FroggersBankId::Audio, 2),
                                             knob(FroggersBankId::Audio, 5),
                                             knob(FroggersBankId::Audio, 8),
                                             sampleRate_);

        // -- Envelope bank -> ASR + voice mix (task 3.2) --------------------
        // Slots: Attack/Sustain/Release x VCO1-3, 0-8 in that order --
        // matches dsp::MixOscVoices's parameter order exactly.
        //
        // UI-rework ITEM 3 (design.md A3d, tasks.md B.3, 2026-07-29):
        // `gatedVoices` receives the POST-gate per-voice values via
        // MixOscVoices's out-parameter (VoiceEnvelope.hpp) -- written to the
        // scope just below instead of the pre-gate raw VCO output
        // Vco::Process() used to write (see that struct's own comment for
        // why). This is the single call site the out-parameter was added
        // for; do not re-apply `adsr.apply` anywhere else (OMNI §8).
        // Stop-must-stop fix (operator bug report 2026-07-29: "the stop button
        // doesnt actually stop all audio in all circumstances"). Closing the
        // transport gate puts every voice into Stage::Release honouring the
        // PATCH's release knob -- up to kMaxReleaseSeconds. So Stop used to
        // begin a multi-second fade during which the voices kept re-exciting
        // the delay and reverb, and ProcessBlock's clear-at-AllIdle could not
        // fire until that fade finished. Lowering kMaxReleaseSeconds does not
        // fix this; even 5s reads as "Stop is broken".
        //
        // While the transport is STOPPED, substitute a fast release. The
        // operator's own release setting is untouched and applies normally the
        // moment the transport runs again -- this only overrides the knob on
        // the stop path. ~50ms is short enough to read as immediate and long
        // enough to avoid a click; an instantaneous cut would click.
        //
        // Derived from VcoAdsrState's own mapping rather than hardcoded, so it
        // stays a true 50ms if kMaxReleaseSeconds is ever retuned again:
        //   mapRelease(k) = kMinTimeSeconds + k*(kMaxReleaseSeconds - kMinTimeSeconds)
        //
        // `wasTransportRunning_` is written earlier in this same per-sample
        // iteration (ProcessBlock's edge check, above this call), so it holds
        // THIS sample's transport state, not the previous one's.
        constexpr float kStopFadeSeconds = 0.05f;
        constexpr float kStopFadeReleaseKnob =
            (kStopFadeSeconds - dsp::VcoAdsrState::kMinTimeSeconds) /
            (dsp::VcoAdsrState::kMaxReleaseSeconds - dsp::VcoAdsrState::kMinTimeSeconds);
        const auto releaseKnob = [&](std::size_t slot) -> float {
            return wasTransportRunning_ ? knob(FroggersBankId::Envelope, slot) : kStopFadeReleaseKnob;
        };

        dsp::GatedVoices gatedVoices;
        const float chainIn = dsp::MixOscVoices(
            audioAdsr_, v1, v2, v3,
            knob(FroggersBankId::Envelope, 0), knob(FroggersBankId::Envelope, 1), releaseKnob(2),
            knob(FroggersBankId::Envelope, 3), knob(FroggersBankId::Envelope, 4), releaseKnob(5),
            knob(FroggersBankId::Envelope, 6), knob(FroggersBankId::Envelope, 7), releaseKnob(8),
            &gatedVoices);

        // Post-gate scope tap (design.md A3d): write the GATED values, not
        // the raw pre-gate VCO output -- so the scope stays flat until the
        // transport's ASR gate has actually opened at least one voice.
        vco1ScopeHolder_.Write(gatedVoices.v1);
        vco2ScopeHolder_.Write(gatedVoices.v2);
        vco3ScopeHolder_.Write(gatedVoices.v3);

        // -- Drive bank -> dsp::FrogBlock + DriveBlendPhase (task 3.9) ------
        // FroggersEngine.hpp:483-490 order: Drive (SetGain) before Shape
        // (SetCoefs, which reads the just-set gain target -- Drive.hpp's own
        // comment), then SRR1/SRR2/XOR/BitDepth/Fuzz; Blend/Phase (slots 7-8)
        // are the authored DriveBlendPhase stage, crossfading dry (chainIn)
        // against wet (FrogBlock's output).
        drive_.polynomialDrive.SetGain(knob(FroggersBankId::Drive, 0));
        drive_.polynomialDrive.SetCoefs(knob(FroggersBankId::Drive, 1));
        drive_.sampleRateReducer1.SetFreq(
            1e-2f + dsp::ZeroedExpCompute(10.0f, 1.0f - knob(FroggersBankId::Drive, 2)));
        drive_.sampleRateReducer2.SetFreq(
            1e-2f + dsp::ZeroedExpCompute(10.0f, 1.0f - knob(FroggersBankId::Drive, 3)));
        drive_.digitalReorganizer.SetFlip(knob(FroggersBankId::Drive, 4));
        drive_.digitalReorganizer.SetHash(knob(FroggersBankId::Drive, 5));
        drive_.fuzz = knob(FroggersBankId::Drive, 6);
        const float driveWet = drive_.Process(chainIn);
        const float driveOut = driveBlendPhase_.Process(
            chainIn, driveWet, knob(FroggersBankId::Drive, 7), knob(FroggersBankId::Drive, 8));

        // -- Filter bank -> dsp::FilterFxChain (task 3.6) -------------------
        // FroggersEngine.hpp:463,465-481 mapping (Comb offset -> pureDelay,
        // Peak freq/gain/Q -> ResonantBump, Comb delay/feedback/LP -> Comb,
        // Comb/Peak -> blend, Scoop -> scoopMix). `useParallel` mirrors
        // `SetUseV2FilterParallel(UsesV2Fuego(hostKind))`
        // (src/core/DesktopHostIO.hpp:330, PagedHostIO.hpp:81) -- the same
        // flag class that gates `SetSimIndependentPm` (which task 3.1's Vco
        // port always takes, D7) and `DelayState::setUseV2Layout` (which
        // Delay.hpp's port always takes, "this app IS the V2 layout"); this
        // app is a V2-fuego host in every other place that flag appears, so
        // it is `true` here too. Clamped to `PureDelay::kSize` so a very high
        // host sample rate cannot push `SetDelaySeconds` past the ported
        // unit's fixed-size ring buffer (defensive; PureDelay itself,
        // packet 3, is not modified).
        const float combOffsetSeconds = std::min(
            dsp::ExpMapCompute(0.001f, 0.1f, knob(FroggersBankId::Filter, 0)),
            static_cast<float>(dsp::PureDelay::kSize - 1) / sampleRate_);
        filterChain_.pureDelay.SetDelaySeconds(combOffsetSeconds, sampleRate_);
        // FroggersEngine.hpp:561-562: the scoop notch shares the peak bump's
        // freq and width verbatim -- hoisted into locals because BOTH
        // ResonantBumps consume them.
        const float bumpFreq =
            dsp::ExpMapCompute(20.0f / sampleRate_, 20000.0f / sampleRate_, knob(FroggersBankId::Filter, 1));
        const float bumpWidth = dsp::ExpMapCompute(0.1f, 10.0f, knob(FroggersBankId::Filter, 3));
        filterChain_.peak.SetFreq(bumpFreq);
        // ITEM 2 (design.md A2, 2026-07-29, deliberate parity divergence --
        // same treatment as Fuegoize.hpp's own D6 note): ceiling lowered
        // from 10x (+20 dB) to 4x (+12 dB). An audible resonant peak does
        // not need a 20 dB multiplier sitting on top of a comb that (post
        // item 1) can still ring for seconds -- 10x was the primary gain
        // offender in the operator's blowout (design.md A1: stage 5 of 10,
        // multiplying the comb's saturator-pinned output before the
        // limiter). scoopNotch (below) shares this same freq/width but its
        // own height is a DIP (max(0.05, 1-0.95*scoop)), not a gain, so it
        // is unaffected and untouched.
        // Item 2 (design A2): ceiling 4.0f (+12 dB), NOT the frozen firmware's 10.0f
        // (+20 dB). A pinned self-oscillating comb through a 20 dB peak is what put
        // ~20x full scale into the output stage. Deliberate divergence from the port
        // source -- parity deprioritised by operator decision 2026-07-28.
        // Lowered again 2026-07-29, operator on hearing it: 4x "is still too
        // harsh when modulated/randomized ... gets very close to blowout
        // territory anyway". 10x -> 4x fixed the gross overload; 4x -> 2x
        // targets the harshness that remains.
        //
        // Why modulation is the case that matters: the knob is a modulation
        // TARGET, so a randomized depth sweeps it to maximum regularly rather
        // than only when the operator dials it there. The comb feeding this
        // stage is bounded near |in| + 0.95 (~2 at full scale), so 4x handed
        // the output stage ~8 -- about 9x over the limiter's 0.9 threshold,
        // which means the limiter rides hard and continuously, and heavy
        // sustained gain reduction is itself the harshness. 2x (+6 dB) roughly
        // halves how hard it has to work while still being an audible
        // resonant peak.
        //
        // If it is STILL harsh, the next lever is the comb feedback (0.95)
        // that feeds it, not this ceiling -- past a point, lowering this just
        // makes the resonance inaudible.
        filterChain_.peak.SetHeight(
            dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, knob(FroggersBankId::Filter, 2)));
        filterChain_.peak.SetWidth(bumpWidth);
        // FroggersEngine.hpp:561-564, restored 2026-07-27. These three
        // setters were dropped in the port even though `FilterFxChain`
        // kept the scoop blend, so `scoopNotch` ran forever on
        // `ResonantBump`'s default `freq = 1000.0f` (FilterFx.hpp:133) --
        // an unnormalized value in a cycles/sample convention, i.e.
        // thousands of times past Nyquist, giving a marginally stable
        // biquad. Under a self-oscillating comb (Randomize All pins the
        // feedback near its -1.1 extreme) its state diverged to non-finite,
        // and `SanitizeOutputSample` then masked every later sample to
        // 0.0f: permanent silence with no recovery. Note `scoopNotch`
        // processes unconditionally, so a poisoned state reaches the output
        // even at `scoopMix == 0` -- IEEE `NaN * 0` is `NaN`.
        // Height is a DIP, not a gain: max(0.05, 1 - 0.95 * scoop).
        filterChain_.scoopNotch.SetFreq(bumpFreq);
        filterChain_.scoopNotch.SetWidth(bumpWidth);
        filterChain_.scoopNotch.SetHeight(
            std::max(0.05f, 1.0f - 0.95f * knob(FroggersBankId::Filter, 8)));
        const float combFreq =
            dsp::ExpMapCompute(20.0f / sampleRate_, 10000.0f / sampleRate_, knob(FroggersBankId::Filter, 4));
        filterChain_.comb.delaySamples = std::min<std::size_t>(
            dsp::Comb::kSize - 1,
            std::max<std::size_t>(1, static_cast<std::size_t>(dsp::Comb::GetDelaySamples(combFreq))));
        filterChain_.comb.SetFeedback(dsp::Comb::GetFeedback(knob(FroggersBankId::Filter, 5)));
        const float cmlp =
            dsp::ExpMapCompute(4.0f * combFreq, 20000.0f / sampleRate_, knob(FroggersBankId::Filter, 6));
        // FroggersEngine.hpp:430-432 (Alpha): 1 - exp(-2*pi*natFreq).
        filterChain_.comb.SetCutoffAlpha(1.0f - std::exp(-2.0f * static_cast<float>(M_PI) * cmlp));
        const float filterOut = filterChain_.Process(
            driveOut, /*useParallel=*/true, knob(FroggersBankId::Filter, 7), knob(FroggersBankId::Filter, 8));

        // -- Delay bank -> dsp::StereoDelay (task 3.10) ---------------------
        // Positioned exactly where the frozen engine's `m_simFxInsert` hook
        // sits: FroggersEngine.hpp:840-843, between the filter chain
        // (:824-839) and Reverb (:844-847) -- confirmed by
        // `sim/WasmSimHost.hpp:34` (`io.m_engine.SetSimFxInsert(
        // simDelayInsertCallback, &delay)`), which wires this exact ported
        // unit's frozen counterpart (`DelayState::processInsert`,
        // `sim/DelayState.hpp:165-198,334`) into that hook. `processInsert`'s
        // own shape is `delay.process(bumpIn, params)` then
        // `delay.toReverbMono(bumpIn, wet, params.dmix)` (:196-198),
        // reproduced identically below.
        const dsp::DelayParams delayParams = dsp::MapRowsToDelayParams(
            knob(FroggersBankId::Delay, 0), knob(FroggersBankId::Delay, 1), knob(FroggersBankId::Delay, 2),
            knob(FroggersBankId::Delay, 3), knob(FroggersBankId::Delay, 4), knob(FroggersBankId::Delay, 5),
            knob(FroggersBankId::Delay, 6), knob(FroggersBankId::Delay, 7), knob(FroggersBankId::Delay, 8));
        const dsp::DelayWetPair delayWet = delay_.Process(filterOut, delayParams);
        const float delayOut = delay_.ToReverbMono(filterOut, delayWet, delayParams.dmix);

        // -- Reverb bank -> dsp::Reverb (task 3.8) --------------------------
        // Last stage, matching FroggersEngine.hpp:844-847's wet/dry blend
        // (folded into Reverb::Process's own return -- see that struct's
        // header comment).
        // Wet/dry ceiling (operator 2026-07-29: "clamp the reverb wetness down
        // to 70% of its current maximum, it's too fucking quiet"). Reverb's
        // blend is `(1-mix)*dry + mix*wet` (dsp/Reverb.hpp), so mix == 1.0
        // removes the dry signal ENTIRELY and leaves only the diffuse tail --
        // which reads as a big drop in level, not as more reverb. Capping the
        // mix at 0.7 keeps at least 30% dry in the output at every knob
        // position, so turning the control up adds tail instead of trading
        // away the source. Applied to the mapped value, not the knob range, so
        // the control still sweeps its whole travel.
        constexpr float kMaxReverbWetMix = 0.7f;
        const float reverbOut = reverb_.Process(
            delayOut,
            kMaxReverbWetMix * knob(FroggersBankId::Reverb, 0),
            knob(FroggersBankId::Reverb, 1), knob(FroggersBankId::Reverb, 2),
            knob(FroggersBankId::Reverb, 3), knob(FroggersBankId::Reverb, 4), knob(FroggersBankId::Reverb, 5),
            knob(FroggersBankId::Reverb, 6), sampleRate_,
            knob(FroggersBankId::Reverb, 7), knob(FroggersBankId::Reverb, 8));

        return SanitizeOutputSample(reverbOut);
    }

    // ITEM 3 (design.md A2/A2a, 2026-07-29): output-stage limiter.
    // Supersedes task 2.8's unconditional hard clamp (2026-07-28 revision,
    // below in `SanitizeOutputSample`'s own comment for the historical
    // record) -- design.md A4 records the clamp as superseded, and the
    // `frogg3rs-dsp-recovery` spec requirement forbidding soft-knee
    // limiting is superseded along with it.
    //
    // GAIN REDUCTION, not per-sample waveshaping: `Process()` computes ONE
    // scalar gain per sample from a smoothed envelope of the input's own
    // instantaneous magnitude and multiplies the sample by it, rather than
    // reshaping the sample the way a saturator/waveshaper would. Below
    // `threshold`, the target gain is mathematically exactly 1.0f (see
    // `DesiredMagnitude()`), and `1.0f * x == x` is an IEEE-754 identity
    // (exact, no rounding) -- so once `envelope` itself settles to exactly
    // 1.0f (its `Reset()`/initial value, never disturbed while every sample
    // stays under threshold, per the Sterbenz-lemma argument in the
    // struct's own `Process()` comment), passthrough is bit-identical. That
    // is this stage's own acceptance test (design.md A2a).
    //
    // B5 (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/tasks.md):
    // the struct itself now lives in `dsp::OutputLimiter`
    // (`dsp/Limiter.hpp`), not here -- it was a PRIVATE nested type until
    // B5 needed a SECOND, independently-tuned instance on the Filter bank's
    // peak branch (`FilterFxChain::peakLimiter`, `dsp/FilterFx.hpp`), which
    // needed the type reachable from a header below this one in the include
    // graph (this class includes `dsp/FilterFx.hpp`, never the reverse --
    // see `dsp/Limiter.hpp`'s own header comment for the full reasoning).
    // `outputLimiter_` below keeps its exact field name, its exact call
    // site (`outputLimiter_.Configure(sampleRate_)` in `PrepareToPlay()`,
    // unchanged), and its exact tuning (the single-argument `Configure()`
    // overload pins the master's original threshold/ceiling/attack/release,
    // `dsp/Limiter.hpp`'s own header comment) -- this move is behaviour-
    // neutral, not a retune.

    // Task 2.8 (revised 2026-07-28; SUPERSEDED 2026-07-29 by the
    // `OutputLimiter` above -- item 3, design.md A2/A2a/A4): this was the
    // final, unconditional hard clamp before a sample reached
    // `synth::AudioBlock::outputs`. In float audio, 1.0 IS full scale
    // (0 dBFS) -- the original 8.0f ceiling let this app hand the output
    // device a signal at +18 dBFS, which the device then hard-clipped into
    // a square wave (the operator-reported "blows the audio out"). Kept
    // here as the historical record of that fix; the clamp itself no longer
    // runs -- `SanitizeOutputSample()` below now hands off to
    // `outputLimiter_.Process()` instead of `std::clamp`.
    //
    // The clamp alone was NOT recovery: it silently capped whatever the
    // upstream chain produced, sample by sample, forever, while a poisoned
    // recursive state upstream (e.g. a biquad or comb whose state went
    // non-finite or diverged) kept producing bad values underneath it.
    // Genuine recovery -- detecting and resetting the poisoned unit itself
    // -- is `RecoverPoisonedUnitState()` below (tasks 2.2-2.5), which the
    // limiter cooperates with but does not replace: the Filter bank's Comb
    // can self-oscillate (its feedback curve is asymmetric +-0.95 as of
    // item 1, `dsp::Comb::GetFeedback` in FilterFx.hpp -- see that
    // function's own divergence-note comment) and the Reverb bank's
    // authored Hold control (task 3.8) pushes its internal feedback
    // coefficient toward, but strictly below, 1.0, so both are ordinarily
    // bounded by construction -- the limiter is the safety net for when
    // "ordinarily" stops holding, not the primary defense (items 1/2 are).
    float SanitizeOutputSample(float x) {
        if (!std::isfinite(x)) {
            return 0.0f;
        }
        if (x != 0.0f && std::fabs(x) < std::numeric_limits<float>::min()) {
            return 0.0f;  // flush subnormal/denormal to zero.
        }
        // Limiter, then a hard bound (design.md A2/A2a/A4; spec
        // "Output is limited, then bounded"). `outputLimiter_` is a
        // feed-forward one-pole gain rider with no lookahead: its gain
        // reduction is computed from the signal it has already passed, so
        // it necessarily lags a fast-changing input and cannot itself
        // guarantee a ceiling -- measured, not assumed: a steady 1.5x
        // 200 Hz tone settled at a stable periodic peak of 1.027426 with
        // the gain envelope already converged at 0.685245, and the
        // self-oscillating-comb + near-unity-reverb-hold patch produced a
        // raw 1.560059 with the envelope at 0.62-0.64. Lookahead would fix
        // that lag but was rejected -- it adds latency to a live
        // instrument. So gain reduction does the work here, and the
        // std::clamp below only catches the residual overshoot the
        // limiter's lag lets through -- typically a few percent, which is
        // inaudible. This is NOT a reinstatement of the defect the old
        // unconditional 8x-full-scale clamp above caused (that produced a
        // guaranteed square wave); clamping a residual after gain
        // reduction has already brought the signal near 1.0 is a
        // different, harmless thing.
        return std::clamp(outputLimiter_.Process(x), -1.0f, 1.0f);
    }

    // Tasks 2.4/2.5 (Tier 2, magnitude recovery) -- the ceiling is DERIVED,
    // not measured, and re-derived here rather than re-tuned by feel.
    // Re-derived 2026-07-29 after items 1/2 tightened both inputs below
    // (design.md A2); the ceiling constant itself is UNCHANGED (100.0 was
    // already comfortably above the tighter figures too, so there was
    // nothing to retune):
    //   - The filter chain's input is bounded to +-1.0 by
    //     `PadeSaturator::Saturate` (FilterFx.hpp:94-99, `std::max(-1.0f,
    //     std::min(1.0f, output))`) before it ever reaches a recursive
    //     stage this recovery watches.
    //   - `ResonantBump`'s peak gain is `A^2 == height`, `height ==
    //     dsp::ExpMapCompute(1.0f, 4.0f, knob)` (FroggersAppCore.hpp's own
    //     RouteAudioSample, Filter bank wiring, item 2) -- at most 4x for
    //     any reachable knob value (was 10x).
    //   - `scoopNotch`'s height is a DIP, not a gain: `max(0.05, 1 - 0.95 *
    //     scoop)` in [0.05, 1] -- adds no gain at all, unaffected by item 2.
    //   - So the largest legitimate magnitude this chain can produce is
    //     roughly 4 (ResonantBump's peak gain), maybe ~8-10 with ringing
    //     (Comb's now sub-unity +-0.95 feedback, item 1 -- a decaying loop,
    //     not a compounding one, but still capable of several round trips'
    //     worth of buildup before it settles). 100.0 remains a full 10x-25x
    //     above ANY of that -- comfortably above legitimate ringing,
    //     comfortably below float overflow (3.4e38, so 100.0 is ~3.4e36x
    //     below it) -- and because divergence under recursive feedback is
    //     exponential, a REAL fault crosses from "normal" to "past 100" in
    //     milliseconds, not minutes, so this ceiling is never mistaken for
    //     a slow legitimate swell.
    // DO NOT retune this constant without re-deriving it from the above.
    static constexpr float kMaxUnitStateMagnitude = 100.0f;

    // "Sustained" (task 2.5's own explicit ask): a unit's state magnitude
    // must stay above kMaxUnitStateMagnitude for at least this much REAL
    // TIME, not merely "the last block-end snapshot", before it is treated
    // as a genuine divergence rather than a transient. Tracked in seconds
    // (not a block count) so the definition does not silently change shape
    // with block size -- app/Makefile's own tests span both blockSize==1
    // (FroggersCrunchyBlowupRepro.cpp) and blockSize==256/512
    // (everything else). 10ms is deliberately short: the ceiling's own
    // derivation above establishes that a real fault's exponential
    // divergence crosses from normal into "past 100" in milliseconds, so it
    // will still read as over-ceiling several block-boundaries later almost
    // certainly (its magnitude keeps growing, it does not hover exactly at
    // the crossing point) -- while a single loud transient that peaks once
    // and decays within a block will already be back under the ceiling by
    // the very next block-end snapshot, resetting this unit's counter to
    // zero before 10ms of continuous over-ceiling readings can ever
    // accumulate. 10ms is also short enough that, on the rare occasion a
    // real fault DOES take this path, the operator hears at most a brief
    // click before recovery, not an audible dropout.
    static constexpr float kSustainedOverCeilingSeconds = 0.01f;

    // Tasks 2.2-2.5: one unit's worth of Tier 1 (finiteness) + Tier 2
    // (sustained magnitude) recovery, called once per block per unit from
    // RecoverPoisonedUnitState() below. `Unit` is any of the dsp:: structs
    // Item 2 gave `StateFinite()`/`StateMagnitude()`/`Reset()` to (dsp::Vco,
    // dsp::ResonantBump, dsp::Comb, dsp::Oversampler2x,
    // dsp::SampleRateReducer, dsp::DriveBlendPhase) -- templated rather than
    // duplicated 10 times over, since the recovery POLICY (finiteness first,
    // then a sustained-magnitude watch) is identical for every one of them
    // and only the concrete unit type differs.
    //
    // Tier 1 fires immediately and unconditionally resets the sustained-
    // magnitude counter too (a unit that just went non-finite has no
    // meaningful prior magnitude history worth preserving). Tier 2 only
    // evaluates once Tier 1 has confirmed the state IS finite -- a
    // non-finite StateMagnitude() read (e.g. NaN) would never compare
    // `>` the ceiling, silently defeating Tier 2, so Tier 1 must run first.
    template <typename Unit>
    void RecoverUnitIfNeeded(Unit& unit, float& overCeilingSeconds, std::size_t blockFrames) {
        if (!unit.StateFinite()) {
            unit.Reset();
            overCeilingSeconds = 0.0f;
            return;
        }
        if (unit.StateMagnitude() > kMaxUnitStateMagnitude) {
            overCeilingSeconds += static_cast<float>(blockFrames) / sampleRate_;
            if (overCeilingSeconds >= kSustainedOverCeilingSeconds) {
                unit.Reset();
                overCeilingSeconds = 0.0f;
            }
        } else {
            overCeilingSeconds = 0.0f;
        }
    }

    // Task 2.3 (Tier 1 ONLY -- no magnitude/Tier 2 counter). Used for
    // `delay_`/`reverb_`: both are exposed to the exact same "a poisoned
    // sample from an upstream unit permanently latches this unit's own
    // state non-finite" failure Tier 1 exists to fix (see
    // dsp::Reverb::StateFinite()'s and dsp::StereoDelay::StateFinite()'s
    // own comments) -- BUT they deliberately do NOT get Tier 2's
    // sustained-magnitude watch, because kMaxUnitStateMagnitude's derivation
    // above is specific to the Filter-chain-bounded units below (input
    // bounded to ~10-30 by construction); `delay_`'s feedback (up to 0.98)
    // and `reverb_`'s authored Hold (up to 0.999) are BIBO-stable feedback
    // loops that can LEGITIMATELY settle to a much larger-but-finite steady
    // state under sustained loud input (roughly input/(1-feedback), which
    // at reverb_'s extreme Hold alone is already >> 100) -- applying the
    // same ceiling there would misfire on a genuinely loud, stable, musical
    // tail, not just a fault.
    template <typename Unit>
    void RecoverIfNonFinite(Unit& unit) {
        if (!unit.StateFinite()) {
            unit.Reset();
        }
    }

    // Tasks 2.2-2.5 (design: per-unit, never global -- see this class's
    // header comment on why a global reset is wrong: it would cut the
    // reverb tail and delay repeats every time one unrelated filter
    // misbehaved). Called once per block, after ProcessBlock()'s per-sample
    // loop (RecoverUnitIfNeeded() above needs `block.numFrames` for Tier 2's
    // time accounting, hence taking it as a parameter rather than being
    // folded into the per-sample loop itself).
    //
    // Tier 1 + Tier 2 (RecoverUnitIfNeeded): exactly the unit set Item 2
    // gave a Reset() to -- the three audio-path VCOs, the authored
    // DriveBlendPhase allpass, the Drive bank's Oversampler2x and both
    // SampleRateReducers, and the Filter bank's two ResonantBumps (peak,
    // scoopNotch) and Comb.
    //
    // Tier 1 ONLY (RecoverIfNonFinite): `delay_`/`reverb_`, reusing their
    // EXISTING Reset()/ClearBuffers() (StereoDelay::Reset() is a thin alias
    // for ClearBuffers(), added so the same generic call works uniformly --
    // see that method's own comment) rather than adding duplicates, per
    // task 2.2's explicit instruction. This is IN ADDITION to their
    // existing, separate, transport-edge-gated reset
    // (`wasTransportRunning_`/`delayReverbClearPending_` above, which exists
    // for a different reason -- silencing self-sustaining feedback on Stop,
    // not fault recovery). The two do not conflict: Reset()/ClearBuffers()
    // is idempotent, and in NORMAL operation `delay_`/`reverb_` never
    // actually go non-finite on their own (both are BIBO-stable by
    // construction, per the comment above) -- Tier 1 only ever fires here
    // when a genuine upstream fault cascaded a non-finite sample into them,
    // exactly the case with no other recovery path.
    void RecoverPoisonedUnitState(std::size_t blockFrames) {
        RecoverUnitIfNeeded(audioVco1_, vco1OverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(audioVco2_, vco2OverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(audioVco3_, vco3OverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(driveBlendPhase_, driveBlendPhaseOverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(drive_.oversampler, driveOversamplerOverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(drive_.sampleRateReducer1, sampleRateReducer1OverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(drive_.sampleRateReducer2, sampleRateReducer2OverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(filterChain_.peak, filterPeakOverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(filterChain_.scoopNotch, filterScoopNotchOverCeilingSeconds_, blockFrames);
        RecoverUnitIfNeeded(filterChain_.comb, filterCombOverCeilingSeconds_, blockFrames);
        RecoverIfNonFinite(delay_);
        RecoverIfNonFinite(reverb_);
        // Item 3: the limiter's envelope is per-sample state too, so it
        // participates in the same per-unit recovery path as every other
        // stage -- Tier 1 only (like delay_/reverb_ above), since a finite
        // input always keeps `envelope` in (0, 1] by construction (this
        // struct's own comment) and there is no analogous "legitimately
        // very large but finite" case Tier 2 would need to tolerate.
        RecoverIfNonFinite(outputLimiter_);
        // B5: the peak branch's own limiter carries the identical per-
        // sample `envelope` state as the master above -- same Tier-1-only
        // treatment, same reasoning, independent instance.
        RecoverIfNonFinite(filterChain_.peakLimiter);
    }

    FroggersParameterModel parameters_;
    FroggersModulationSlate modulation_;

    // Task 6a: the real audio path's own DSP state (sample-rate-dependent
    // members are (re)initialized in PrepareToPlay()). Deliberately separate
    // instances from modulation_'s private VCOs -- see this class's
    // PrepareToPlay() comment.
    float sampleRate_ = 48000.0f;
    dsp::Vco audioVco1_;
    dsp::Vco audioVco2_;
    dsp::Vco audioVco3_;
    dsp::VcoAdsrState audioAdsr_;
    dsp::FrogBlock drive_;
    dsp::DriveBlendPhase driveBlendPhase_;
    dsp::FilterFxChain filterChain_;
    dsp::StereoDelay delay_;
    dsp::Reverb reverb_;
    // Item 3 (design.md A2/A2a): the output-stage limiter's own per-sample
    // gain-envelope state -- see the comment above `SanitizeOutputSample()`
    // for the full design rationale, and `dsp::OutputLimiter`'s own
    // definition (`dsp/Limiter.hpp`, moved out by B5) for the struct itself.
    dsp::OutputLimiter outputLimiter_;

    // Tasks 2.4/2.5 (Tier 2 recovery): per-unit "how many consecutive
    // seconds of real audio has this unit's state stayed over
    // kMaxUnitStateMagnitude" counters, one per unit RecoverPoisonedUnitState()
    // watches -- see RecoverUnitIfNeeded()'s own comment for why this is
    // tracked in seconds (block-size-independent) rather than a block count.
    float vco1OverCeilingSeconds_ = 0.0f;
    float vco2OverCeilingSeconds_ = 0.0f;
    float vco3OverCeilingSeconds_ = 0.0f;
    float driveBlendPhaseOverCeilingSeconds_ = 0.0f;
    float driveOversamplerOverCeilingSeconds_ = 0.0f;
    float sampleRateReducer1OverCeilingSeconds_ = 0.0f;
    float sampleRateReducer2OverCeilingSeconds_ = 0.0f;
    float filterPeakOverCeilingSeconds_ = 0.0f;
    float filterScoopNotchOverCeilingSeconds_ = 0.0f;
    float filterCombOverCeilingSeconds_ = 0.0f;

    // Stop-transport reset (operator report "Stop doesn't work" -- the ASR
    // gate closes on Stop but delay_/reverb_ are feedback structures that
    // self-sustain: StereoDelay feedback runs up to 0.98, Reverb Hold pushes
    // its feedback arbitrarily close to 1.0). Remembers the PREVIOUS frame's
    // transport-running state so ProcessBlock can detect the exact
    // running->stopped edge and reset both units, once, on the audio thread
    // that owns them -- see ProcessBlock's own comment at the gate/edge
    // computation for why this is not done from the UI thread's kStop
    // handler (FroggersUiSurface.hpp's HandleAction) instead.
    bool wasTransportRunning_ = false;

    // ITEM 1 (revised): true while stopped and a clear is still owed --
    // armed either at the running->stopped edge (if a voice was still
    // releasing there) or left false (if the edge's own AllIdle() check
    // already fired the clear directly). Stays true, without re-clearing
    // every block, until either AllIdle() first turns true (fires the one
    // owed clear) or the transport resumes (cancels it, since a resumed
    // delay/reverb is legitimately playing and must not be wiped) -- see
    // ProcessBlock's per-sample loop, where the running->stopped edge is
    // detected, for the full logic.
    bool delayReverbClearPending_ = false;

    // Task 7.2 (design D7): one ScopeWriter, ReserveChans(1) per VCO
    // returning a ScopeWriterHolder -- declared before the holders below
    // since the constructor's init list calls vcoScopeWriter_.ReserveChans()
    // to construct them (member init order follows declaration order, not
    // ctor init-list order).
    synth::ScopeWriter vcoScopeWriter_;
    synth::ScopeWriterHolder vco1ScopeHolder_;
    synth::ScopeWriterHolder vco2ScopeHolder_;
    synth::ScopeWriterHolder vco3ScopeHolder_;
    // Task 7.1: populated once per block (ProcessBlock(), after the
    // per-sample loop) via dsp::Vco::PopulateUIState().
    dsp::Vco::UIState vco1ScopeUiState_;
    dsp::Vco::UIState vco2ScopeUiState_;
    dsp::Vco::UIState vco3ScopeUiState_;
    // Task 7.3: the ScopeVisualizer<UIState> instance itself -- this is the
    // "no app-level drawing code" half of D7 (VcoScopeVisualizer() above
    // exposes it for packet 10 to place).
    synth::ui::ScopeVisualizer<dsp::Vco::UIState> vcoScopeVisualizer_;

    // Task 9.1 (design D10): populated once per block from filterChain_'s
    // live peak/comb, same convention as the VCO UIStates above.
    dsp::ResonantBump::UIState peakUiState_;
    dsp::Comb::UIState combUiState_;
    // Task 9.2/9.3: one TransferFunctionVisualizer per filter, attached as
    // the Filter bank's Peak/Comb parameter underlays via
    // parameters_.Init()'s peakVisualizer_/combVisualizer_ arguments (task
    // 9.3) -- rendered automatically as grid-cell underlays (D9b), not
    // placed into the surface's own tree directly.
    TransferFunctionVisualizer peakVisualizer_;
    TransferFunctionVisualizer combVisualizer_;

    // Packet 10 (design D11/D14): the UI-thread -> audio-thread request
    // bridge (see this file's header comment) plus the audio-thread-only
    // active-bank/drill-in bookkeeping it drives.
    std::atomic<int> pendingBankSelect_{-1};
    std::atomic<int> pendingEncoderPress_{-1};
    std::atomic<bool> pendingRandomizeAll_{false};
    std::atomic<bool> pendingRandomizePage_{false};
    std::atomic<double> pendingTempoBpmRequest_{-1.0};

    std::atomic<double> tempoDisplayBpm_{synth::MasterClock::kDefaultTempoBpm};
    std::atomic<bool> tempoExternallyClocked_{false};
    // Task 3.6 (design E3e): published once per block, same contract as the
    // two atomics above -- see TransportRunning()'s own comment.
    std::atomic<bool> transportRunningDisplay_{false};
    // A4: published once per Randomize All/Page press from ProcessFrame() --
    // see LastRandomizePartial()'s own comment.
    std::atomic<bool> lastRandomizePartial_{false};

    // D17 robustness fix (see PrepareToPlay()'s own comment): the surface's
    // last explicit Play(true)/Stop(false) request, independent of
    // `MasterClock::TransportState()` -- defaults false so a fresh app (or a
    // headless rig that never presses Play) stays silent exactly as before.
    std::atomic<bool> desiredTransportRunning_{false};

    std::size_t activeBankIx_ = 0;
    std::optional<FroggersModulationDrillIn> drillIn_;
};

}  // namespace synth_froggers
