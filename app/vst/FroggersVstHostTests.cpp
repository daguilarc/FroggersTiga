// FroggersVstHostTests.cpp -- task 6.4 (froggers-vst-host delta, group 6:
// DAW-external transport, host tempo, plugin-mode editor row), extended by
// group 8 (task 8.2: the carry-forward bank-agreement test and an
// end-to-end plugin-mode-surface test through the real processor). CTest-
// wired via app/vst/CMakeLists.txt's add_test(), same "drive the REAL
// compiled FroggersPluginProcessor, not a second copy" shape as
// FroggersVstSmokeTest.cpp (links the same FroggersVst shared-code target).
// The one thing group 8 does NOT test in this binary is the real
// FroggersPluginEditor's own construction/destruction -- that needs a
// juce::Component, not just a juce::AudioProcessor, and is isolated in its
// own CTest binary instead (FroggersVstEditorTest.cpp's own header comment
// explains why).
//
// Six things this file needs proven, each its own section below:
//   1. Transport edges (task 6.1): a fake juce::AudioPlayHead drives
//      processBlock()'s real edge-detector; PumpMessageThreadForTest()
//      (timerCallback() itself, since a headless CTest binary runs no
//      message loop to fire a real juce::Timer -- see
//      FroggersPluginProcessor.hpp's own comment on that seam) drives the
//      real producer. Counted AT THE BUS (UiBusPendingCountForTest(),
//      engine_.UiBus().Size()) rather than inferred from eventual state, so
//      "exactly one message per transition, none while holding" is an
//      actual count, not a guess -- true for transitions the test spaces
//      out across pumps, as every case below does; pendingTransportEdge_ is
//      a single-slot coalescing atomic (FroggersPluginProcessor.hpp's own
//      comment on it), so transitions arriving faster than this class's
//      ~33ms (30Hz) pump rate collapse to the latest one, by design, not
//      tested here.
//   2. Freeze semantics (task 6.4): engaged via the real production seam
//      (PortableSurface().DispatchAction(kFreeze), the same call
//      FroggersUiSurface's Freeze button itself makes), mirroring the
//      DIRECTION of app/FroggersAudioRoutingTests.cpp's T7.3(a)
//      (freeze_alone_holds_the_ring_above_an_audible_floor_and_stops_the_
//      transport) -- transport reads stopped, FreezeLatched() true, audio
//      stays audible past stopping_transport_silences_...'s own settle
//      window -- on the DEFAULT patch rather than that test's specially
//      tuned self-sustaining delay/reverb ring: FroggersAppCore forces
//      `setGate(gateOpen || FreezeLatched())` while latched (that file's own
//      T7.1 comment), so the envelope holds open regardless of patch
//      tuning -- the ring recipe exists to prove something ELSE (recirculating
//      energy persists), not to make the gate stay open, so this simpler
//      setup still exercises the actual property task 6.4 asks for.
//   3. Tempo-follow (task 6.3): host bpm -> DisplayTempoBpm()/
//      TempoExternallyClocked(), RequestTempoBpm() rejected while slaved,
//      both directions (works before engaging, rejected while slaved, works
//      again after disengaging).
//   4. Surface row (task 6.2): app/FroggersUiSurface.hpp's own
//      FroggersUiSurface, attached context-free (mirrors
//      FroggersSurfaceTests.cpp's BuildFroggersTreeAt/FindNodeById --
//      app/vst/ cannot include that .cpp, a standalone binary with its own
//      main(), so the same technique is reproduced here rather than
//      imported) -- default mode byte-identical children, plugin mode
//      thinned to Freeze + label. The REAL default-mode identity proof is
//      running app/FroggersSurfaceTests.cpp and the rest of the app suite
//      UNCHANGED (this group's report cites that CTest run); the check here
//      is a second, explicit, redundant confirmation of the same property
//      from this binary.
//   5. Bank-selection agreement after host automation (group 8, carry-
//      forward from the group 7 review): a host write into a non-visible
//      bank must leave ActiveBankIndex(), the rendered/selected bank tab,
//      and a page-scoped action (Reset Page) all agreeing on the SAME real
//      bank -- see that TEST_CASE's own header comment for the full trace
//      of the bug this proves fixed.
//   6. Plugin-mode surface tree end-to-end (task 8.2): the REAL processor's
//      surface (not a bare hand-built one) renders Play/Stop/Record absent,
//      Freeze + its "FREEZE" label present, and BPM as a display-only
//      StatusText while the host clock is engaged -- proving production
//      code actually calls SetPluginHostMode(true) (section 4's own test
//      never constructs a real processor, so it cannot see that call at
//      all), combined with 6.3's host-tempo-slaving.

#include "FroggersPluginProcessor.hpp"

#include "FroggersUiSurface.hpp"
#include "synth/PortableUI.hpp"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

// Same self-contained TEST_CASE/REQUIRE_TRUE idiom every app/*Tests.cpp file
// already defines locally (e.g. app/FroggersSurfaceTests.cpp:54-82) --
// mirrored here rather than shared via a new header, matching that
// per-file-self-contained convention.
struct TestCase {
    const char* name;
    void (*fn)();
};

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Register {
    Register(const char* name, void (*fn)()) {
        Registry().push_back({name, fn});
    }
};

#define TEST_CASE(name)                \
    void name();                       \
    Register reg_##name(#name, &name); \
    void name()

#define REQUIRE_TRUE(expr)                                                        \
    do {                                                                          \
        if (!(expr)) {                                                            \
            std::ostringstream oss;                                               \
            oss << __FILE__ << ":" << __LINE__ << " requirement failed: " #expr;  \
            throw std::runtime_error(oss.str());                                  \
        }                                                                         \
    } while (false)

// Same shape as FroggersVstSmokeTest.cpp's ScratchDataPaths(), parameterized
// by test name so each TEST_CASE's processor gets its own scratch root
// (several processors are constructed in this one binary; a shared root
// would let one test's patch state leak into another's).
synth::RuntimeDataPaths ScratchDataPaths(const char* testName) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-vst-host-tests" / testName;
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

