#pragma once

// frogg3rs_vst::FroggersPluginProcessor -- Group 5 (VST/AU plugin skeleton)
// of the frogg3rs-browser-and-vst-hosts change. This is the ONLY JUCE-facing
// wrapper over the JUCE-free app core (synth_froggers::FroggersApp,
// app/Froggers.hpp) -- every file under app/vst/ may see JUCE; the core
// under app/ and its check_no_juce gate (app/Makefile) stay untouched.
//
// Init/drive sequence traced from the two existing JUCE-facing callers of
// synth::Engine<App>, both read start-to-finish before writing this file:
//   - External/Sheaf/projects/synth/runtime/Runtime.hpp (the real JUCE host
//     shell FroggersMain.cpp's launcher session uses under the hood, via
//     synth_runtime::RuntimeShellSession -- Runtime.hpp:100-106 constructs
//     synth::Engine<App> with a NowMicros() timestamp provider; Start()
//     (:178-230) calls engine_.SetRuntimeDataPaths(...) THEN
//     engine_.Initialize() exactly once; audioDeviceAboutToStart (:594-599)
//     calls engine_.Prepare(sampleRate, blockSize) on every device
//     negotiation (not just once); audioDeviceIOCallbackWithContext (:591)
//     calls engine_.ProcessBlock(block, NowMicros()) every callback.
//   - External/Sheaf/projects/synth/tests/support/SynthRig.hpp, the
//     headless JUCE-free harness FroggersHeadlessTests.cpp/
//     FroggersRandomizeAllRepro.cpp already drive FroggersApp through
//     (SynthRig.hpp:60,93-94 constructor: Initialize() then Prepare() in
//     that order; RunOneBlockAt :513-539 builds a synth::AudioBlock with
//     inputs=nullptr/numInputChannels=0 -- FroggersAppCore::Config()
//     requests zero audio inputs -- and calls engine_.ProcessBlock(block,
//     timestamp); StartAt/StopAt :174-184 push synth::MessageIn::Start/Stop
//     on engine_.UiBus(), the exact message FroggersUiSurface::HandleAction's
//     Play/Stop branches push (app/FroggersUiSurface.hpp:1838,1874), paired
//     with FroggersApp::SetDesiredTransportRunning(true/false)
//     (FroggersUiSurface.hpp:1845,1887-1888) -- the UI-thread seam
//     PrepareToPlay() re-asserts across a MasterClock::Prepare() reset,
//     see FroggersAppCore.hpp's own PrepareToPlay() comment).
//
// This class mirrors Runtime.hpp's call order exactly (constructor:
// SetRuntimeDataPaths + Initialize(), once; prepareToPlay(): Prepare(),
// every call; processBlock(): ProcessBlock(), every callback) without any
// of Runtime.hpp's device-manager/MIDI-connection/window machinery -- none
// of that is needed to drive the core, so none of it is duplicated (the
// group 5 brief's BLOCKED condition, "core cannot be driven headlessly
// without launcher-session machinery," does not apply: synth::Engine<App>
// is the seam, and it is already JUCE-free and driven exactly this way by
// SynthRig.hpp's own JUCE-free tests).
//
// Data path: reuses the SAME "frogg3rs" stable app id and shared
// ~/Library/Sheaf data root FroggersMain.cpp's direct-launch app uses
// (app/FroggersMain.cpp:19,48,53 cites this reasoning: "so existing saved
// patches ... are not orphaned"), via the same tiny data-path helper
// (synth_runtime::SheafUserApplicationDataRoot(), HostDataPaths.cpp) rather
// than duplicating its logic -- that helper depends only on juce_core (a
// plugin dependency anyway) and owns no window/device/thread state, so
// reusing it is not "launcher-code" in the sense the brief's BLOCKED
// condition means.
//
// Transport: FroggersAppCore's ASR gate stays closed (silence) until the
// transport is started via synth::MessageIn::Start + SetDesiredTransportRunning
// (FroggersAppCore.hpp's ProcessBlock()/TransportQuarterNotesAt() gating
// comment). Group 5 wired no DAW-host transport source at all and added no
// note handling (group 5 brief: MIDI buffer accepted and ignored).
//
// Group 6: the DAW is now the transport AND tempo authority. Both producers
// below are driven from `getPlayHead()`, and both obey the SAME
// audio-thread-safety trace (see the header comment on
// pendingTransportEdge_ below for the full citation chain):
// `juce::AudioPlayHead::getPosition()` may ONLY be called from
// processBlock() (juce_AudioPlayHead.h's own doc comment: "undefined
// behaviour ... multithreading issues if it's not called on the audio
// thread"), while `engine_.UiBus()` (the same bus
// FroggersUiSurface::PushMessage/HandleAction write from the UI/message
// thread, FroggersUiSurface.hpp:2061-2065) is an SPSC ring buffer
// (MessageInBus::Push, ParameterModulation.cpp:3995-4005: an unsynchronized
// read-modify-write of `tail_`, safe for exactly one producer) already
// claimed by that same thread's Push calls in every other host of this
// surface -- so processBlock() must never call Push() itself, on pain of a
// second concurrent producer racing the first. The split this group uses
// throughout: processBlock() (audio thread) ONLY reads the playhead and
// republishes what it saw into lock-free atomics; timerCallback() (message
// thread, wired per carry-forward 2 below) is the ONLY thing that ever
// calls engine_.UiBus().Push() or engine_.RequestSyncConfiguration() from
// this class, reading those atomics to decide what to push.
//
// TestStartTransport()/TestStopTransport() below still exist (retained, not
// retired -- carry-forward 3's first option) purely for
// FroggersVstSmokeTest.cpp's existing headless "no host at all" smoke
// coverage; the REAL producer (host playhead edge-trigger + host-tempo
// clock-tick synthesis) is exercised by app/vst/FroggersVstHostTests.cpp
// (task 6.4) via a fake AudioPlayHead and PumpMessageThreadForTest(), the
// deterministic stand-in for a real juce::Timer firing (a headless CTest
// binary runs no message loop, so juce::Timer callbacks never fire on
// their own -- see PumpMessageThreadForTest()'s own comment).
//
// Group 7 (frogg3rs-vst-host spec, "Parameters are external via a stable
// automation surface"): every user parameter of the six-bank model
// (FroggersParameterModel, app/FroggersParameters.hpp) is exposed as a
// juce::AudioProcessorParameter with a flat stable ID, bridged
// BIDIRECTIONALLY to FroggersParameterModel -- the app's single parameter
// authority -- plus Freeze (already wired via DispatchAction in group 6).
// SAME audio-thread/message-thread split as 6.1/6.3 above, extended, not
// replaced: processBlock() (audio thread) additionally publishes each
// parameter's current value into a per-parameter atomic UIState snapshot
// (Parameter::PopulateUIState(), the exact call synth::Engine's own
// ProcessBlock already makes for its throttled UI publish -- see
// processBlock()'s own comment for why this class makes its OWN,
// unthrottled, bank-selection-independent calls instead of reusing that
// one); timerCallback() (message thread, via PumpHostParameterBridge()) is
// the ONLY thing that reads those snapshots to notify the host, and the
// ONLY thing that pushes a host-driven write into the core (via the SAME
// engine_.UiBus().Push()/DispatchAction() seams 6.1/6.3/Freeze already use)
// -- processBlock() still never touches UiBus or DispatchAction itself. No
// plugin-side MIDI mapping or MIDI learn exists anywhere in this class (per
// the governing spec): DAW-side MIDI mapping reaches this instrument
// entirely through the ordinary host-parameter-automation surface below,
// the same way it would for any other automatable plugin parameter.

