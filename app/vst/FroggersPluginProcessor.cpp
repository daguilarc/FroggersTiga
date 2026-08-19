#include "FroggersPluginProcessor.hpp"

// Group 8: the real editor createEditor() below constructs, and the
// FroggersUiSurface downcast the constructor uses to call
// SetPluginHostMode() (see that call site's own comment).
#include "FroggersPluginEditor.hpp"
#include "FroggersUiSurface.hpp"
// Only FroggersManifest() (the app's own single-sourced identity, appId
// "frogg3rs") -- see ProductionDataPaths() below for why. JUCE-free itself
// (that file's own header comment), so this adds no new dependency weight.
#include "FroggersRegistration.hpp"

#include <array>
#include <cmath>
#include <utility>

namespace frogg3rs_vst {

namespace {

// Mirrors app/FroggersMain.cpp:48,53's dataRoot_/SheafPatchDataPathsForApp
// pair (same stable app id -- see that file's own header comment on why:
// "so existing saved patches ... are not orphaned"), so a patch saved from
// the standalone Frogg3rs app and one saved from this plugin land in, and
// load from, the same ~/Library/Sheaf/synth/sheaf-patch/patches/frogg3rs/
// directory. Reads the id from FroggersRegistration.hpp's own
// FroggersManifest().appId -- the app's existing single-sourced identity,
// already how FroggersApp registers with the launcher -- rather than
// restating "frogg3rs" as a second, independent literal here. app/
// FroggersMain.cpp's own pairing still carries its own separate literal
// (pre-existing, out of scope for this app/vst file to touch).
synth::RuntimeDataPaths ProductionDataPaths() {
    const std::filesystem::path dataRoot = synth_runtime::SheafUserApplicationDataRoot();
    return synth::SheafPatchDataPathsForApp(dataRoot, synth_froggers::FroggersManifest().appId);
}

// -- 7.1: stable host-parameter IDs --------------------------------------
// Derived ONLY from structural facts of FroggersParameterModel that are
// fixed by design and documented as such in app/FroggersParameters.hpp,
// never from iteration order, display strings, or anything that shifts
// when the model grows:
//   - bankIx: FroggersBankId's own enum value (Audio=0 .. Reverb=5,
//     FroggersParameters.hpp) -- a named, documented identity per bank, not
//     an incidental loop index (it IS a loop index here too, but one that
//     is guaranteed to match FroggersBankId's own fixed assignment, since
//     FroggersParameterModel::Init() creates banks in exactly that order
//     and nothing else creates banks on this manager).
//   - position: the parameter's fixed 0-13 slot within its bank's
//     14-parameter row (kFroggersParamsPerBank -- a structural layout fact,
//     independent of that slot's current display name; the file's own
//     header comment cites a real precedent for this exact stability
//     requirement -- "Ph.mod 1" was renamed from "Phase mod N" with its
//     slot/default left unchanged), or the named kFroggersCrispySlot (14) /
//     kFroggersCrunchySlot (15) constants already used throughout that file
//     for exactly this purpose.
// A bank/slot rename, or a defaultValue/color tweak, changes none of these
// three inputs -- exactly the "stable across sessions and releases"
// requirement the governing spec states. Freeze is not part of the model at
// all (see HostParamEntry::Kind::kFreeze's own comment); "crunchy" needs no
// bank/slot qualifier since exactly one exists, globally, in the whole
// model (task 4.2/4.6, FroggersParameters.hpp).
juce::String HostParamStableId(std::size_t bankIx, std::size_t paramIx) {
    return "bank" + juce::String(static_cast<int>(bankIx)) + ".slot" + juce::String(static_cast<int>(paramIx));
}

juce::String HostParamStableIdCrispy(std::size_t bankIx) {
    return "bank" + juce::String(static_cast<int>(bankIx)) + ".crispy";
}

constexpr const char* kHostParamStableIdCrunchy = "crunchy";
constexpr const char* kHostParamStableIdFreeze = "freeze";

}  // namespace

FroggersPluginProcessor::FroggersPluginProcessor()
    : FroggersPluginProcessor(ProductionDataPaths()) {}

FroggersPluginProcessor::FroggersPluginProcessor(synth::RuntimeDataPaths dataPathsForTest)
    // No .withInput(...) call: bus posture is stereo OUTPUT ONLY, no audio
    // input bus (group 5 brief, binding) -- matches FroggersAppCore::Config()
    // requesting zero audio inputs (FroggersAppCore.hpp's Config() comment).
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , startTime_(std::chrono::steady_clock::now())
    , engine_([this] { return NowMicros(); }) {
    // Runtime.hpp Start() order (:223,230, this file's header comment):
    // SetRuntimeDataPaths() BEFORE Initialize() -- startup patch/config
    // discovery reads dataPaths_ during Initialize() (Engine.hpp:213-222's
    // own doc comment on step 8).
    engine_.SetRuntimeDataPaths(std::move(dataPathsForTest));
    // Called ONCE, in the constructor -- not in prepareToPlay(), which JUCE
    // may call repeatedly (sample-rate/block-size renegotiation). Mirrors
    // Runtime::Start() calling engine_.Initialize() once, before any audio
    // device exists, separately from the per-negotiation engine_.Prepare()
    // in audioDeviceAboutToStart (Runtime.hpp:230,594-599).
    engine_.Initialize();

    // Group 8 (task 8.1): this is SetPluginHostMode()'s first PRODUCTION
    // call site (app/FroggersUiSurface.hpp's own comment on that method
    // names this exact call: "The plugin host (group 8's editor) calls
    // SetPluginHostMode(true) before/at attach time" -- previously
    // test-only, FroggersVstHostTests.cpp's BuildFroggersTree()). Called
    // here, right after engine_.Initialize() (i.e. as early as the surface
    // object exists at all), on the SAME instance EditorSurface() exposes
    // to the editor and PortableSurface() exposes to DispatchAction()
    // elsewhere in this class -- so every rebuild (BuildTree() reruns every
    // frame, that method's own comment) renders plugin mode from the very
    // first frame, with no separate opt-in the editor itself has to
    // remember to perform.
    //
    // SetPluginHostMode() is FroggersUiSurface-specific (a runtime flag
    // AppendTransportRow() reads, not part of the generic synth::ui::
    // Surface interface PortableSurface()/EditorSurface() expose) --
    // reaching it needs exactly one downcast. static_cast, not
    // dynamic_cast: engine_ is concretely synth::Engine<synth_froggers::
    // FroggersApp> (this class's own member type above), so
    // engine_.Application() is concretely FroggersApp&, and
    // FroggersApp::PortableSurface() (Froggers.hpp:52) is defined as
    // `return ui_;` over its own declared-concrete-type member
    // (`FroggersUiSurface ui_;`, Froggers.hpp:55) -- so the object
    // PortableSurface() returns a reference to is ALWAYS, provably, a
    // FroggersUiSurface; a runtime check here would guard a branch that
    // cannot exist without an edit to Froggers.hpp itself (omni-rule
    // defensive-code guidance: protect only against real edge cases; a
    // proven-safe static relationship is this codebase's own established
    // idiom for exactly this shape, e.g. app/FroggersMain.cpp's own
    // "T5.3c" comment on activeSession_'s concrete type).
    static_cast<synth_froggers::FroggersUiSurface&>(engine_.Application().PortableSurface())
        .SetPluginHostMode(true);

    // Group 7 (task 7.1): FroggersParameterModel's Parameters (and
    // FroggersApp's production DispatchAction seam, for Freeze) only exist
    // once engine_.Initialize() has returned (app_.Init() runs inside it,
    // this file's header comment) -- so the host parameter inventory is
    // built HERE, once, immediately after, and nowhere else.
    BuildHostParameterInventory();
    // Carry-forward 2 (group 5 review): pump engine_.MessageThreadTick()
    // from a message-thread juce::Timer, same as Sheaf Runtime.hpp
    // (Runtime.hpp:343 starts it at `config.uiFrameHz > 0 ? ... : 30`; this
    // plugin has no equivalent config knob, so 30Hz directly). Started once,
    // here, in the constructor -- mirrors Runtime::Start() calling
    // startTimerHz() once (Runtime.hpp:343), not per prepareToPlay().
    //
    // Review fix, Minor 3: juce::Timer::startTimer() (which startTimerHz()
    // calls) itself asserts JUCE_ASSERT_MESSAGE_MANAGER_EXISTS in Debug
    // builds (juce_Timer.cpp:372-377, "If you're calling this before (or
    // after) the MessageManager is running, then you're not going to get
    // any timer callbacks!") -- true for every headless CTest binary in
    // this group's own test suite (FroggersVstSmokeTest.cpp/
    // FroggersVstHostTests.cpp construct this class directly, no host, no
    // juce::MessageManager ever created). Guarded rather than left to trip:
    // a real host is guaranteed to have a MessageManager running before it
    // constructs any juce::AudioProcessor (every plugin-client entry point
    // runs on the message thread), so this guard is a no-op in production;
    // those same headless tests never need the REAL timer to fire anyway,
    // since PumpMessageThreadForTest() (this file's own header comment)
    // drives timerCallback() directly and deterministically.
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
        startTimerHz(30);
    }
}