// A minimal juce::AudioPlayHead test double: exactly the two PositionInfo
// fields tasks 6.1/6.3 read (isPlaying, bpm). getPosition()'s own JUCE doc
// comment restricts real callers to processBlock() (see
// FroggersPluginProcessor.cpp's own citation of that comment) -- this class
// makes no such promise itself, it is only ever driven from this test's own
// single thread, calling processBlock() directly, so that restriction is
// respected by construction.
class FakePlayHead final : public juce::AudioPlayHead {
public:
    void SetPlaying(bool playing) { info_.setIsPlaying(playing); }
    void SetBpm(double bpm) { info_.setBpm(juce::Optional<double>(bpm)); }
    void ClearBpm() { info_.setBpm(juce::Optional<double>()); }

    juce::Optional<PositionInfo> getPosition() const override { return info_; }

private:
    PositionInfo info_;
};

// -- 1. Transport edges (task 6.1) -------------------------------------------
// run -> stop -> run: exactly one message per transition, none while
// holding, counted at the bus (UiBusPendingCountForTest()), not inferred.
TEST_CASE(transport_edges_run_stop_run_produce_exactly_one_message_per_transition) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("transport_edges"));
    FakePlayHead playHead;
    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };

    // First observation establishes the baseline (group 6 brief: "on host
    // play-state TRANSITIONS only") -- must NOT itself produce a message.
    playHead.SetPlaying(false);
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);

    // Holding "not playing" across several blocks: still nothing pending.
    runBlock();
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);

    // -- RUN (Stopped -> Playing transition #1) ------------------------------
    playHead.SetPlaying(true);
    runBlock();  // detects the edge, buffers it (does not push).
    processor.PumpMessageThreadForTest();  // pushes exactly the one Start.
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 1);
    runBlock();  // drains+applies it.
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    REQUIRE_TRUE(processor.ApplicationForTest().TransportRunning());

    // Holding "playing" across several more blocks + pumps: no new message.
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    REQUIRE_TRUE(processor.ApplicationForTest().TransportRunning());

    // -- STOP (Playing -> Stopped transition) --------------------------------
    playHead.SetPlaying(false);
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 1);
    runBlock();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    REQUIRE_TRUE(!processor.ApplicationForTest().TransportRunning());

    // -- RUN again (Stopped -> Playing transition #2) ------------------------
    playHead.SetPlaying(true);
    runBlock();
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 1);  // exactly one, not accumulated from before.
    runBlock();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    REQUIRE_TRUE(processor.ApplicationForTest().TransportRunning());

    processor.releaseResources();
    processor.setPlayHead(nullptr);

    std::cout << "  [6.1] run->stop->run: exactly one bus message per transition, none while holding.\n";
}

// Absent playhead (standalone hosting) must never synthesize a transport
// message -- group 6 brief: "Handle absent playhead ... gracefully: no
// messages."
TEST_CASE(transport_absent_playhead_produces_no_messages) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("transport_absent_playhead"));
    // No setPlayHead() call at all -- getPlayHead() returns nullptr, exactly
    // the standalone-hosting case.
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    for (int i = 0; i < 8; ++i) {
        buffer.clear();
        processor.processBlock(buffer, midi);
    }
    processor.PumpMessageThreadForTest();
    REQUIRE_TRUE(processor.UiBusPendingCountForTest() == 0);
    REQUIRE_TRUE(!processor.ApplicationForTest().TransportRunning());

    processor.releaseResources();
    std::cout << "  [6.1] absent playhead: no messages across 8 blocks.\n";
}

// -- 2. Freeze semantics -----------------------------------------------------
// Mirrors the DIRECTION of app/FroggersAudioRoutingTests.cpp's T7.3(a) --
// see this file's own header comment for why the default patch already
// exercises the property under test (setGate(gateOpen || FreezeLatched())).
TEST_CASE(freeze_via_production_seam_holds_audio_and_reads_stopped_like_t7_3a) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("freeze_semantics"));
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlocksMeasuringPeak = [&](int blocks) {
        float peak = 0.0f;
        for (int i = 0; i < blocks; ++i) {
            buffer.clear();
            processor.processBlock(buffer, midi);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* samples = buffer.getReadPointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    peak = std::max(peak, std::fabs(samples[s]));
                }
            }
        }
        return peak;
    };

    // Real production Play seam (same one TestStartTransport() now routes
    // through) -- start the transport and let the default patch settle into
    // audible steady state, same ~1s budget FroggersVstSmokeTest.cpp uses.
    processor.TestStartTransport();
    const float settledPeak = runBlocksMeasuringPeak(188);
    // Same -60dBFS silence floor FroggersVstSmokeTest.cpp establishes (that
    // file's own kSilenceFloorLinear citation of
    // app/FroggersAudioRoutingTests.cpp's constant).
    constexpr float kSilenceFloorLinear = 1.0e-3f;
    REQUIRE_TRUE(settledPeak >= kSilenceFloorLinear);  // actually audible before trusting anything below.

    // -- Engage Freeze via the REAL production seam --------------------------
    // (PortableSurface().DispatchAction(kFreeze), the exact call the Freeze
    // button itself makes -- FroggersUiSurface.hpp's kFreeze branch: latch
    // engage + Stop push + SetDesiredTransportRunning(false), all in one
    // press, per T7.1.)
    processor.ApplicationForTest().PortableSurface().DispatchAction(
        synth::ui::Action::Named(synth_froggers::FroggersActions::kFreeze));
    // One block to drain+apply the Stop the Freeze press pushed.
    buffer.clear();
    processor.processBlock(buffer, midi);

    REQUIRE_TRUE(processor.ApplicationForTest().FreezeLatched());
    REQUIRE_TRUE(!processor.ApplicationForTest().TransportRunning());  // T7.3(a): transport reads stopped.

    // Past the settle window a plain (unlatched) Stop would have silenced
    // within: the gate is forced open by FreezeLatched(), so output must
    // STAY audible.
    const float heldPeak = runBlocksMeasuringPeak(94);  // ~0.5s, same window group 5's own pre-start check uses.
    REQUIRE_TRUE(heldPeak >= kSilenceFloorLinear);
    REQUIRE_TRUE(processor.ApplicationForTest().FreezeLatched());  // still latched -- nothing released it.
    REQUIRE_TRUE(!processor.ApplicationForTest().TransportRunning());  // still stopped.

    std::cout << "  [Freeze] settledPeak=" << settledPeak << " heldPeak(frozen)=" << heldPeak
              << " (>= silence floor " << kSilenceFloorLinear << ")\n";

    // Cleanup: release the latch and stop cleanly (does not restart
    // transport, T7.1 -- not asserted further here, out of this test's
    // scope).
    processor.releaseResources();
}