#include "Froggers.hpp"
#include "synth/AppRegistry.hpp"
#include "synth/Engine.hpp"
#include "synth/MasterClock.hpp"
#include "synth/ParameterModulation.hpp"

#include "HostDataPaths.hpp"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace frogg3rs_vst {

class FroggersPluginProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
    // Production entry point (also what JUCE's generated createPluginFilter()
    // constructs, see FroggersPluginProcessor.cpp): resolves the shared
    // "frogg3rs" data root (see this file's own header comment).
    FroggersPluginProcessor();

    // Test-only entry point (5.2): lets the smoke test point the engine at a
    // scratch data root instead of the shared production one, the same
    // reason FroggersHeadlessTests.cpp's UseScratchRuntimeDataPaths() exists
    // -- a headless test must never read or write the operator's real
    // ~/Library/Sheaf state.
    explicit FroggersPluginProcessor(synth::RuntimeDataPaths dataPathsForTest);

    // Not defaulted: must stopTimer() before the rest of this object (in
    // particular engine_) tears down -- a juce::Timer's callback can fire on
    // the message thread right up until stopTimer() returns, and
    // timerCallback() below touches engine_.
    ~FroggersPluginProcessor() override;

    FroggersPluginProcessor(const FroggersPluginProcessor&) = delete;
    FroggersPluginProcessor& operator=(const FroggersPluginProcessor&) = delete;

    // --- juce::AudioProcessor ------------------------------------------
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Group 8: the real portable-surface editor (FroggersPluginEditor.hpp)
    // -- defined out-of-line in the .cpp so this header does not need to
    // include the JUCE-GUI-heavy PortableJuceBackend.hpp chain, and so
    // FroggersPluginEditor.hpp (which itself needs this class's full
    // definition, for EditorSurface()/SetEditorRepaintHook() below) does not
    // have to be included before this class is complete. hasEditor() is
    // true now that createEditor() returns a real editor -- every host
    // still automates/reads back every parameter below via its own generic
    // UI too, this just adds the portable-surface UI on top for hosts that
    // show it.
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Frogg3rs"; }
    // NEEDS_MIDI_INPUT TRUE (CMakeLists.txt) declares MIDI input accepted;
    // acceptsMidi() must agree. The buffer is ignored every block (see
    // processBlock()) -- group 5 brief: "a synth that declares MIDI input
    // accepted is fine; no note wiring."
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // Group 7 exposes every user parameter as a host-automatable
    // juce::AudioProcessorParameter below (BuildHostParameterInventory()),
    // but DAW SESSION state persistence (serializing current parameter/
    // patch state into a host-saved MemoryBlock and restoring it) is a
    // separate concern the group 7 brief does not ask for -- left as
    // no-ops here, same as group 5/6. A host that automates/records this
    // plugin's parameters within a live session works fully; a host that
    // saves a project and reopens it will NOT recall this plugin's last
    // parameter values (out of this group's scope, flagged for a future
    // group rather than silently implied by the parameter surface below).
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // --- 5.2 test seam (retained, carry-forward 3) -----------------------
    // Dispatches the exact same kPlay/kStop actions
    // (FroggersUiSurface.hpp:1826-1876, via
    // engine_.Application().PortableSurface().DispatchAction()) the real
    // Play/Stop buttons dispatch -- see this file's header comment and
    // timerCallback()'s own comment for why this goes through
    // DispatchAction rather than hand-mirroring HandleAction's message
    // sequence. Not reachable from any host UI (no editor); exists solely
    // so FroggersVstSmokeTest.cpp can drive the core with no host/playhead
    // at all, the same way SynthRig::StartAt/StopAt do for the app core's
    // own test suite. Safe to call from any single thread that is not
    // concurrently calling processBlock() -- like every other UiBus
    // producer, it is not safe to call from two threads at once (see this
    // file's header comment on the SPSC contract); the smoke test drives it
    // single-threaded, so this holds.
    void TestStartTransport();
    void TestStopTransport();

    // --- 6.4 test seam ----------------------------------------------------
    // A headless CTest binary runs no JUCE message loop, so a real
    // juce::Timer started via startTimerHz() never fires on its own --
    // juce_events' dispatch loop is what calls timerCallback(), and nothing
    // in a plain `int main()` pumps it. Rather than stand up a
    // juce::MessageManager + a run-until-idle loop in the test binary (real,
    // but adds real flake risk: how long is "long enough" to wait for a
    // timer that fires every ~33ms is itself a race), this exposes
    // timerCallback() itself as the deterministic pump: production calls it
    // from the real Timer; app/vst/FroggersVstHostTests.cpp calls it
    // directly, synchronously, after driving processBlock() with a fake
    // playhead. Same function either way -- this is not a second, weaker
    // code path, just a second caller.
    void PumpMessageThreadForTest() { timerCallback(); }

    // Review fix, Important 2: public (not private, unlike the activity/
    // counter state it gates) specifically so
    // app/vst/FroggersVstHostTests.cpp's staleness-teardown test can drive
    // exactly this window rather than hardcoding a second copy of the
    // number that could silently drift from timerCallback()'s own.
    //
    // Carry-forward (group 6 re-review): this used to be a FIXED PUMP COUNT
    // (kStaleProcessBlockPumpThreshold == 3, ~100ms at this class's 30Hz
    // pump) -- flagged as an unvalidated heuristic: a scheduling-jitter
    // spike, or a host that legitimately pauses processBlock() delivery for
    // just over 100ms while still very much alive (briefly idling a track,
    // a slow buffer-size renegotiation, etc.), would trip the exact same
    // "host is gone" disengage as a real teardown -- producing a visible
    // tempo-display blip (disengage, then immediately re-engage on the next
    // usable block) for no real host departure. Replaced with a TIME-based
    // window, using the SAME NowMicros() clock this class already uses
    // everywhere else (startTime_-relative monotonic microseconds -- see
    // that member's own comment), widened to ~1 second: far outside
    // ordinary pump-scheduling jitter (a full second is ~30 missed 30Hz
    // pumps in a row, not a handful), while still disengaging within one
    // second of an actual teardown -- "prompt" is enough here because
    // nothing audio-critical depends on this deadline: this only gates the
    // host-tempo *display* slave (6.3), not audio output silencing, which
    // is FroggersAppCore's own transport-gated ASR, unaffected by this
    // window. The immediate releaseResourcesSeen_ path (below) is
    // UNCHANGED by this carry-forward: it remains the primary, fast
    // teardown trigger for the ordinary "host cleanly tore the track down"
    // case; this window only covers the case releaseResources() is never
    // called at all (see releaseResourcesSeen_'s own comment).
    static constexpr std::uint64_t kStaleActivityWindowMicros = 1'000'000;

    // Test-only accessors (6.4): the real consumer/producer state
    // app/vst/FroggersVstHostTests.cpp asserts against, rather than a
    // second, weaker copy of it. ApplicationForTest() is the same
    // FroggersApp& TestStartTransport()/timerCallback() already drive
    // (DisplayTempoBpm()/TempoExternallyClocked()/FreezeLatched()/
    // TransportRunning()/RequestTempoBpm() -- all real, existing
    // FroggersAppCore API, see that file's own comments). UiBusPendingCount
    // ForTest() reads engine_.UiBus().Size() (MessageInBus::Size(),
    // ParameterModulation.hpp:1022) -- the actual SPSC ring buffer 6.1/6.3
    // push onto, letting a test count messages AT THE BUS, per task 6.4's
    // own instruction, without draining them (draining only happens inside
    // engine_.ProcessBlock(), i.e. only when the test itself calls
    // processBlock() again).
    synth_froggers::FroggersApp& ApplicationForTest() { return engine_.Application(); }
    // Not const: engine_.UiBus() itself has no const overload (Engine.hpp).
    std::size_t UiBusPendingCountForTest() { return engine_.UiBus().Size(); }

    // Task 7.2 tests need no new test-only accessors beyond what's already
    // public: every exposed host parameter is a plain
    // juce::AudioProcessorParameter reachable via the standard
    // juce::AudioProcessor::getParameters() (public JUCE API, this class
    // adds nothing on top of it), and its ground truth on the core side is
    // ApplicationForTest().Parameters() (FroggersAppCore::Parameters(),
    // already public) -- see BuildHostParameterInventory()'s own comment
    // for the exact stable-ID scheme a test reconstructs to find a specific
    // parameter, and PumpHostParameterBridge()'s own comment for why a
    // juce::AudioProcessorParameter::Listener (standard JUCE, added by the
    // test itself) is the right tool to observe this bridge's
    // setValueNotifyingHost() traffic rather than a bespoke counter here.

    // -- Group 8: editor render-host seam -----------------------------------
    // The exact synth::ui::Surface& FroggersPluginEditor renders through
    // synth_juce::PortableComponent -- the SAME instance DispatchAction()/
    // TestStartTransport()/PumpHostParameterBridge() already drive via
    // Application().PortableSurface() (Froggers.hpp:52), so the editor
    // observes/drives the live production surface with no second copy and
    // no new plumbing. Named for its real caller (NOT "...ForTest()", unlike
    // ApplicationForTest() above) because this IS the production accessor;
    // FroggersVstHostTests.cpp's own tests use ApplicationForTest().
    // PortableSurface() instead, by the same naming logic in reverse.
    synth::ui::Surface& EditorSurface() { return engine_.Application().PortableSurface(); }

    // Mirrors synth_runtime::Runtime<App>::SetRepaintHook (Sheaf
    // runtime/Runtime.hpp:500) -- the SAME single-slot "the message-thread
    // timer that already drives per-tick work also drives a UI repaint"
    // idiom this class's own timerCallback() follows for
    // engine_.MessageThreadTick() (see this file's header comment, "Tick
    // order mirrors Sheaf Runtime.hpp's own timerCallback()"). Exactly one
    // editor can be open at a time (JUCE's own AudioProcessorEditor
    // contract: a host calls createEditor() again only after the previous
    // editor has been destroyed), so a single slot -- not a list -- is the
    // right shape, same as Runtime<App>'s own repaintHook_. The editor
    // registers this in its constructor and MUST clear it (pass {}) in its
    // destructor BEFORE any of its own members (in particular the
    // PortableComponent this hook calls into) finish tearing down -- same
    // ordering contract synth_runtime::RuntimeShellSession's destructor
    // documents for this exact hook shape (Shell.hpp:91-95). Plain
    // std::function, no atomic: both the write (the editor's ctor/dtor) and
    // the read (timerCallback(), below) happen only ever on the message
    // thread -- editors are host-constructed/destroyed on the message
    // thread by JUCE's own contract, and timerCallback() only ever runs
    // there too, so the two can never interleave, only be ordered (which
    // the editor's own destructor comment covers).
    void SetEditorRepaintHook(std::function<void()> hook) { editorRepaintHook_ = std::move(hook); }