FroggersPluginProcessor::~FroggersPluginProcessor() {
    // juce::Timer::stopTimer()'s own doc comment: "No more timer callbacks
    // will be triggered after this method returns" -- true unconditionally
    // when called from the message thread (the expected case for plugin
    // teardown, and the same thread every timerCallback() runs on, so there
    // is no concurrent in-flight call to race here). Called explicitly,
    // before any member (in particular engine_, which timerCallback()
    // touches) begins tearing down.
    stopTimer();
}

// Group 8 (task 8.1): one FroggersPluginEditor per call, exactly the
// juce::AudioProcessor::createEditor() contract (a fresh juce::Component the
// HOST owns and destroys -- see juce_AudioProcessor.h's own doc comment).
// Defined here rather than inline in the header so the header itself never
// needs to include FroggersPluginEditor.hpp's own JUCE-GUI chain
// (PortableJuceBackend.hpp et al.) -- every OTHER app/vst/ translation unit
// that only needs FroggersPluginProcessor.hpp (FroggersVstSmokeTest.cpp,
// most of FroggersVstHostTests.cpp) stays exactly as GUI-free to compile as
// it was before this group.
juce::AudioProcessorEditor* FroggersPluginProcessor::createEditor() { return new FroggersPluginEditor(*this); }

std::uint64_t FroggersPluginProcessor::NowMicros() const {
    // Same derivation as Runtime.hpp's own NowMicros() (:986-990): a
    // monotonic microsecond counter relative to construction time, not
    // wall-clock time -- MasterClock/message-batching only need monotonic
    // ordering and real elapsed duration between calls, not an absolute
    // epoch.
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime_).count());
}

void FroggersPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Runtime.hpp's audioDeviceAboutToStart (:594-599): engine_.Prepare() is
    // called on EVERY negotiation, not just the first -- see
    // FroggersAppCore::PrepareToPlay()'s own comment on why re-preparing
    // mid-session is a real, expected event (and why it re-asserts a
    // previously-started transport via desiredTransportRunning_).
    engine_.Prepare(sampleRate, samplesPerBlock);
}

void FroggersPluginProcessor::releaseResources() {
    // No traced teardown for audio state itself: Runtime.hpp's
    // audioDeviceStopped() override (:614) only clears a diagnostic counter
    // (activeInputChannels_) that this plugin has no equivalent of (zero
    // input channels, by construction). synth::Engine/FroggersAppCore
    // expose no stop/release-type hook at all -- ProcessBlock's own
    // transport-gated ASR (FroggersAppCore.hpp) is what silences output on
    // transport stop, not a host-driven teardown call.
    //
    // Review fix, Important 1: this USED to write hostClockEngaged_/
    // hostTempoValid_ directly. JUCE gives releaseResources() no thread
    // guarantee at all (juce_AudioProcessor.h's own declaration carries no
    // thread annotation, unlike processBlock()'s explicit audio-thread
    // contract), while this class's 30Hz Timer fires independently on the
    // message thread -- two plain writers on two possibly-different,
    // unsynchronized threads racing to decide the SAME engage/disengage
    // state is a real bug (could leave the clock re-engaged immediately
    // after this call thought it had disengaged it), not just an untidy
    // one. Fixed by making this call set ONLY its own dedicated atomic
    // request (releaseResourcesSeen_) -- timerCallback() (message thread)
    // remains the sole reader/writer of hostClockEngaged_/
    // nextHostClockTickMicros_/hostTempoValid_, exactly like every other
    // piece of 6.1/6.3 state that crosses threads in this class (see this
    // file's own header comment on the audio-thread/message-thread split).
    releaseResourcesSeen_.store(true, std::memory_order_release);
}