// -- 3. Tempo-follow (task 6.3) ----------------------------------------------
TEST_CASE(host_tempo_follows_and_suppresses_user_requests_while_slaved_both_directions) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("tempo_follow"));
    FakePlayHead playHead;
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };

    // -- Positive control, direction 1: unslaved, user tempo works ----------
    // No playhead attached yet -- hostTempoValid_ is false by construction,
    // so this is the same "standalone hosting" state as
    // transport_absent_playhead_produces_no_messages above.
    REQUIRE_TRUE(!processor.ApplicationForTest().TempoExternallyClocked());
    processor.ApplicationForTest().RequestTempoBpm(90.0);
    runBlock();  // ProcessFrame() drains the pending request.
    REQUIRE_TRUE(std::fabs(processor.ApplicationForTest().DisplayTempoBpm() - 90.0) < 0.5);

    // -- Engage: host reports a playhead with a tempo -----------------------
    processor.setPlayHead(&playHead);
    playHead.SetPlaying(false);  // 6.3 is tempo-only; transport state is irrelevant here.
    playHead.SetBpm(140.0);
    runBlock();                              // processBlock observes bpm=140, republishes it.
    processor.PumpMessageThreadForTest();    // engages receiveClock, pushes tick #1.
    runBlock();                              // drains+applies tick #1 (establishes hasLastClock only).
    REQUIRE_TRUE(processor.ApplicationForTest().TempoExternallyClocked());  // receiveClock is now true.

    // A second, correctly-spaced tick is needed before activeBpm_ actually
    // reflects 140 (MasterClock::HandleExternalClock's own interval
    // estimator, see FroggersPluginProcessor.cpp's timerCallback() comment)
    // -- sleep past one tick interval (60e6/(24*140) ~= 17.9ms) so the next
    // pump's catch-up loop pushes ticks spaced by REAL elapsed time, then
    // pump+drain again.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    processor.PumpMessageThreadForTest();
    runBlock();

    const double displayedBpm = processor.ApplicationForTest().DisplayTempoBpm();
    std::cout << "  [6.3] host bpm=140 -> DisplayTempoBpm()=" << displayedBpm << "\n";
    REQUIRE_TRUE(std::fabs(displayedBpm - 140.0) < 5.0);
    REQUIRE_TRUE(processor.ApplicationForTest().TempoExternallyClocked());

    // -- User request suppressed while slaved --------------------------------
    processor.ApplicationForTest().RequestTempoBpm(200.0);
    runBlock();
    REQUIRE_TRUE(std::fabs(processor.ApplicationForTest().DisplayTempoBpm() - 200.0) > 1.0);  // NOT accepted.
    REQUIRE_TRUE(processor.ApplicationForTest().TempoExternallyClocked());

    // -- Positive control, direction 2: disengage, user tempo works again ---
    // Task 6.3's teardown concern: the host stops reporting a usable tempo
    // (here, no bpm at all) -- mirrors "the DAW vanishes."
    playHead.ClearBpm();
    runBlock();                            // hostTempoValid_ -> false.
    processor.PumpMessageThreadForTest();  // disengage: RequestSyncConfiguration({}).
    runBlock();                            // ApplySyncConfig applies the disengage.
    REQUIRE_TRUE(!processor.ApplicationForTest().TempoExternallyClocked());

    processor.ApplicationForTest().RequestTempoBpm(90.0);
    runBlock();
    REQUIRE_TRUE(std::fabs(processor.ApplicationForTest().DisplayTempoBpm() - 90.0) < 0.5);  // works again.

    processor.releaseResources();
    processor.setPlayHead(nullptr);
    std::cout << "  [6.3] user request suppressed while slaved; both positive controls pass.\n";
}

// -- 3b. Teardown (review fix, Important 1 & 2) ------------------------------
// Shared "get to a known-engaged state" fixture for the two teardown tests
// below -- factored out (unlike the tempo-follow test above, which predates
// this fix and is left as-is) since both new tests need the identical
// engage sequence.
struct EngagedClockFixture {
    frogg3rs_vst::FroggersPluginProcessor processor;
    FakePlayHead playHead;
    juce::AudioBuffer<float> buffer{2, 256};
    juce::MidiBuffer midi;

    explicit EngagedClockFixture(const char* scratchName) : processor(ScratchDataPaths(scratchName)) {
        processor.setRateAndBufferSizeDetails(48000.0, 256);
        processor.prepareToPlay(48000.0, 256);
    }