private:
    // juce::Timer override (private per the private-inheritance idiom
    // Sheaf's own Runtime.hpp uses, `class Runtime : private
    // juce::AudioIODeviceCallback, private juce::Timer` -- Runtime.hpp:100).
    // Carry-forward 2 (group 5 review): pumps engine_.MessageThreadTick()
    // every tick, same call Runtime.hpp's own timerCallback() makes first
    // (Runtime.hpp:975) -- non-realtime-safe work (patch IO, serialization
    // arena growth, MIDI-out processor pumps) that must never run on the
    // audio thread. Also the ONLY place this class calls
    // engine_.UiBus().Push() or engine_.RequestSyncConfiguration() -- see
    // this file's header comment on why processBlock() itself must not.
    void timerCallback() override;

    // -- 6.1: host transport edge-trigger -----------------------------------
    // processBlock() (audio thread) is the only place allowed to call
    // getPlayHead()->getPosition() (juce_AudioPlayHead.h's own doc comment:
    // "You can ONLY call this from your processBlock() method!"), so it is
    // also the only place that can DETECT a host play-state transition. It
    // may not PUSH the resulting Start/Stop message itself (see this file's
    // header comment) -- so it records at most one pending edge here, a
    // single-slot atomic exactly like FroggersAppCore's own
    // pendingBankSelect_/pendingEncoderPress_ idiom
    // (FroggersAppCore.hpp:557-568's own comment: "a single-slot pending
    // request; a later write ... simply coalesces (acceptable: ...
    // control-rate, human-paced actions, never a data stream)" -- a host
    // transport toggle is exactly that kind of action). timerCallback()
    // claims it with exchange() and, if non-empty, pushes the mirrored
    // Play/Stop-button message sequence.
    enum class PendingTransportEdge : int { kNone = 0, kStart = 1, kStop = 2 };
    std::atomic<int> pendingTransportEdge_{static_cast<int>(PendingTransportEdge::kNone)};
    // Audio-thread-owned (processBlock only): the last host play-state this
    // object observed, and whether it has observed one yet at all. Not
    // published anywhere -- only processBlock() ever reads or writes these,
    // so no atomics are needed for them (contrast pendingTransportEdge_
    // above, which crosses to the message thread).
    bool haveHostPlayingBaseline_ = false;
    bool lastHostIsPlaying_ = false;

    // -- 6.3: host tempo via external-clock slaving -------------------------
    // Same audio-thread/message-thread split as 6.1 above: processBlock()
    // republishes what it read from the playhead into these atomics;
    // timerCallback() is the only reader, and the only thing that acts on
    // them (engine_.RequestSyncConfiguration()/engine_.UiBus().Push()).
    // hostTempoValid_ covers BOTH "no playhead at all" and "playhead present
    // but this host block reported no bpm" -- see processBlock()'s own
    // comment for why those collapse to the same "nothing usable this
    // block" signal down here.
    std::atomic<bool> hostTempoValid_{false};
    std::atomic<double> hostTempoBpm_{synth::MasterClock::kDefaultTempoBpm};
    // Message-thread-owned -- timerCallback() is the SOLE writer of
    // hostClockEngaged_/nextHostClockTickMicros_ (review fix, Important 1):
    // releaseResources() used to write hostClockEngaged_ directly, but JUCE
    // gives releaseResources() no thread guarantee at all
    // (juce_AudioProcessor.h's own declaration carries no thread
    // annotation, unlike processBlock()'s explicit "audio thread" contract)
    // while this class's 30Hz Timer fires independently on the message
    // thread -- two plain-bool writers on two different, unsynchronized
    // threads is a real data race (not merely a logical one), and could
    // leave the clock re-engaged immediately after releaseResources()
    // thought it had disengaged it. Fixed by routing BOTH triggers (the
    // Timer's own staleness detection AND releaseResources()) through one
    // atomic REQUEST (releaseResourcesSeen_ below) that only ever gates
    // what timerCallback() itself decides -- the alternative (making
    // hostClockEngaged_ itself atomic) would still leave two independent
    // threads racing to decide WHETHER to engage/disengage even if each
    // individual flag flip were data-race-free; funneling through a single
    // decision-maker removes the race at the decision level, not just at
    // the storage level.
    bool hostClockEngaged_ = false;
    std::uint64_t nextHostClockTickMicros_ = 0;

    // Review fix, Important 1: releaseResources() (any/unspecified thread,
    // see hostClockEngaged_'s own comment) sets ONLY this atomic; it never
    // touches hostClockEngaged_/nextHostClockTickMicros_/hostTempoValid_
    // itself. timerCallback() (message thread, sole owner of the fields
    // above) claims it with exchange() and treats it as an immediate
    // "treat the host as stale right now" signal -- same effect as the
    // staleness counter below reaching its threshold, just without waiting
    // for it.
    std::atomic<bool> releaseResourcesSeen_{false};

    // Review fix, Important 2 (teardown gap): the "disengage once
    // hostTempoUsable goes false" logic silently assumed processBlock()
    // keeps being called at all -- if a host suspends/disables this track
    // WITHOUT calling releaseResources(), processBlock() simply stops, so
    // hostTempoValid_/hostTempoBpm_ are never refreshed and stay stuck at
    // their last (possibly "usable") values forever, so hostClockEngaged_
    // never disengages either. processBlock() increments
    // processBlockCounter_ unconditionally, every call, as a pure liveness
    // heartbeat (it synchronizes no other data, so relaxed ordering is
    // enough on both ends); timerCallback() compares it against the value
    // it last observed to detect "processBlock has not run since the
    // previous pump" and, once kStaleActivityWindowMicros of REAL elapsed
    // time (not pump count) has passed with the counter unmoved, treats the
    // host as gone -- see kStaleActivityWindowMicros's own comment and
    // timerCallback()'s own comment for the full justification.
    std::atomic<std::uint64_t> processBlockCounter_{0};
    // Message-thread-owned (timerCallback only). lastObservedProcessBlockCounter_
    // is the last processBlockCounter_ value this pump observed CHANGE;
    // lastActivityMicros_ is the NowMicros() timestamp of the pump that last
    // observed that change -- i.e. "how long ago did processBlock() last
    // actually run," the carry-forward's time-based replacement for the old
    // fixed pump-count streak (staleProcessBlockPumpStreak_, removed).
    // Zero-initialized: at construction time NowMicros() is itself ~0 (it is
    // relative to startTime_, set in the same constructor), so the very
    // first pump's elapsed-time computation starts from "just built," not
    // "already stale" -- same grace period the old streak-based version got
    // for free from starting its counter at 0.
    std::uint64_t lastObservedProcessBlockCounter_ = 0;
    std::uint64_t lastActivityMicros_ = 0;

    // -- 7.1: stable-ID host parameter surface -------------------------------
    // One entry per exposed juce::AudioProcessorParameter: the 6*14 page
    // parameters + 6 per-bank Crispy + 1 shared Crunchy the model's own
    // enumeration produces (FroggersParameterModel -- see
    // BuildHostParameterInventory()'s own comment for how this is computed,
    // never hardcoded), plus one more for Freeze (not a ParameterManager
    // parameter at all -- see the Freeze branch of that method and of
    // PumpHostParameterBridge()).
    struct HostParamEntry {
        enum class Kind { kPageParam, kCrispy, kCrunchy, kFreeze };

        Kind kind = Kind::kPageParam;
        // Non-owning: juce::AudioProcessor::addParameter() (called once, in
        // BuildHostParameterInventory()) takes ownership and destroys this
        // along with every other parameter when the processor itself is
        // destroyed -- same lifetime contract as every other
        // juce::AudioProcessorParameter this codebase adds.
        juce::RangedAudioParameter* juceParam = nullptr;
        // Null only for kind == kFreeze -- Freeze is not a ParameterManager
        // parameter (see PumpHostParameterBridge()'s own comment on why its
        // bridge goes through DispatchAction(kFreeze)/FreezeLatched()
        // instead of this pointer).
        synth::Parameter* coreParam = nullptr;

        // Addressing for the host -> core write direction (see
        // PumpHostParameterBridge()'s own comment for the full trace of why
        // ParamSetAbsolute needs both of these, and why Crunchy/Freeze need
        // neither): bankIx/position identify WHICH Parameter this entry
        // targets on synth::ParameterManager's single physical BankSlot
        // (slotIx is always 0 -- FroggersParameterModel::Init() creates
        // exactly one). needsBankSelect is false only for kCrunchy (the
        // SAME Parameter object is registered at position 15 in every bank,
        // so any currently-selected bank resolves it correctly) and
        // kFreeze (bypasses the BankSlot/ParameterManager surface
        // entirely).
        std::size_t bankIx = 0;
        std::size_t position = 0;
        bool needsBankSelect = false;

        // Audio-thread-published (processBlock(), every block, via
        // Parameter::PopulateUIState() -- see processBlock()'s own comment)
        // / message-thread-read (PumpHostParameterBridge()) snapshot of
        // this parameter's current, post-fuego, post-modulation display
        // value -- Parameter::UIState::values[] is an atomic array
        // (ParameterModulation.hpp), safe for exactly this cross-thread
        // read/publish split. Null only for kind == kFreeze (FreezeLatched()
        // is already its own atomic -- no UIState needed).
        std::unique_ptr<synth::Parameter::UIState> uiState;

        // Message-thread-owned: the value BOTH sides last agreed on (either
        // "the host wrote this and we relayed it" or "the core settled to
        // this and we notified the host of it"). This is the single piece
        // of state PumpHostParameterBridge()'s feedback guard is built
        // around -- see that method's own comment for the full two-phase
        // per-parameter-per-pump algorithm.
        float shadowNormalized = 0.0f;
    };

    // Built once, in the constructor, right after engine_.Initialize() (the
    // point FroggersParameterModel's 91 Parameters and FroggersApp's
    // production surface first exist) -- see that method's own comment for
    // the full derivation.
    void BuildHostParameterInventory();
    // Message-thread-only (called from timerCallback(), see this file's
    // header comment on the audio-thread/message-thread split this class
    // has maintained since group 6): both directions of the bridge, one
    // pass over hostParams_ per pump. See this method's own comment (in the
    // .cpp) for the full bidirectional trace and feedback-guard design.
    void PumpHostParameterBridge();

    std::vector<HostParamEntry> hostParams_;
    // Message-thread-owned shadow of which bank this class itself last
    // selected on the shared BankSlot via a host-driven write (see
    // PumpHostParameterBridge()'s own comment) -- lets repeated writes to
    // the SAME bank skip a redundant MessageIn::SelectParamBank push.
    // FroggersParameterModel::Init() selects bank 0 by default
    // (`slot_->SelectBank(banks_[0])`), so this starts in agreement with
    // that real initial selection rather than forcing an unnecessary first
    // select.
    std::size_t lastSelectedBankIxForHostWrites_ = 0;

    // Group 8: see SetEditorRepaintHook()'s own comment above for the full
    // precedent trace. Empty (default-constructed, falsy) whenever no
    // editor is open -- the `if (editorRepaintHook_)` check in
    // timerCallback() then costs one branch and no repaint work.
    std::function<void()> editorRepaintHook_;

    std::uint64_t NowMicros() const;

    std::chrono::steady_clock::time_point startTime_;
    synth::Engine<synth_froggers::FroggersApp> engine_;
};

}  // namespace frogg3rs_vst