bool FroggersPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Binding bus posture (group 5 brief): stereo output only, no input bus.
    // The constructor's BusesProperties never declares an input bus, so
    // getMainInputChannelSet() is already always disabled(); checked
    // explicitly here anyway so a future edit that adds an input bus to the
    // constructor without updating this method fails loudly instead of
    // silently accepting a mono/multichannel input layout.
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) {
        return false;
    }
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FroggersPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    // Group 5 brief: "the plugin does NOT consume notes (buffer accepted and
    // ignored)". No note/MIDI wiring exists in FroggersAppCore's ProcessBlock
    // at all -- draining nothing out of `midiMessages` here is deliberate,
    // not an oversight.
    juce::ignoreUnused(midiMessages);

    // Review fix, Important 2 (teardown gap): a pure liveness heartbeat for
    // timerCallback()'s staleness detection -- incremented unconditionally,
    // every call, regardless of whether a playhead is present this block
    // (see processBlockCounter_'s own header comment for why this exists:
    // without it, a host that stops calling processBlock() at all -- no
    // releaseResources(), no more blocks, just silence -- would leave
    // hostTempoValid_/hostClockEngaged_ stuck at their last values forever).
    // Relaxed on both ends: this counter synchronizes no other data, it is
    // read only for inequality against a previously observed value.
    processBlockCounter_.fetch_add(1, std::memory_order_relaxed);

    // Group 6 (tasks 6.1/6.3): read the host playhead HERE, and only here --
    // juce::AudioPlayHead::getPosition()'s own doc comment: "You can ONLY
    // call this from your processBlock() method! Calling it at other times
    // will produce undefined behaviour ... some hosts will almost certainly
    // have multithreading issues if it's not called on the audio thread."
    // This method therefore never pushes anything onto engine_.UiBus() or
    // calls engine_.RequestSyncConfiguration() itself -- it only republishes
    // what it observed into the lock-free atomics timerCallback() (message
    // thread) reads and acts on. See this file's header comment for the
    // full audio-thread-safety trace on why the split is drawn exactly
    // here.
    if (juce::AudioPlayHead* playHead = getPlayHead()) {
        if (const auto position = playHead->getPosition()) {
            // -- 6.1: transport edge-trigger ---------------------------------
            const bool isPlayingNow = position->getIsPlaying();
            if (!haveHostPlayingBaseline_) {
                // The FIRST observation establishes the baseline rather than
                // firing: there is no prior state to TRANSITION from yet
                // (group 6 brief: "on host play-state TRANSITIONS only"), so
                // a plugin instantiated mid-playback stays silent (matches
                // group 5's own "silent by construction" default) until the
                // host's transport next actually toggles.
                haveHostPlayingBaseline_ = true;
                lastHostIsPlaying_ = isPlayingNow;
            } else if (isPlayingNow != lastHostIsPlaying_) {
                // Single-slot, coalescing store -- same "control-rate,
                // human-paced action" idiom FroggersAppCore::
                // RequestBankSelect/RequestEncoderPress already use
                // (FroggersAppCore.hpp:550-556's own comment), applied here
                // because pushing directly from this thread is not an
                // option (see this file's header comment).
                pendingTransportEdge_.store(
                    static_cast<int>(isPlayingNow ? PendingTransportEdge::kStart : PendingTransportEdge::kStop),
                    std::memory_order_relaxed);
                lastHostIsPlaying_ = isPlayingNow;
            }

            // -- 6.3: republish the host's reported tempo, if any -----------
            if (const auto bpm = position->getBpm()) {
                hostTempoBpm_.store(*bpm, std::memory_order_relaxed);
                hostTempoValid_.store(true, std::memory_order_release);
            } else {
                // Playhead present, but this block's position carries no
                // bpm -- collapse to the same "nothing usable" signal an
                // absent playhead produces below (timerCallback() cannot
                // and should not tell the two apart: either way there is no
                // host tempo to slave to right now).
                hostTempoValid_.store(false, std::memory_order_release);
            }
        } else {
            hostTempoValid_.store(false, std::memory_order_release);
        }
    } else {
        // No playhead at all (standalone hosting, or this file's own
        // no-host test constructions) -- group 6 brief: "Handle absent
        // playhead (standalone hosting) gracefully: no messages." The
        // transport baseline is deliberately left untouched (nothing to
        // compare against changed this block); tempo is marked not-usable,
        // which is also what drives 6.3's teardown disengage in
        // timerCallback() below.
        hostTempoValid_.store(false, std::memory_order_release);
    }

    const int numSamples = buffer.getNumSamples();
    std::array<float*, 2> outputPointers{buffer.getWritePointer(0), buffer.getWritePointer(1)};

    // Same AudioBlock shape SynthRig::RunOneBlockAt builds
    // (SynthRig.hpp:513-521) for zero input channels: inputs=nullptr,
    // numInputChannels/numRequestedInputChannels=0 (matches
    // FroggersAppCore::Config()'s numAudioInputs=0 -- Engine::ProcessBlock
    // asserts block.numRequestedInputChannels == config_.numAudioInputs,
    // Engine.hpp:410). startSample/clockPlan are OUT params the engine sets
    // itself during ProcessBlock (Engine.hpp:402-405) -- left
    // default-constructed here, exactly as SynthRig does.
    synth::AudioBlock block;
    block.inputs = nullptr;
    block.outputs = outputPointers.data();
    block.numInputChannels = 0;
    block.numOutputChannels = static_cast<int>(outputPointers.size());
    block.numFrames = static_cast<std::size_t>(numSamples);
    block.numRequestedInputChannels = 0;

    engine_.ProcessBlock(block, NowMicros());

    // Group 7 (task 7.1): publish every host-exposed parameter's current
    // display value into its own atomic UIState snapshot, for
    // PumpHostParameterBridge() (message thread) to read. AFTER
    // engine_.ProcessBlock() (so this block's own ProcessSamplePhase1/2
    // slewing has already run), on THIS thread (the audio thread owns every
    // Parameter's internal uiDisplayCenters_/uiDisplaySpreadEnergies_ etc,
    // the non-atomic storage Parameter::PopulateUIState() reads -- exactly
    // the same audio-thread requirement synth::Engine's own ProcessBlock
    // already honors for its OWN throttled `manager_.PopulateUIState(...)`
    // call, Engine.hpp's own "7. throttled PopulateUIState every
    // uiPublishInterval_ blocks" step). This class calls
    // Parameter::PopulateUIState() directly, per parameter, UNTHROTTLED and
    // bank-selection-independent, rather than reusing that one: Engine's
    // own publish only reaches ParameterManager::UIState's
    // slots[0].cells[position], which reflects Bank::VisibleParameter(ix)
    // of whichever bank the shared BankSlot currently has SELECTED (traced
    // via BankSlot::HandleSetAbsolute/Bank::FindVisibleCell,
    // ParameterModulation.cpp) -- with one physical BankSlot shared by all
    // six banks, that path can only ever see ONE bank's 16 parameters at a
    // time, never all 91 simultaneously. Calling PopulateUIState()
    // per-Parameter instead sidesteps BankSlot/Bank entirely (every
    // Parameter in this group gets its ProcessSamplePhase1/2 slewing every
    // sample regardless of bank selection -- FroggersParameterModel::
    // ProcessSample() drives group_->ProcessSamplePhase1/2 for the WHOLE
    // group, not just the visible bank), so host readback for a bank that
    // is not currently "selected" stays live and correct. Cost is
    // negligible: a handful of relaxed atomic stores per parameter, once
    // per audio callback, no allocation -- see HostParamEntry::uiState's
    // own comment.
    for (HostParamEntry& entry : hostParams_) {
        if (entry.coreParam != nullptr) {
            entry.coreParam->PopulateUIState(*entry.uiState);
        }
    }

    // Engine::MessageThreadTick() (patch-response draining, serialization
    // arena growth, MIDI output processor pumps) is deliberately NOT called
    // from here -- it performs non-realtime-safe work (patch IO, heap
    // growth), exactly like Runtime.hpp, which drives it from a
    // message-thread juce::Timer (:975 call site), never the audio
    // callback. Carry-forward 2 (group 5 review) wires the same pump for
    // this plugin: see timerCallback() below (started in the constructor
    // via startTimerHz(30)).
}