    void RunBlock() {
        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    // Engages host-tempo slaving (task 6.3) and confirms it before
    // returning -- both teardown tests below start from this known state.
    void EngageAt(double bpm) {
        processor.setPlayHead(&playHead);
        playHead.SetPlaying(false);
        playHead.SetBpm(bpm);
        RunBlock();
        processor.PumpMessageThreadForTest();
        RunBlock();
        REQUIRE_TRUE(processor.ApplicationForTest().TempoExternallyClocked());
    }
};

// Review fix, Important 1 (data race): releaseResources() must be able to
// disengage the clock ON ITS OWN, immediately, without relying on
// processBlockCounter_'s staleness streak ever accumulating -- proving that
// requires a scenario where the streak provably never reaches
// kStaleProcessBlockPumpThreshold, i.e. exactly one processBlock() call
// after releaseResources() (needed only so ApplySyncConfig() on the audio
// thread can apply the disengage request the very next pump makes -- not
// itself part of the trigger under test).
TEST_CASE(release_resources_alone_disengages_the_clock_with_no_further_process_block) {
    EngagedClockFixture fixture("teardown_release_resources");

    fixture.EngageAt(120.0);
    REQUIRE_TRUE(fixture.processor.ApplicationForTest().TempoExternallyClocked());

    fixture.processor.releaseResources();  // sets releaseResourcesSeen_ only (Important 1's fix).
    fixture.processor.PumpMessageThreadForTest();  // consumes it, forces disengage on THIS one pump.
    fixture.RunBlock();  // lets ApplySyncConfig() apply the disengage -- verification only.

    REQUIRE_TRUE(!fixture.processor.ApplicationForTest().TempoExternallyClocked());
    fixture.processor.setPlayHead(nullptr);
    std::cout << "  [teardown] releaseResources() alone disengaged the clock on the very next pump "
                 "(no staleness streak needed).\n";
}

// Review fix, Important 2 (teardown gap): if the host simply stops calling
// processBlock() at all -- no releaseResources() either -- the clock must
// still disengage once REAL elapsed time (not pump count -- see the
// carry-forward comment on kStaleActivityWindowMicros,
// FroggersPluginProcessor.hpp) reaches kStaleActivityWindowMicros with the
// counter unmoved. A single settling pump first is needed for a robust test
// (not part of the scenario under test): EngageAt()'s own last
// processBlock() call happens AFTER its own single pump, so
// lastActivityMicros_ has not yet observed that last increment; one more
// pump here syncs it, so the test below starts its own timing from a clean
// baseline regardless of EngageAt()'s internal call pattern.
//
// Proves BOTH triggers independently, per the carry-forward's own
// instruction: (1) a POSITIVE CONTROL well before the ~1s window elapses --
// a handful of quick pumps must NOT disengage the clock, proving this is
// genuinely time-gated rather than a fixed pump count in a different shape
// -- then (2) the actual time-based trigger, reached only after sleeping
// past the real window.
TEST_CASE(clock_disengages_after_the_time_based_staleness_window_when_process_block_simply_stops) {
    EngagedClockFixture fixture("teardown_staleness_window");

    fixture.EngageAt(120.0);
    REQUIRE_TRUE(fixture.processor.ApplicationForTest().TempoExternallyClocked());
    fixture.processor.PumpMessageThreadForTest();  // settle: sync lastActivityMicros_, see comment above.

    // -- Positive control: several quick pumps, no processBlock() in
    // between, well UNDER the ~1s window -- must still read engaged.
    for (int i = 0; i < 5; ++i) {
        fixture.processor.PumpMessageThreadForTest();
    }
    REQUIRE_TRUE(fixture.processor.ApplicationForTest().TempoExternallyClocked());

    // The scenario under test: NO processBlock() call anywhere from here on
    // -- only the message thread's own pump keeps running, exactly like a
    // host that suspends/disables the track without a clean teardown call.
    // Sleep past the real kStaleActivityWindowMicros (~1s) window, with
    // margin, before pumping again.
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    fixture.processor.PumpMessageThreadForTest();
    fixture.RunBlock();  // lets ApplySyncConfig() apply the disengage -- verification only.

    REQUIRE_TRUE(!fixture.processor.ApplicationForTest().TempoExternallyClocked());
    fixture.processor.setPlayHead(nullptr);
    std::cout << "  [teardown] clock disengaged after the ~1s time-based staleness window elapsed with zero "
                 "intervening processBlock() calls (and did NOT disengage on a handful of quick pumps well "
                 "under that window).\n";
}

// -- 4. Surface row (task 6.2) -----------------------------------------------
// Context-free surface build, mirroring app/FroggersSurfaceTests.cpp's own
// BuildFroggersTreeAt/FindNodeById technique (that file's own lines 84-127,
// 309-318) -- reproduced here rather than included, since that file is a
// standalone binary with its own main() app/vst/ cannot link against.
const synth::ui::Node* FindNodeById(const synth::ui::NodeTree& tree, const std::string& id) {
    for (const synth::ui::Node& node : tree.nodes) {
        if (node.id.value == id) {
            return &node;
        }
    }
    return nullptr;
}

synth::ui::NodeTree BuildFroggersTree(bool pluginHostMode) {
    synth::RuntimeConfig config = synth_froggers::FroggersApp::Config();
    synth::AppContext context;
    context.config = &config;
    synth_froggers::FroggersUiSurface surface;
    surface.Attach(&context, nullptr);
    surface.SetPluginHostMode(pluginHostMode);
    return surface.BuildTree();
}

TEST_CASE(default_mode_transport_row_is_unchanged_play_stop_freeze_record) {
    const synth::ui::NodeTree tree = BuildFroggersTree(/*pluginHostMode=*/false);
    const synth::ui::Node* row = FindNodeById(tree, synth_froggers::FroggersNodeIds::kTransportRow);
    REQUIRE_TRUE(row != nullptr);
    REQUIRE_TRUE(row->children.size() == 4);
    REQUIRE_TRUE(row->children[0].value == synth_froggers::FroggersNodeIds::kPlay);
    REQUIRE_TRUE(row->children[1].value == synth_froggers::FroggersNodeIds::kStop);
    REQUIRE_TRUE(row->children[2].value == synth_froggers::FroggersNodeIds::kFreeze);
    REQUIRE_TRUE(row->children[3].value == synth_froggers::FroggersNodeIds::kRecord);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kFreezeLabel) == nullptr);
}

TEST_CASE(plugin_mode_transport_row_thins_to_freeze_and_label_only) {
    const synth::ui::NodeTree tree = BuildFroggersTree(/*pluginHostMode=*/true);
    const synth::ui::Node* row = FindNodeById(tree, synth_froggers::FroggersNodeIds::kTransportRow);
    REQUIRE_TRUE(row != nullptr);
    REQUIRE_TRUE(row->children.size() == 2);
    REQUIRE_TRUE(row->children[0].value == synth_froggers::FroggersNodeIds::kFreeze);
    REQUIRE_TRUE(row->children[1].value == synth_froggers::FroggersNodeIds::kFreezeLabel);

    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kPlay) == nullptr);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kStop) == nullptr);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kRecord) == nullptr);

    const synth::ui::Node* freeze = FindNodeById(tree, synth_froggers::FroggersNodeIds::kFreeze);
    REQUIRE_TRUE(freeze != nullptr);
    REQUIRE_TRUE(freeze->action.has_value() && freeze->action->name == synth_froggers::FroggersActions::kFreeze);

    const synth::ui::Node* label = FindNodeById(tree, synth_froggers::FroggersNodeIds::kFreezeLabel);
    REQUIRE_TRUE(label != nullptr);
    // Builder::Label() stores its text in Node::text, not Node::label
    // (PortableUIBuilders.hpp:184-190) -- Node::label is per-kind CAPTION
    // text for controls that render their own (buttons/sliders/etc, see
    // that struct's own comment); a Label node's displayed text is `text`.
    REQUIRE_TRUE(label->text == "FREEZE");
}

// -- 5. Stable-ID host parameter surface (task 7.2) --------------------------
// Every helper below reconstructs the SAME structural facts
// FroggersPluginProcessor::BuildHostParameterInventory() itself uses
// (kFroggersBankCount/kFroggersParamsPerBank/kFroggersCrispySlot/
// kFroggersCrunchySlot, FroggersParameters.hpp) -- not a copy of that
// method's own string-building, but the same INPUTS, so a test failure here
// means the plugin's actual construction order/IDs disagree with the
// model's own enumeration, not that two independent guesses disagree with
// each other.

// The expected ordered stable-ID list this plugin's construction order
// produces: bank0.slot0..13, bank0.crispy, bank1.slot0..13, bank1.crispy,
// ..., bank5.crispy, crunchy, freeze. Computed from the model's own
// constants, never a hardcoded flat list -- see this section's own header
// comment on why (a hardcoded 92-entry snapshot would duplicate
// BuildHostParameterInventory()'s derivation and silently drift if the
// model ever grows).
std::vector<std::string> ExpectedHostParamIdsInOrder() {
    std::vector<std::string> ids;
    for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount; ++bankIx) {
        for (std::size_t paramIx = 0; paramIx < synth_froggers::kFroggersParamsPerBank; ++paramIx) {
            ids.push_back("bank" + std::to_string(bankIx) + ".slot" + std::to_string(paramIx));
        }
        ids.push_back("bank" + std::to_string(bankIx) + ".crispy");
    }
    ids.push_back("crunchy");
    ids.push_back("freeze");
    return ids;
}

juce::AudioProcessorParameter* FindHostParamById(frogg3rs_vst::FroggersPluginProcessor& processor,
                                                 const std::string& id) {
    for (auto* p : processor.getParameters()) {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            if (ranged->getParameterID().toStdString() == id) {
                return p;
            }
        }
    }
    return nullptr;
}

// -- Count: exposed parameter count equals the model's own enumeration ------
TEST_CASE(host_parameter_count_matches_the_models_own_enumeration) {
    // Computed from FroggersParameterModel's own constants (kFroggersBankCount
    // * kFroggersParamsPerBank page parameters + kFroggersBankCount Crispy +
    // 1 shared Crunchy), never a hardcoded literal, plus 1 for Freeze (not a
    // ParameterManager parameter at all -- see BuildHostParameterInventory()'s
    // own comment).
    const std::size_t expectedModelParamCount =
        synth_froggers::kFroggersBankCount * synth_froggers::kFroggersParamsPerBank
        + synth_froggers::kFroggersBankCount + 1;
    const std::size_t expectedHostParamCount = expectedModelParamCount + 1;

    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("param_count"));
    REQUIRE_TRUE(static_cast<std::size_t>(processor.getParameters().size()) == expectedHostParamCount);
    REQUIRE_TRUE(ExpectedHostParamIdsInOrder().size() == expectedHostParamCount);

    std::cout << "  [7.2] exposed host parameter count = " << processor.getParameters().size()
              << " (model enumeration: " << expectedModelParamCount << " + 1 Freeze = " << expectedHostParamCount
              << ").\n";
}

// -- Stable IDs: deterministic across construction, structurally sound ------
TEST_CASE(host_parameter_ids_are_stable_structural_and_identical_across_construction) {
    const std::vector<std::string> expectedIds = ExpectedHostParamIdsInOrder();

    frogg3rs_vst::FroggersPluginProcessor processorA(ScratchDataPaths("param_ids_a"));
    frogg3rs_vst::FroggersPluginProcessor processorB(ScratchDataPaths("param_ids_b"));

    REQUIRE_TRUE(static_cast<std::size_t>(processorA.getParameters().size()) == expectedIds.size());
    REQUIRE_TRUE(static_cast<std::size_t>(processorB.getParameters().size()) == expectedIds.size());

    std::vector<std::string> seen;
    for (std::size_t i = 0; i < expectedIds.size(); ++i) {
        auto* rangedA = dynamic_cast<juce::RangedAudioParameter*>(processorA.getParameters()[static_cast<int>(i)]);
        auto* rangedB = dynamic_cast<juce::RangedAudioParameter*>(processorB.getParameters()[static_cast<int>(i)]);
        REQUIRE_TRUE(rangedA != nullptr && rangedB != nullptr);

        const std::string idA = rangedA->getParameterID().toStdString();
        const std::string idB = rangedB->getParameterID().toStdString();

        // Two independently constructed processors produce the IDENTICAL
        // ordered ID list (task 7.2's own "construct the processor twice"
        // requirement).
        REQUIRE_TRUE(idA == idB);
        // ...and match the structural expectation derived from the model's
        // own constants (this section's header comment).
        REQUIRE_TRUE(idA == expectedIds[i]);

        // Structural invariant: every page-parameter/Crispy ID is qualified
        // by its bank ("bankN." prefix); Crunchy/Freeze are not (exactly
        // one of each exists, globally).
        if (idA != "crunchy" && idA != "freeze") {
            REQUIRE_TRUE(idA.rfind("bank", 0) == 0);
        }
        seen.push_back(idA);
    }

    // No duplicates -- every stable ID is unique.
    std::vector<std::string> sortedSeen = seen;
    std::sort(sortedSeen.begin(), sortedSeen.end());
    REQUIRE_TRUE(std::adjacent_find(sortedSeen.begin(), sortedSeen.end()) == sortedSeen.end());

    std::cout << "  [7.2] " << expectedIds.size()
              << " stable IDs identical across two independent constructions, all unique.\n";
}