void FroggersPluginProcessor::timerCallback() {
    // Tick order mirrors Sheaf Runtime.hpp's own timerCallback()
    // (Runtime.hpp:974-984): the engine's message-thread tick runs first.
    engine_.MessageThreadTick();

    // -- 6.1: drain the pending host transport edge, if any -----------------
    // Routed through the SAME production seam the Play/Stop buttons use --
    // FroggersApp::PortableSurface() (Froggers.hpp:52) returns the exact
    // FroggersUiSurface instance already Attach()-ed to this engine
    // (Froggers.hpp:49, run once during engine_.Initialize() above), so
    // DispatchAction() here runs the literal HandleAction kPlay/kStop
    // branches (FroggersUiSurface.hpp:1826-1876) -- including their
    // SetFreezeLatched(false) disarm and happens-before-ordered
    // PushMessage/SetDesiredTransportRunning pair -- rather than a
    // hand-mirrored copy of that logic that could drift from it. No editor
    // is required for this: DispatchAction() does not touch anything
    // editor-owned.
    const int pendingEdge = pendingTransportEdge_.exchange(
        static_cast<int>(PendingTransportEdge::kNone), std::memory_order_relaxed);
    if (pendingEdge == static_cast<int>(PendingTransportEdge::kStart)) {
        engine_.Application().PortableSurface().DispatchAction(
            synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
    } else if (pendingEdge == static_cast<int>(PendingTransportEdge::kStop)) {
        engine_.Application().PortableSurface().DispatchAction(
            synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
    }

    // -- 6.3: host tempo via the existing external-clock slave path ---------
    // Trace (task 6.3): MasterClock::SetTempoBpm no-ops unconditionally
    // while `syncConfig_.receiveClock` is true (src/MasterClock.cpp:
    // 963-965) -- that suppression is a property of the flag, not of HOW
    // the tempo estimate itself gets updated. MasterClock::
    // HandleExternalClock (src/MasterClock.cpp:1097-1193) is the only path
    // that updates `activeBpm_` while slaved, and it derives that estimate
    // from the MEASURED INTERVAL between successive external-origin
    // MessageIn::Clock ticks (recoveredBpm = 60e6 / (ppqn *
    // filteredPeriodMicros), :1174-1177). So reaching "host tempo changes
    // reach the clock" AND "requests suppressed" through the SAME existing
    // mechanism requires SYNTHESIZING that tick stream -- calling
    // RequestTempoBpm(hostBpm) instead would funnel into the very
    // SetTempoBpm that no-ops while slaved, so it cannot be what updates
    // the tempo once slaved; the brief's two candidates are not actually
    // interchangeable once receiveClock is engaged, which is why this group
    // synthesizes ticks rather than requesting tempo directly.
    //
    // Zero core edits either way: RouteRealtimeBatch already routes any
    // Origin::ExternalMidi MessageIn::Clock to
    // MasterClock::HandleExternalClock (Engine.hpp:907-916), and
    // Engine::RequestSyncConfiguration is an existing lock-free atomic
    // request already applied once per block by ProcessBlock's own
    // ApplySyncConfig call (Engine.hpp:346-349, 489-495) -- both pre-date
    // this group. grep across app/ and the runtime shell turned up no
    // OTHER production caller of RequestSyncConfiguration at all: this
    // plugin is the first thing that ever engages external-clock slaving
    // outside a test rig (FroggersSurfaceTests.cpp:2095's SetSyncConfig is
    // a synchronous test-only bypass), so there is no existing "how
    // MIDI-clock slaving engages" production call site to defer to beyond
    // this mechanism itself.
    // Review fix, Important 2 (teardown gap): hostTempoValid_ on its own
    // only tells us what processBlock() last PUBLISHED -- if processBlock()
    // stops being called at all (host suspends/disables this track without
    // ever calling releaseResources()), that published value simply freezes
    // at whatever it last was and never goes false on its own. Detected via
    // processBlockCounter_ (a pure liveness heartbeat, see its own header
    // comment): if the counter has not moved since the last pump, this pump
    // measures how long it has been since it last DID move
    // (lastActivityMicros_, using the same NowMicros() clock as everywhere
    // else in this class); once that elapsed time reaches
    // kStaleActivityWindowMicros (~1 second -- see that constant's own
    // comment for the carry-forward that widened this from a fixed 3-pump
    // count to this time-based window and why), the host is treated as
    // gone. At this class's own 30Hz pump rate (~33ms/pump) and a typical
    // real-world audio block far shorter than that (e.g. 256 samples @
    // 48kHz ~= 5.3ms), processBlock() ordinarily fires several times BETWEEN
    // two consecutive pumps whenever the host is genuinely still calling
    // it -- observing literally zero calls for a full second is far outside
    // normal scheduling jitter and reliably means processBlock() has
    // stopped being called at all.
    //
    // releaseResourcesSeen_ (review fix, Important 1: see its own header
    // comment) is folded into the SAME staleness signal rather than
    // handled as a separate branch -- releaseResources() is simply a
    // stronger, immediate version of "the host is gone," so it forces
    // activityStale true on this pump without waiting out the window.
    const std::uint64_t nowMicros = NowMicros();
    const std::uint64_t observedBlockCounter = processBlockCounter_.load(std::memory_order_relaxed);
    bool activityStale = false;
    if (observedBlockCounter != lastObservedProcessBlockCounter_) {
        lastObservedProcessBlockCounter_ = observedBlockCounter;
        lastActivityMicros_ = nowMicros;
    } else {
        activityStale = (nowMicros - lastActivityMicros_) >= kStaleActivityWindowMicros;
    }
    if (releaseResourcesSeen_.exchange(false, std::memory_order_acq_rel)) {
        activityStale = true;
    }

    const bool hostTempoValid = hostTempoValid_.load(std::memory_order_acquire);
    const double hostTempoBpm = hostTempoBpm_.load(std::memory_order_relaxed);
    const bool hostTempoUsable = !activityStale && hostTempoValid && std::isfinite(hostTempoBpm) && hostTempoBpm > 0.0;

    if (hostTempoUsable) {
        // Reuses the SAME nowMicros captured above for the activity-staleness
        // check -- one NowMicros() call per pump, not two.
        if (!hostClockEngaged_) {
            // Engage. receiveTransport is deliberately left false (its
            // SyncConfig default): 6.1 above pushes plain Origin::Internal
            // Start/Stop -- exactly the Play/Stop buttons' own messages --
            // not Origin::ExternalMidi transport commands, so the
            // transport-arming state machine HandleExternalTransport would
            // add (ArmedStart/ArmedContinue, splice-crossing enumeration) is
            // neither wanted nor engaged here; only tempo is slaved.
            engine_.RequestSyncConfiguration(synth::SyncConfig{.receiveClock = true});
            hostClockEngaged_ = true;
            nextHostClockTickMicros_ = nowMicros;
        }

        // Standard MIDI clock rate: SyncConfig::ppqn's own default (24
        // pulses per quarter note, synth/MasterClock.hpp), READ rather than
        // restated as a literal so this can never silently drift from
        // Sheaf's own default if it ever changes. Ticks are timestamped
        // this-many-micros apart using the CURRENT host tempo, so
        // HandleExternalClock's interval-based estimator recovers the right
        // bpm even though every tick pushed in a given pump actually
        // arrives in one batch -- MessageInBus::Pop gates on the message's
        // OWN timestamp field, not wall-clock delivery time
        // (ParameterModulation.cpp:4007-4020), so a burst of correctly-
        // spaced-by-timestamp ticks estimates tempo exactly as a real,
        // evenly-spaced hardware clock would.
        constexpr int kHostClockPpqn = synth::SyncConfig{}.ppqn;
        const double tickIntervalMicros = 60'000'000.0 / (static_cast<double>(kHostClockPpqn) * hostTempoBpm);
        if (std::isfinite(tickIntervalMicros) && tickIntervalMicros > 0.0) {
            // Capped: if this timer went an unusually long time between
            // pumps (e.g. the host suspended the plugin), do not replay an
            // unbounded backlog of ticks in one call.
            constexpr int kMaxTicksPerPump = 64;
            int ticksPushed = 0;
            while (nextHostClockTickMicros_ <= nowMicros && ticksPushed < kMaxTicksPerPump) {
                engine_.UiBus().Push(synth::MessageIn::Clock(
                    nextHostClockTickMicros_, synth::MessageIn::Origin::ExternalMidi, /*externalControllerSlot=*/0));
                nextHostClockTickMicros_ += static_cast<std::uint64_t>(tickIntervalMicros);
                ++ticksPushed;
            }
            if (ticksPushed == kMaxTicksPerPump) {
                // Resynchronize to "now" rather than keep falling further
                // behind on the next pump too.
                nextHostClockTickMicros_ = nowMicros + static_cast<std::uint64_t>(tickIntervalMicros);
            }
        }
    } else if (hostClockEngaged_) {
        // -- Teardown (task 6.3's "mind teardown") ---------------------------
        // MasterClock's OWN external-source staleness check
        // (ExpireExternalSource/ClearExternalSource, src/MasterClock.cpp:
        // 350-393) is reactive, not ambient: it only runs from inside
        // AcceptExternalSource, itself only reached when ANOTHER clock/
        // transport message arrives (src/MasterClock.cpp:395-419) -- if the
        // host truly vanishes, no such message ever arrives again, so that
        // path alone would never fire. And even when it DOES fire, it only
        // resets `acquisitionState_`/`source_`, never `syncConfig_.
        // receiveClock` itself (ClearExternalSource, :350-367) -- so
        // TempoExternallyClocked() (FroggersAppCore.hpp:1267, reads
        // SyncConfiguration().receiveClock directly) would stay stuck true,
        // permanently suppressing the BPM slider with no live source
        // driving it. There is no existing production disengage call site
        // to mirror (this group is the first production caller of
        // RequestSyncConfiguration at all) -- so this IS the disengage,
        // reached by EITHER of two independent triggers folded into
        // hostTempoUsable above (both handled entirely on this, the message
        // thread -- see hostClockEngaged_'s own header comment on why that
        // matters): (1) processBlock() itself observes the host reporting
        // no usable playhead/bpm any more, or (2) processBlockCounter_'s
        // staleness streak proves processBlock() has stopped being called
        // at all, whether or not releaseResources() ever runs.
        // releaseResourcesSeen_ additionally forces this branch immediately
        // rather than waiting out the staleness window, for the ordinary
        // "host cleanly tore the track down" case.
        engine_.RequestSyncConfiguration(synth::SyncConfig{});
        hostClockEngaged_ = false;
    }

    // -- 7.1: stable-ID host parameter surface, both directions -------------
    // Same message-thread-only discipline as 6.1/6.3 above: this is the
    // ONLY place that pushes MessageIn::SelectParamBank/ParamSetAbsolute or
    // calls DispatchAction(kFreeze) for a host-driven write, and the ONLY
    // place that reads the per-parameter UIState snapshots processBlock()
    // (audio thread) publishes into hostParams_[i].uiState -- see
    // PumpHostParameterBridge()'s own comment for the full bidirectional
    // trace.
    PumpHostParameterBridge();

    // Group 8: repaint the editor, if one is open, LAST -- after every other
    // state mutation this pump makes above (message tick, transport, tempo,
    // parameter bridge) -- so a live editor's RefreshFromSurface() rebuilds
    // its tree from THIS pump's freshest state, never a stale mid-pump
    // snapshot. Zero cost when no editor is open (editorRepaintHook_ is
    // empty, SetEditorRepaintHook()'s own comment).
    if (editorRepaintHook_) {
        editorRepaintHook_();
    }
}

void FroggersPluginProcessor::TestStartTransport() {
    // Carry-forward 3 (group 5 review): group 5's version of this seam
    // pushed MessageIn::Start/SetDesiredTransportRunning directly, missing
    // the T7.7 Freeze-latch disarm the real Play button performs
    // (FroggersUiSurface.hpp:1826-1846). Routed through DispatchAction (the
    // production seam, see timerCallback()'s own comment) instead of a
    // second hand-mirrored fix, so this seam and the real 6.1 producer
    // cannot independently drift from HandleAction's actual kPlay branch.
    engine_.Application().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kPlay));
}