// -- Automation round-trip: host write -> core follows -> host readback -----
TEST_CASE(host_write_round_trips_through_the_core_parameter_authority) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("param_roundtrip"));
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };
    // ~1.5s of pumps+blocks: enough for (a) the message-thread pump to push
    // the write and (b) Parameter's own ~10Hz uiDisplayCenterAlpha one-pole
    // slew (ParameterModulation.hpp) to settle to within float epsilon.
    auto pumpAndSettle = [&] {
        for (int i = 0; i < 45; ++i) {
            processor.PumpMessageThreadForTest();
            for (int b = 0; b < 7; ++b) {
                runBlock();
            }
        }
    };

    // "bank5.slot3" = Reverb bank ("Pre-delay", FroggersBankLayouts()'s
    // Reverb row), an arbitrary non-Audio/non-bank-0 target chosen
    // specifically so this test also exercises the SelectParamBank
    // pre-step (PumpHostParameterBridge()'s own comment) -- bank 0 is
    // already selected by default (FroggersParameterModel::Init()), so a
    // bank-0 target alone would never prove that step actually runs.
    juce::AudioProcessorParameter* target = FindHostParamById(processor, "bank5.slot3");
    REQUIRE_TRUE(target != nullptr);

    constexpr float kHostTarget = 0.75f;
    target->setValue(kHostTarget);  // simulates the host applying automation.
    pumpAndSettle();

    // Assert at the REAL parameter authority (FroggersParameterModel::
    // PageParameter(5, 3), the literal Parameter object FroggersBankLayouts
    // ()[5].params[3] registers) -- not this bridge's own shadow/uiState
    // mirror.
    synth::Parameter& coreParam = processor.ApplicationForTest().Parameters().PageParameter(5, 3);
    constexpr float kTolerance = 0.01f;
    REQUIRE_TRUE(std::fabs(coreParam.UIDisplayCenter(0) - kHostTarget) < kTolerance);

    // Host readback matches too -- the core->host half of this bridge.
    REQUIRE_TRUE(std::fabs(target->getValue() - kHostTarget) < kTolerance);

    std::cout << "  [7.2] host wrote bank5.slot3=" << kHostTarget
              << " -> core UIDisplayCenter=" << coreParam.UIDisplayCenter(0) << " -> host readback=" << target->getValue()
              << ".\n";

    processor.releaseResources();
}

// -- Reverse direction: a core-side change is reflected to the host --------
TEST_CASE(core_side_randomize_is_reflected_to_every_host_parameter) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("param_reverse"));
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };
    auto pumpAndSettle = [&] {
        for (int i = 0; i < 45; ++i) {
            processor.PumpMessageThreadForTest();
            for (int b = 0; b < 7; ++b) {
                runBlock();
            }
        }
    };

    pumpAndSettle();  // let construction-time defaults fully mirror before snapshotting.

    std::vector<float> before(static_cast<std::size_t>(processor.getParameters().size()));
    for (int i = 0; i < processor.getParameters().size(); ++i) {
        before[static_cast<std::size_t>(i)] = processor.getParameters()[i]->getValue();
    }

    // Real production "the app itself changed the value" seam -- Randomize
    // All (FroggersAppCore::RequestRandomizeAll(), the SAME Request*/
    // pending*_ audio-thread bridge the real Randomize button drives via
    // FroggersUiSurface.hpp's kRandomizeAll branch), not a mock of the
    // bridge under test. ParameterManager's own random source is
    // fixed-seeded (`std::mt19937 randomEngine_{0x51EA5EEDu}`,
    // ParameterModulation.hpp), so this is deterministic, not flaky.
    processor.ApplicationForTest().RequestRandomizeAll();
    pumpAndSettle();

    int changedCount = 0;
    for (int i = 0; i < processor.getParameters().size(); ++i) {
        if (std::fabs(processor.getParameters()[i]->getValue() - before[static_cast<std::size_t>(i)]) > 0.01f) {
            ++changedCount;
        }
    }
    REQUIRE_TRUE(changedCount > 0);  // Randomize actually moved something.

    // Every host parameter mirrors the real parameter authority -- walked
    // in the SAME construction order this plugin itself uses, so each
    // index lines up with a specific, known core Parameter without needing
    // to parse IDs back apart.
    synth_froggers::FroggersParameterModel& model = processor.ApplicationForTest().Parameters();
    int hostIx = 0;
    constexpr float kTolerance = 0.01f;
    for (std::size_t bankIx = 0; bankIx < synth_froggers::kFroggersBankCount; ++bankIx) {
        for (std::size_t paramIx = 0; paramIx < synth_froggers::kFroggersParamsPerBank; ++paramIx) {
            const float hostValue = processor.getParameters()[hostIx++]->getValue();
            REQUIRE_TRUE(std::fabs(hostValue - model.PageParameter(bankIx, paramIx).UIDisplayCenter(0)) < kTolerance);
        }
        const float hostCrispy = processor.getParameters()[hostIx++]->getValue();
        REQUIRE_TRUE(std::fabs(hostCrispy - model.Crispy(bankIx).UIDisplayCenter(0)) < kTolerance);
    }
    const float hostCrunchy = processor.getParameters()[hostIx++]->getValue();
    REQUIRE_TRUE(std::fabs(hostCrunchy - model.Crunchy().UIDisplayCenter(0)) < kTolerance);
    // hostIx now points at Freeze -- RandomizeAll does not touch it
    // (transport-only latch), not asserted further here.

    std::cout << "  [7.2] RandomizeAll moved " << changedCount
              << "/" << processor.getParameters().size() << " host parameters; every one mirrors the core.\n";

    processor.releaseResources();
}

// -- No-feedback: bounded notifications, with a positive control -----------
struct CountingParamListener : juce::AudioProcessorParameter::Listener {
    int count = 0;
    void parameterValueChanged(int, float) override { ++count; }
    void parameterGestureChanged(int, bool) override {}
};

TEST_CASE(host_write_produces_a_bounded_number_of_notifications_not_an_endless_loop) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("param_no_feedback"));
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };
    auto pumpAndSettle = [&] {
        for (int i = 0; i < 45; ++i) {
            processor.PumpMessageThreadForTest();
            for (int b = 0; b < 7; ++b) {
                runBlock();
            }
        }
    };

    juce::AudioProcessorParameter* target = FindHostParamById(processor, "bank2.slot5");
    REQUIRE_TRUE(target != nullptr);

    // setValueNotifyingHost() is the ONLY thing that fires
    // parameterValueChanged() (juce_AudioProcessorParameter.cpp: plain
    // setValue() -- what the test below calls to simulate the host's own
    // write -- does NOT notify listeners), so this listener counts
    // PumpHostParameterBridge()'s own core->host notifications exclusively,
    // never the simulated host write itself.
    CountingParamListener listener;
    target->addListener(&listener);

    // -- Positive control, part 1: steady state produces zero notifications.
    pumpAndSettle();
    REQUIRE_TRUE(listener.count == 0);

    // -- Simulated host write --------------------------------------------
    target->setValue(0.42f);
    pumpAndSettle();  // Parameter's own slew converges; several notifies may fire while it does (not a bug).
    const int countAfterSettle = listener.count;
    // Positive control, part 2: the guard can fire at all -- something WAS
    // mirrored while the core converged toward the host's target, proving
    // this is not just an always-suppress no-op.
    REQUIRE_TRUE(countAfterSettle > 0);

    // -- Bounded, not endless: well past convergence, the count must be
    // FLAT -- no further notifies once the core has genuinely settled
    // (Parameter's uiDisplayCenterAlpha one-pole reaches a true, bit-exact
    // fixed point in finite time on floating-point hardware -- see
    // PumpHostParameterBridge()'s own comment).
    pumpAndSettle();
    REQUIRE_TRUE(listener.count == countAfterSettle);

    std::cout << "  [7.2] host write -> " << countAfterSettle
              << " notifications while settling, then flat (0 more) over a further ~1.5s of pumping.\n";

    target->removeListener(&listener);
    processor.releaseResources();
}

// -- 5. Carry-forward (group 7 review, addressed group 8): bank-selection
// agreement after host automation --------------------------------------
// The g7 bridge's host -> core direction (PumpHostParameterBridge(),
// FroggersPluginProcessor.cpp) pushes MessageIn::SelectParamBank directly
// onto engine_.UiBus() to keep the bank-select and the immediately-following
// ParamSetAbsolute write ordered within the SAME ProcessBlock() call (see
// that method's own header comment for why a plain RequestBankSelect()-only
// fix would land the switch one block late). That direct push alone never
// touched FroggersAppCore's own activeBankIx_/drillIn_ (RequestBankSelect()'s
// own pending-atomic, drained by ProcessFrame() -- FroggersAppCore.hpp:
// 557-568,622-664) -- so ActiveBankIndex()/ActiveDrillIn() (read by
// RandomizePage()/ResetPage(), FroggersAppCore.hpp:687-706) went stale after
// any cross-bank host write, even though the REAL selection authority
// (BankSlot::SelectedBank(), and therefore FroggersUiSurface::
// CurrentBankIndex() -- what the editor actually renders, task 8.1's
// render-host seam trace) was always correct. Fixed by ALSO calling
// RequestBankSelect() at the same call site (FroggersPluginProcessor.cpp's
// own comment there has the full trace); this test proves the fix from the
// OUTSIDE, driving only real production seams.
TEST_CASE(host_automation_in_a_non_visible_bank_keeps_active_bank_and_page_actions_in_sync) {
    frogg3rs_vst::FroggersPluginProcessor processor(ScratchDataPaths("bank_agreement"));
    processor.setRateAndBufferSizeDetails(48000.0, 256);
    processor.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto runBlock = [&] {
        buffer.clear();
        processor.processBlock(buffer, midi);
    };
    // Same "~1.5s settle" shape as the 7.2 round-trip tests above.
    auto pumpAndSettle = [&] {
        for (int i = 0; i < 45; ++i) {
            processor.PumpMessageThreadForTest();
            for (int b = 0; b < 7; ++b) {
                runBlock();
            }
        }
    };

    // FroggersParameterModel::Init() selects bank 0 by default
    // (FroggersPluginProcessor.hpp's own comment on
    // lastSelectedBankIxForHostWrites_).
    REQUIRE_TRUE(processor.ApplicationForTest().ActiveBankIndex() == 0);

    // Bank 3, slot 2 -- an arbitrary non-zero target, same "prove
    // SelectParamBank actually ran" reasoning as
    // host_write_round_trips_through_the_core_parameter_authority above.
    constexpr std::size_t kTargetBank = 3;
    constexpr std::size_t kTargetSlot = 2;
    juce::AudioProcessorParameter* target = FindHostParamById(processor, "bank3.slot2");
    REQUIRE_TRUE(target != nullptr);

    // Comfortably away from 0.0 -- see the Reset-Page assertion below for
    // why 0.0 (not each parameter's registered defaultValue) is the target
    // this test checks against; a value this far from it is an unambiguous
    // positive control (omni-rule 9.1) no matter which page parameter
    // "bank3.slot2" happens to be.
    constexpr float kHostTarget = 0.81f;

    target->setValue(kHostTarget);
    // ONE pump applies both the bridge's writes (SelectParamBank +
    // ParamSetAbsolute, drained by DrainMessageBus) and ProcessFrame's own
    // pendingBankSelect_ drain (RequestBankSelect(), this carry-forward's
    // fix) -- both inside the SAME upcoming ProcessBlock() call (see
    // PumpHostParameterBridge()'s own header comment on why both land in
    // one block); pumpAndSettle() also lets the slew fully converge before
    // the readback assertion below.
    pumpAndSettle();

    synth::Parameter& targetCoreParam = processor.ApplicationForTest().Parameters().PageParameter(kTargetBank, kTargetSlot);
    constexpr float kTolerance = 0.01f;
    REQUIRE_TRUE(std::fabs(targetCoreParam.UIDisplayCenter(0) - kHostTarget) < kTolerance);  // the write landed.

    // -- The carry-forward's own claim: activeBankIx_ (and therefore
    // drillIn_, reconstructed alongside it -- FroggersAppCore.hpp:622-641)
    // now agrees with the real selection, not stuck at the pre-automation
    // default.
    REQUIRE_TRUE(processor.ApplicationForTest().ActiveBankIndex() == kTargetBank);

    // -- The REAL authority (BankSlot::SelectedBank(), published via
    // ParameterManager::PopulateUIState to uiState->banks[].selected --
    // Sheaf ParameterModulation.cpp:3716-3733) agrees too, and this is
    // SPECIFICALLY what the editor renders: FroggersUiSurface::
    // CurrentBankIndex() (app/FroggersUiSurface.hpp:1872-1882) reads this
    // live state directly, never ActiveBankIndex() -- see task 8.1's report
    // for the full render-host seam trace. Read via the SAME BuildTree()
    // call the editor's PortableComponent makes every refresh.
    const synth::ui::NodeTree tree = processor.ApplicationForTest().PortableSurface().BuildTree();
    const synth::ui::Node* targetBankTab =
        FindNodeById(tree, synth_froggers::FroggersNodeIds::BankButton(kTargetBank));
    REQUIRE_TRUE(targetBankTab != nullptr);
    REQUIRE_TRUE(targetBankTab->selected);
    const synth::ui::Node* startingBankTab = FindNodeById(tree, synth_froggers::FroggersNodeIds::BankButton(0));
    REQUIRE_TRUE(startingBankTab != nullptr);
    REQUIRE_TRUE(!startingBankTab->selected);  // not still showing the stale pre-automation bank.

    // -- Page-scoped action (Reset Page) targets the SAME bank -----------
    // RandomizePage/ResetPage both act on *drillIn_ (FroggersAppCore.hpp:
    // 691-694/705-706), which the carry-forward's fix keeps bound to the
    // REAL active bank. Reset (not Randomize) gives an unambiguous,
    // deterministic observable: FroggersModulation.hpp's
    // ResetParameterValueAndDepths (ResetPage's own per-parameter helper,
    // level 0) calls `parameter.HandleSetAbsolute(pole, 0.0f)` for BOTH
    // scene poles -- a HARDCODED normalized 0.0, not each parameter's own
    // registered defaultValue (that field only seeds the value at
    // construction/Randomize time, per FroggersParameters.hpp; Reset does
    // not read it at all) -- so the correct post-Reset expectation is
    // exactly 0.0, verified directly against the same in-domain constant
    // ResetParameterValueAndDepths itself writes, not re-derived from a
    // different source that could silently drift from it.
    constexpr float kResetPageTarget = 0.0f;
    processor.ApplicationForTest().RequestResetPage();
    pumpAndSettle();

    REQUIRE_TRUE(std::fabs(targetCoreParam.UIDisplayCenter(0) - kResetPageTarget) < kTolerance);
    REQUIRE_TRUE(std::fabs(target->getValue() - kResetPageTarget) < kTolerance);  // host readback agrees too.

    std::cout << "  [carry-forward] host automation in bank " << kTargetBank << " slot " << kTargetSlot
              << ": ActiveBankIndex()=" << processor.ApplicationForTest().ActiveBankIndex()
              << ", rendered-selected bank tab agrees, and RequestResetPage() reset THIS bank's parameter (-> "
              << targetCoreParam.UIDisplayCenter(0)
              << ") rather than leaving it at the host-automated " << kHostTarget << ".\n";

    processor.releaseResources();
}