void FroggersPluginProcessor::TestStopTransport() {
    // Carry-forward 3: see TestStartTransport()'s own comment.
    engine_.Application().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kStop));
}

// -- 7.1: stable-ID host parameter surface -----------------------------------
void FroggersPluginProcessor::BuildHostParameterInventory() {
    synth_froggers::FroggersParameterModel& model = engine_.Application().Parameters();
    const auto& layouts = synth_froggers::FroggersBankLayouts();

    // Sized from the model's OWN enumeration constants, never a hardcoded
    // literal (governing spec, task 7.2's own "Count" requirement) -- 6
    // banks * 14 page parameters + 6 per-bank Crispy + 1 shared Crunchy + 1
    // Freeze.
    hostParams_.reserve(synth_froggers::kFroggersBankCount * synth_froggers::kFroggersParamsPerBank
                         + synth_froggers::kFroggersBankCount + 1 + 1);

    // JUCE's own versionHint doc comment (juce_ParameterID.h): "Influences
    // parameter ordering in Audio Unit plugins" -- a monotonically
    // increasing hint over this SAME stable construction order is the JUCE
    // convention for it; it plays no role in this class's own identity or
    // addressing (parameterID/bankIx/position do that), so it is not part
    // of the stability contract HostParamStableId()'s own comment makes.
    int versionHint = 1;

    for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount; ++bankIx) {
        const synth_froggers::FroggersBankLayout& layout = layouts[bankIx];

        for (std::size_t paramIx = 0; paramIx < synth_froggers::kFroggersParamsPerBank; ++paramIx) {
            synth::Parameter& coreParam = model.PageParameter(bankIx, paramIx);

            HostParamEntry entry;
            entry.kind = HostParamEntry::Kind::kPageParam;
            entry.coreParam = &coreParam;
            entry.bankIx = bankIx;
            entry.position = paramIx;
            entry.needsBankSelect = true;
            entry.uiState = std::make_unique<synth::Parameter::UIState>();
            // voiceCapacity=1 (FroggersParameterModel::kNumVoices, D4's mono
            // model); this bridge needs no modulator/gesture color info, so
            // both those capacities are 0 -- Parameter::UIState::Configure()
            // (ParameterModulation.cpp) accepts 0 for either with no error,
            // it simply allocates zero-length arrays for them.
            entry.uiState->Configure(/*voiceCapacity=*/1, /*modulatorColorCapacity=*/0, /*gestureColorCapacity=*/0);
            // Matches the REAL starting value FroggersParameterModel::Init()
            // seeds this exact Parameter with (RegisterParameter's own
            // `.defaultValue = spec.defaultValue`, FroggersParameters.hpp)
            // -- read from the SAME FroggersParamSpec that call reads, not
            // re-guessed.
            const float defaultValue = layout.params[paramIx].defaultValue;
            entry.shadowNormalized = defaultValue;

            // name = coreParam.Name(): the qualified "<Bank> <Param>"
            // display name FroggersParameterModel::Init() itself computes
            // and registers (`qualifiedName = layout.name + " " +
            // spec.name`, FroggersParameters.hpp) -- read directly from the
            // live Parameter, not re-derived, so it can never drift from
            // the model's own display-name authority.
            auto* juceParam = new juce::AudioParameterFloat(
                juce::ParameterID(HostParamStableId(bankIx, paramIx), versionHint++), juce::String(coreParam.Name()),
                juce::NormalisableRange<float>(0.0f, 1.0f), defaultValue);
            entry.juceParam = juceParam;
            addParameter(juceParam);
            hostParams_.push_back(std::move(entry));
        }

        // Crispy (position kFroggersCrispySlot==14): per-bank (D5a), so
        // still needs bank selection like an ordinary page parameter.
        {
            synth::Parameter& coreParam = model.Crispy(bankIx);

            HostParamEntry entry;
            entry.kind = HostParamEntry::Kind::kCrispy;
            entry.coreParam = &coreParam;
            entry.bankIx = bankIx;
            entry.position = synth_froggers::kFroggersCrispySlot;
            entry.needsBankSelect = true;
            entry.uiState = std::make_unique<synth::Parameter::UIState>();
            entry.uiState->Configure(1, 0, 0);
            // Crispy's own RegisterParameter call never sets `.defaultValue`
            // (FroggersParameters.hpp), leaving ParameterConfig's own
            // default (0.0f) -- matched here, not re-guessed.
            constexpr float kCrispyDefaultValue = 0.0f;
            entry.shadowNormalized = kCrispyDefaultValue;

            auto* juceParam = new juce::AudioParameterFloat(
                juce::ParameterID(HostParamStableIdCrispy(bankIx), versionHint++), juce::String(coreParam.Name()),
                juce::NormalisableRange<float>(0.0f, 1.0f), kCrispyDefaultValue);
            entry.juceParam = juceParam;
            addParameter(juceParam);
            hostParams_.push_back(std::move(entry));
        }
    }

    // Crunchy (position kFroggersCrunchySlot==15): ONE Parameter object,
    // the SAME pointer registered at slot 15 in every bank (task 4.2/4.6,
    // FroggersParameters.hpp) -- so unlike page parameters/Crispy, writing
    // it needs no bank selection at all: whichever bank the shared BankSlot
    // currently has selected, position 15 always resolves to this same
    // Parameter*.
    {
        synth::Parameter& coreParam = model.Crunchy();

        HostParamEntry entry;
        entry.kind = HostParamEntry::Kind::kCrunchy;
        entry.coreParam = &coreParam;
        entry.bankIx = 0;  // Unused for addressing (needsBankSelect is false); any bank resolves the same Parameter*.
        entry.position = synth_froggers::kFroggersCrunchySlot;
        entry.needsBankSelect = false;
        entry.uiState = std::make_unique<synth::Parameter::UIState>();
        entry.uiState->Configure(1, 0, 0);
        // Crunchy's own RegisterParameter call never sets `.defaultValue`
        // either (FroggersParameters.hpp) -- same 0.0f default as Crispy.
        constexpr float kCrunchyDefaultValue = 0.0f;
        entry.shadowNormalized = kCrunchyDefaultValue;

        auto* juceParam = new juce::AudioParameterFloat(
            juce::ParameterID(kHostParamStableIdCrunchy, versionHint++), juce::String(coreParam.Name()),
            juce::NormalisableRange<float>(0.0f, 1.0f), kCrunchyDefaultValue);
        entry.juceParam = juceParam;
        addParameter(juceParam);
        hostParams_.push_back(std::move(entry));
    }

    // Freeze: NOT a ParameterManager parameter (governing brief: "group 6
    // wired kFreeze via DispatchAction; the host parameter must drive the
    // same production seam and preserve SetFreezeLatched semantics") --
    // coreParam/uiState stay null; PumpHostParameterBridge()'s Freeze
    // branch reads FroggersAppCore::FreezeLatched() (already an atomic,
    // acquire-load getter -- FroggersAppCore.hpp -- safe cross-thread with
    // no PopulateUIState snapshot needed) and writes via
    // PortableSurface().DispatchAction(kFreeze), the EXACT seam group 6's
    // 6.1 transport edge-trigger and this file's own TestStartTransport()
    // already use, and the exact seam the real Freeze button itself uses
    // (FroggersUiSurface.hpp's kFreeze branch). No plugin-side MIDI
    // mapping/learn is introduced anywhere by this -- Freeze becomes
    // automatable/host-MIDI-mappable purely by being an ordinary
    // juce::AudioProcessorParameter, exactly like every other parameter
    // above.
    {
        HostParamEntry entry;
        entry.kind = HostParamEntry::Kind::kFreeze;
        // FreezeLatched() defaults false (FroggersAppCore.hpp) -- matched
        // here, not re-guessed.
        entry.shadowNormalized = 0.0f;

        auto* juceParam = new juce::AudioParameterBool(
            juce::ParameterID(kHostParamStableIdFreeze, versionHint++), "Freeze", /*defaultValue=*/false);
        entry.juceParam = juceParam;
        addParameter(juceParam);
        hostParams_.push_back(std::move(entry));
    }
}