// -- 6. Plugin-mode surface tree, end-to-end through the REAL processor
// (task 8.2) --------------------------------------------------------------
// The existing plugin_mode_transport_row_thins_to_freeze_and_label_only
// test above already proves FroggersUiSurface's OWN branching logic is
// correct GIVEN pluginHostMode_ == true, using a bare, hand-constructed
// surface THIS FILE sets the flag on directly (BuildFroggersTree()). What
// that test cannot prove -- because it never constructs a real
// FroggersPluginProcessor at all -- is that PRODUCTION actually calls
// SetPluginHostMode(true) anywhere (task 8.1's first production caller,
// FroggersPluginProcessor.cpp's constructor). This test drives the REAL
// processor and reads the REAL engine_.Application().PortableSurface() --
// the exact instance the editor's PortableComponent renders -- to close
// that gap, and combines it with an ENGAGED host clock (EngagedClockFixture,
// already defined above for the teardown tests) to additionally prove the
// BPM control's "display-only while slaved" rendering (governed entirely by
// TempoExternallyClocked(), AppendBpmControl() -- app/FroggersUiSurface.hpp:
// 1272-1300 -- independent of pluginHostMode_ itself) still works correctly
// for a plugin-hosted surface. Extends, not duplicates, g6's own coverage.
TEST_CASE(production_processor_surface_is_plugin_mode_with_bpm_display_only_while_host_tempo_engaged) {
    EngagedClockFixture fixture("plugin_mode_surface_tree");
    fixture.EngageAt(128.0);

    const synth::ui::NodeTree tree = fixture.processor.ApplicationForTest().PortableSurface().BuildTree();

    // -- Plugin-mode row shape: belt-and-suspenders confirmation of g6's own
    // branching-logic proof, now through the REAL production object graph.
    const synth::ui::Node* row = FindNodeById(tree, synth_froggers::FroggersNodeIds::kTransportRow);
    REQUIRE_TRUE(row != nullptr);
    REQUIRE_TRUE(row->children.size() == 2);
    REQUIRE_TRUE(row->children[0].value == synth_froggers::FroggersNodeIds::kFreeze);
    REQUIRE_TRUE(row->children[1].value == synth_froggers::FroggersNodeIds::kFreezeLabel);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kPlay) == nullptr);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kStop) == nullptr);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kRecord) == nullptr);
    const synth::ui::Node* freezeLabel = FindNodeById(tree, synth_froggers::FroggersNodeIds::kFreezeLabel);
    REQUIRE_TRUE(freezeLabel != nullptr && freezeLabel->text == "FREEZE");

    // -- BPM display-only while host-tempo-slaved (task 8.2's own explicit
    // requirement) -- AppendBpmControl() emits a StatusText (not a Slider)
    // and no adjacent kBpmLabel while TempoExternallyClocked() is true.
    const synth::ui::Node* bpm = FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpm);
    REQUIRE_TRUE(bpm != nullptr);
    REQUIRE_TRUE(bpm->kind == synth::ui::NodeKind::StatusText);
    REQUIRE_TRUE(FindNodeById(tree, synth_froggers::FroggersNodeIds::kBpmLabel) == nullptr);

    fixture.processor.releaseResources();
    fixture.processor.setPlayHead(nullptr);
    std::cout << "  [8.2] production processor's surface: plugin-mode transport row + BPM StatusText \""
              << bpm->text << "\" while host-tempo-slaved.\n";
}

}  // namespace

int main() {
    int failed = 0;
    for (const auto& test : Registry()) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << "\n";
        } catch (const std::exception& ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << "\n";
        }
    }
    return failed == 0 ? 0 : 1;
}