// PumpHostParameterBridge() -- message-thread-only (called from
// timerCallback(), never processBlock() -- see this file's header comment
// on the audio-thread/message-thread split). One pass over hostParams_ per
// pump, both directions, per entry:
//
//   host -> core: if the host has moved this parameter since the shadow
//   last agreed with it (`juceParam->getValue() != entry.shadowNormalized`
//   beyond a small float epsilon), push exactly the write a real host
//   automation event or MIDI-mapped-by-the-DAW controller move would
//   produce -- MessageIn::ParamSetAbsolute at this entry's (slotIx=0,
//   position), preceded by MessageIn::SelectParamBank when (and only when)
//   the target bank is not already the one this class itself last selected
//   (lastSelectedBankIxForHostWrites_). Both messages land on the SAME
//   engine_.UiBus() this class has pushed onto since group 6 (6.1/6.3),
//   applied in push order within ONE MessageInBus::Process() call inside
//   the next engine_.ProcessBlock() (Engine.hpp's DrainMessageBus(uiBus_,
//   ...) step) -- so the select is guaranteed to land before the set it
//   precedes, even though bank selection ALSO has its own, separate,
//   audio-thread Request*/pending*_ bridge (FroggersAppCore::
//   RequestBankSelect(), applied inside ProcessFrame() -- traced and
//   deliberately NOT used AS A REPLACEMENT here: ProcessFrame() runs AFTER
//   DrainMessageBus(uiBus_, ...) within one Engine::ProcessBlock() call
//   (Engine.hpp's own binding step order), so a RequestBankSelect() pushed
//   IN PLACE OF the direct push, in the same pump as a ParamSetAbsolute,
//   would apply ONE BLOCK LATE relative to the value write it was meant to
//   precede -- landing the write on whatever bank was selected BEFORE this
//   pump instead of the intended target. MessageIn::SelectParamBank has no
//   such gap: it drains through the exact same uiBus_ queue, in the exact
//   same DrainMessageBus() call, as ParamSetAbsolute itself).
//
//   CARRY-FORWARD (group 7 review, addressed group 8): the direct
//   MessageIn::SelectParamBank push above updates BankSlot::SelectedBank()
//   (the real selection authority, ParameterManager::SelectBankForSlot) but
//   -- on its own -- never touched FroggersAppCore::activeBankIx_/drillIn_
//   at all, since those are updated ONLY by RequestBankSelect()'s own
//   pending-atomic, drained inside ProcessFrame() (FroggersAppCore.hpp:
//   557-568, 622-664, which ALSO reconstructs drillIn_ for the new bank).
//   Consequence: after any cross-bank host write, ActiveBankIndex()/
//   ActiveDrillIn() went stale, and RandomizePage()/ResetPage()
//   (FroggersAppCore.hpp:687-706), which act on *drillIn_, silently kept
//   targeting whatever bank was active BEFORE the automation -- even though
//   FroggersUiSurface::CurrentBankIndex() (app/FroggersUiSurface.hpp:
//   1872-1882, what the editor actually renders -- task 8.1's render-host
//   seam trace) reads the real BankSlot selection directly and was NEVER
//   stale. Fix: ALSO call RequestBankSelect() here, at the same "bank
//   actually changed" gate the direct push already uses -- both this call
//   and the direct push above are applied within the SAME upcoming
//   ProcessBlock() call (DrainMessageBus runs first and re-selects the
//   already-selected bank, a harmless no-op per BankSlot::SelectBank's own
//   early-exit; ProcessFrame runs after and reconstructs drillIn_ for the
//   real target bank), so this closes the staleness gap without
//   reintroducing the one-block-late defect the comment above rules out for
//   a REPLACEMENT. Single-slot coalescing (RequestBankSelect()'s own
//   contract) means multiple different-bank writes batched into one pump
//   still converge correctly: the LAST RequestBankSelect() call in program
//   order is also the LAST SelectParamBank message DrainMessageBus applies,
//   so both paths agree on the same final bank either way. No core header
//   edit needed: RequestBankSelect() is an existing PUBLIC
//   FroggersAppCore method already called from FroggersUiSurface.hpp;
//   this is a new call site, not a new core capability. Verified by
//   FroggersVstHostTests.cpp's
//   host_automation_in_a_non_visible_bank_keeps_active_bank_and_page_actions_in_sync.
//
//   Crunchy needs no SelectParamBank at all (needsBankSelect is false --
//   see HostParamEntry::needsBankSelect's own comment); the shared bank
//   selection is deliberately left wherever the write leaves it afterward
//   (no "restore the previous bank" step). AUDIT CORRECTION (was: "with no
//   editor in this group, nothing observes the shared BankSlot's current
//   selection except this bridge itself" -- true when this bridge was
//   written in group 6, false today): group 8 added the real editor, and
//   FroggersUiSurface::CurrentBankIndex() (cited above) reads this exact
//   selection, so a host automation write to a bank other than the one
//   currently showing now visibly flips the editor to it, with no
//   restore. Whether that is the right behavior -- vs. restoring the
//   previously-viewed bank, or some other policy -- is a product decision
//   this bridge does not make unilaterally: it belongs to
//   openspec/changes/frogg3rs-host-state-and-visibility's "Cross-bank
//   automation policy" task group (design D), which owns tracing the
//   options and getting an operator decision before this behavior changes.
//   Until then, this bridge's own behavior is unchanged: the bank a host
//   automation event most recently touched stays selected, which also
//   mirrors what a real user turning that specific parameter's own
//   encoder would leave behind.
//
//   core -> host: reached only when the host did NOT just write this
//   parameter THIS pump (host writes always win a same-pump race, and
//   `continue` past the core-check below, in the per-entry branches above,
//   deliberately skip it -- the message this pump just pushed has not been
//   applied to the core yet, so reading uiState/FreezeLatched() THIS pump
//   would still see the OLD value and could otherwise echo it straight
//   back over the host's fresh write). If the core's published value
//   (entry.uiState->values[0], or FreezeLatched() for kFreeze) differs from
//   the shadow, this is a genuine core-side change (randomize, a scene
//   change, or -- ordinarily -- the settling tail of an EARLIER host write
//   still slewing toward its target via Parameter's own ~10Hz
//   uiDisplayCenterAlpha one-pole, ParameterModulation.hpp) --
//   setValueNotifyingHost() relays it and the shadow is updated to match.
//
//   Feedback-guard: the single shadowNormalized per entry is what makes
//   this safe from an endless notify loop. The moment this method itself
//   calls setValueNotifyingHost(coreValue), it also sets
//   shadowNormalized = coreValue -- so on the VERY NEXT pump,
//   juceParam->getValue() (unchanged since nothing else wrote it) still
//   equals shadowNormalized, the host-check finds no diff, and only the
//   core-check runs; that check only fires again once the core's ACTUAL
//   published value has moved again. Because Parameter's own slew is a
//   one-pole IIR converging on floating-point hardware, it reaches a true,
//   bit-exact fixed point in finite time (the update term shrinks below the
//   float ULP at that magnitude and the stored value stops changing bit for
//   bit) -- so this is not merely "unlikely to loop forever," it provably
//   goes to a fixed count of notifies and stops. Task 7.2's own
//   no-feedback test proves this by pumping well past that settle window
//   and asserting the notify count has gone flat, plus a positive control
//   (a genuine core-side change DOES still notify) proving the guard
//   discriminates rather than just suppressing everything.
void FroggersPluginProcessor::PumpHostParameterBridge() {
    constexpr float kEpsilon = 1.0e-5f;

    for (HostParamEntry& entry : hostParams_) {
        if (entry.kind == HostParamEntry::Kind::kFreeze) {
            const bool hostTarget = entry.juceParam->getValue() >= 0.5f;
            const bool shadowBool = entry.shadowNormalized >= 0.5f;
            if (hostTarget != shadowBool) {
                // host -> core: DispatchAction(kFreeze) TOGGLES
                // (FroggersUiSurface.hpp's own kFreeze branch: `engaging =
                // !app_->FreezeLatched(); app_->SetFreezeLatched(engaging)`)
                // -- converted here into "set to this absolute target" by
                // only dispatching when the toggle would actually move the
                // latch to match what the host asked for.
                if (hostTarget != engine_.Application().FreezeLatched()) {
                    engine_.Application().PortableSurface().DispatchAction(
                        synth::ui::Action::Named(synth_froggers::FroggersActions::kFreeze));
                }
                entry.shadowNormalized = hostTarget ? 1.0f : 0.0f;
                continue;
            }

            const bool coreValue = engine_.Application().FreezeLatched();
            if (coreValue != shadowBool) {
                entry.juceParam->setValueNotifyingHost(coreValue ? 1.0f : 0.0f);
                entry.shadowNormalized = coreValue ? 1.0f : 0.0f;
            }
            continue;
        }

        const float hostValue = entry.juceParam->getValue();
        if (std::fabs(hostValue - entry.shadowNormalized) > kEpsilon) {
            // host -> core.
            if (entry.needsBankSelect && entry.bankIx != lastSelectedBankIxForHostWrites_) {
                engine_.UiBus().Push(synth::MessageIn::SelectParamBank(NowMicros(), /*slotIx=*/0, entry.bankIx));
                // Carry-forward (group 7 review, see this method's own
                // header comment for the full trace): ALSO keep
                // FroggersAppCore::activeBankIx_/drillIn_ in sync via the
                // core's own request bridge -- both this call and the
                // direct push above land within the SAME upcoming
                // ProcessBlock() call, so the value write immediately below
                // still resolves against the correct, freshly-selected
                // bank exactly as before this fix.
                engine_.Application().RequestBankSelect(entry.bankIx);
                lastSelectedBankIxForHostWrites_ = entry.bankIx;
            }
            engine_.UiBus().Push(
                synth::MessageIn::ParamSetAbsolute(NowMicros(), /*slotIx=*/0, entry.position, hostValue));
            entry.shadowNormalized = hostValue;
            continue;
        }

        // core -> host (only reached when the host did not just write this
        // parameter this same pump -- see this method's own header
        // comment).
        const float coreValue = entry.uiState->values[0].load(std::memory_order_relaxed);
        if (std::fabs(coreValue - entry.shadowNormalized) > kEpsilon) {
            entry.juceParam->setValueNotifyingHost(coreValue);
            entry.shadowNormalized = coreValue;
        }
    }
}

}  // namespace frogg3rs_vst

// JUCE plugin-client entry point (juce_audio_plugin_client, every format
// wrapper calls createPluginFilterOfType() -> this): production processor
// only, never the test constructor overload.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new frogg3rs_vst::FroggersPluginProcessor();
}
