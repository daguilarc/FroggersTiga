#pragma once

// synth_froggers::FroggersParameterModel -- packet 4 of the froggers-sheaf-app
// change (openspec/changes/froggers-sheaf-app/tasks.md, section "4. Parameter
// model (mono, 16-slot banks)", tasks 4.1-4.6; design D4 (mono), D5a (bank
// layout / fixed Crispy+Crunchy slots / per-bank colour), D9a (encoder ring
// shows the *processed* value -- the per-sample ProcessSamplePhase1/2 loop
// this class drives, wired into FroggersApp::ProcessBlock in Froggers.hpp, is
// exactly what makes SceneCenter/blend interpolation reach that published
// ring state)).
//
// Packet 5 (tasks.md section "5. Fuegoization on the filter seam", tasks
// 5.1-5.6; design D6/D9/D9a) adds ApplyFuegoSeam() below, called from
// ProcessSample() between ParameterGroup::ProcessSamplePhase1() and
// ProcessSamplePhase2() -- the exact seam Braid uses for its own per-sample
// filtering (apps/braid-4/Braid4Core.hpp:457-459 ProcessParameterPhase1 ->
// FilterParameterCaches -> ProcessParameterPhase2, filtering implementation
// :569-627). This is the ONE fuego application point (task 5.2's "exactly
// one" requirement): see ApplyFuegoSeam()'s own comment.
//
// Scope (hard constraint from the operator brief -- "parameter/bank model
// only"):
//   * No modulation slate (packet 6). ParameterGroupConfig::numModulators is
//     set to 15 now, per task 4.1's literal config shape, but none of the 15
//     sources are SetModulationSource()'d here -- every slot's
//     ModulatorMetadata stays default {connected=false}, and
//     Modulators::UpdateModValues() / Parameter's route-processing are
//     documented no-ops for unconnected slots
//     (External/Sheaf/projects/synth/src/ParameterModulation.cpp:576-577),
//     so driving the sample loop below is safe with zero sources wired.
//   * No UI layout (packet 10). FroggersUiSurface in Froggers.hpp is
//     untouched; this header only builds the Sheaf-side parameter/bank
//     graph.
//
// One ParameterManager, one mono ParameterGroup (task 4.1); six banks sharing
// one BankSlot (task 4.4), each bank holding one v2 page's nine parameters at
// slots 0-8, a local Crispy at slot 14, and the single shared global Crunchy
// Parameter at slot 15 in every bank (task 4.2/4.6). See FroggersBankLayouts()
// below for the independently re-derived per-page parameter lists.
//
// NOT decided here (deliberately out of packet 4's scope, left at
// ParameterConfig's own defaults): per-parameter `defaultValue` (stays 0.0f)
// and `range` (stays RangeKind::Unipolar). Task 4.3 says "register ... with
// Froggers value scaling" -- read here as the *mechanism* (RangeKind +
// baseColor, both of which ARE set below), not each parameter's true
// numeric default/curve, which is a DSP-wiring concern for the packet that
// actually consumes these values (no DSP is wired to the parameter model
// yet; that starts around packet 7+). Flagged explicitly rather than
// invented silently -- see the packet report.

#include "dsp/Fuegoize.hpp"

#include "synth/AppContext.hpp"
#include "synth/Color.hpp"
#include "synth/ParameterModulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace synth_froggers {

// Which v2 page each bank corresponds to; also this bank's index into
// FroggersParameterModel's own bank array (and, since it is the only bank
// creator on this ParameterManager, ParameterManager::BankAt()'s index too).
enum class FroggersBankId : std::size_t {
    Audio = 0,
    Envelope = 1,
    Filter = 2,
    Drive = 3,
    Delay = 4,
    Reverb = 5,
};

inline constexpr std::size_t kFroggersBankCount = 6;
inline constexpr std::size_t kFroggersParamsPerBank = 9;  // bank slots 0-8
inline constexpr std::size_t kFroggersSlotsPerBank = 16;  // bank slots 0-15
inline constexpr std::size_t kFroggersCrispySlot = 14;    // design D5a/D6
inline constexpr std::size_t kFroggersCrunchySlot = 15;   // design D5a/D6

struct FroggersParamSpec {
    const char* name;
    const char* shortName;
    // Packet 4 deliberately left every parameter's true numeric default at
    // ParameterConfig's own 0.0f, flagging it as "a DSP-wiring concern for
    // the packet that actually consumes these values" (see this header's
    // own file-header note) -- packet 6a (design D15) is that packet.
    // Task 3.2's dsp::VcoAdsrState/MixOscVoices port treats the Envelope
    // bank's Sustain knob as a literal target level (0..1, clamped), not an
    // ExpMap-style knob with a nonzero floor like every other bank's
    // parameters -- so leaving it at the ordinary 0.0f default means the
    // ASR permanently holds at zero level regardless of gate state,
    // silencing every VCO. Since no packet in this change's roadmap (7-10)
    // introduces a note-on/gate gesture, and this packet's own brief
    // requires a freshly-started app to make sound with no user input
    // (D16), the three Sustain parameters below are given a nonzero
    // default (1.0f, full level) here; Attack/Release stay at the ordinary
    // 0.0f (their ExpMap floor, `VcoAdsrState::kMinTimeSeconds`, is already
    // a fast-but-nonzero time, so 0.0f there is an "instant on" default, not
    // a silencing one).
    float defaultValue = 0.0f;
};

struct FroggersBankLayout {
    FroggersBankId id;
    const char* name;
    synth::Color color;
    std::array<FroggersParamSpec, kFroggersParamsPerBank> params;
};

// Independently re-derived (task 4.2's "verify it against the header
// yourself, do not take it on faith") from
// desktop-v2/Source/V2DesktopPageDisplayNames.hpp's forHostPageRow
// (:125-164): grid rows < 7 come from kHostRowGrid[gridPage][row]
// (:148-155, gridPage = hostPage-1 for hostPage in [1,4]), rows 7-9 from
// kExpansionTailRowLabels[gridPage][row-7] (:156-159). kHostRowGrid's row
// arrays are 8 wide with a trailing index-7 "Crispy" that forHostPageRow
// never reads (documented dead data at V2DesktopPageDisplayNames.hpp's
// comment above kAudioRowLabels, :95-103) -- NOT copied wholesale here.
// Audio (hostPage 0) instead comes from kAudioRowLabels (:101-103, 7
// entries: rows 0-5 used, row 6 is Audio's own dead local Crispy) with the
// three Shape (VCO morph) controls added as ordinary on-grid slots per D5a
// (they are a separate global axis in v2, FroggersV2AppManifest.hpp:118-125,
// folded onto the grid here -- a different parameter from Drive's own
// "Shape" wavefolder control, D5a's note). Envelope (hostPage 5) comes from
// kEnvelopeRowLabels (:81-86, 10 entries: rows 0-8 used, row 9 is Envelope's
// dead local Crispy).
//
// Result matches design D5a's table exactly (independently confirmed, not
// copied from the design doc):
// Bank ORDER follows the signal path (task 3.2, operator 2026-07-28); it
// deliberately no longer matches the order D5a's table happened to list.
// The parameter CONTENT of each bank is unchanged from that table.
//   Audio    -- VCO1, VCO2, VCO3, Shape 1/2/3, Phase mod 1/2/3
//   Envelope -- Attack/Sustain/Release x VCO1, VCO2, VCO3
//   Filter   -- Comb offset, Peak freq, Peak gain, Peak Q, Comb delay,
//               Comb feedback, Comb LP, Comb/Peak, Scoop
//   Drive    -- Drive, Shape, SRR 1, SRR 2, XOR, Bit depth, Fuzz, Blend,
//               Phase
//   Delay    -- Delay time, Send, Feedback, Stereo width, Detune, Mod
//               depth, Wet mix, Color, Halo
//   Reverb   -- Wet/dry, Room size, Decay, Pre-delay, Damping, Stereo width,
//               Diffusion, Mod depth, Hold
inline const std::array<FroggersBankLayout, kFroggersBankCount>& FroggersBankLayouts() {
    static const std::array<FroggersBankLayout, kFroggersBankCount> layouts{{
        {FroggersBankId::Audio, "Audio", synth::Color::Red, {{
            // Task 6.2: the ordinary 0.0f default maps (via
            // Vco::PitchToPhaseIncrement, app/dsp/Vco.hpp:118-121,
            // f = 20 * 1000^knob) to 20 Hz -- inaudible on laptop speakers.
            // 0.2468/0.3471/0.4058 = ln(f/20)/ln(1000) for f = 110/220/330 Hz
            // (arithmetic given by the task spec, verified against
            // PitchToPhaseIncrement above). Sustain specs below set the
            // precedent for a nonzero `defaultValue` at this same call site.
            {"VCO1", "VCO1", 0.2468f}, {"VCO2", "VCO2", 0.3471f}, {"VCO3", "VCO3", 0.4058f},
            {"Shape 1", "Shp1"}, {"Shape 2", "Shp2"}, {"Shape 3", "Shp3"},
            {"Phase mod 1", "PM1"}, {"Phase mod 2", "PM2"}, {"Phase mod 3", "PM3"},
        }}},
        {FroggersBankId::Envelope, "Envelope", synth::Color::Green, {{
            {"Attack VCO1", "Atk1"}, {"Sustain VCO1", "Sus1", 1.0f}, {"Release VCO1", "Rel1"},
            {"Attack VCO2", "Atk2"}, {"Sustain VCO2", "Sus2", 1.0f}, {"Release VCO2", "Rel2"},
            {"Attack VCO3", "Atk3"}, {"Sustain VCO3", "Sus3", 1.0f}, {"Release VCO3", "Rel3"},
        }}},
        {FroggersBankId::Filter, "Filter", synth::Color::Blue, {{
            {"Comb offset", "CmbOff"}, {"Peak freq", "PkFreq"}, {"Peak gain", "PkGain"},
            {"Peak Q", "PkQ"}, {"Comb delay", "CmbDly"}, {"Comb feedback", "CmbFb"},
            {"Comb LP", "CmbLP"}, {"Comb/Peak", "Cmb/Pk"}, {"Scoop", "Scoop"},
        }}},
        {FroggersBankId::Drive, "Drive", synth::Color::Orange, {{
            {"Drive", "Drive"}, {"Shape", "Shape"}, {"SRR 1", "SRR1"},
            {"SRR 2", "SRR2"}, {"XOR", "XOR"}, {"Bit depth", "BitDp"},
            {"Fuzz", "Fuzz"}, {"Blend", "Blend"}, {"Phase", "Phase"},
        }}},
        {FroggersBankId::Delay, "Delay", synth::Color::Indigo, {{
            {"Delay time", "DlyTm"}, {"Send", "Send"}, {"Feedback", "Fb"},
            {"Stereo width", "Width"}, {"Detune", "Detune"}, {"Mod depth", "ModDp"},
            {"Wet mix", "WetMx"}, {"Color", "Color"}, {"Halo", "Halo"},
        }}},
        {FroggersBankId::Reverb, "Reverb", synth::Color::Cyan, {{
            {"Wet/dry", "Wet"}, {"Room size", "Room"}, {"Decay", "Decay"},
            {"Pre-delay", "PreDly"}, {"Damping", "Damp"}, {"Stereo width", "Width"},
            {"Diffusion", "Diff"}, {"Mod depth", "ModDp"}, {"Hold", "Hold"},
        }}},
    }};
    return layouts;
}

// D5a: Crunchy is a single fixed global-control colour in every bank (it
// cannot take on each bank's colour -- it is one shared Parameter with one
// baseColor). Distinct from all six bank colours above.
inline synth::Color FroggersCrunchyColor() { return synth::Color::Yellow; }

class FroggersParameterModel {
public:
    // task 4.1: ParameterGroupConfig{numVoices=1, numModulators=15,
    // numScenes=..., maxParameters=...} (ParameterModulation.hpp:195-198).
    static constexpr std::size_t kNumVoices = 1;
    static constexpr std::size_t kNumModulators = 15;  // D5's slate size; unfilled until packet 6.
    // Two scenes, matching apps/braid-4's convention (Braid4Core.hpp:125,137)
    // and the single scene-blend slider the surface design describes for
    // the chrome band (tasks.md 10.2/10.6) -- one scene *pair*.
    //
    // Task 3.5 (operator 2026-07-28, design E3d): the COUNT (two scenes, one
    // blend) still matches Braid 4, but the SCENE BUTTONS' behaviour no
    // longer does. Braid 4's own S1/S2 (apps/braid-4/Braid4UiModel.hpp:
    // 402-404) dispatch `SetLessSelectedScene`, which reassigns which stored
    // scene occupies the less-weighted endpoint and never touches the
    // blend -- at either blend extreme, pressing the button that already
    // owns the dominant endpoint does nothing audible. This app's own
    // buttons (FroggersUiSurface.hpp's AppendChromeBand/HandleAction) push
    // `MessageIn::SetSceneBlend(0.0f/1.0f)` directly instead, so pressing
    // either one is always audible. This is a deliberate product decision,
    // not an oversight -- do not "restore" Braid 4's convention here.
    static constexpr std::size_t kNumScenes = 2;
    // Sized for this packet's 61 top-level parameters (6 banks x 9 page
    // parameters + 6 per-bank Crispy + 1 shared Crunchy = 61, independently
    // matching design D14's "61 top-level parameters" count) plus a little
    // slack. Modulation-depth parameters (packet 6; up to 915 level-1 plus
    // more at level 2) are deliberately NOT sized for here -- that growth
    // rides ParameterGroup's own storage-batch request mechanism
    // (RequestParameterStorageBatch / ParameterMessageOutBus) once a later
    // packet wires the modulation slate; out of this packet's
    // "parameter/bank model only" scope.
    static constexpr std::size_t kMaxParameters = 64;

    // Task 9.3 (design D10): attach the Filter bank's bump/comb
    // transfer-function visualizers as underlays via
    // `ParameterConfig::visualizer` -- D9's "per-parameter underlay
    // visualizers via ParameterConfig::visualizer" mechanism (distinct from
    // task 8.3's `ModulatorMetadata::visualizer`, which is for modulation
    // SOURCES, not ordinary page parameters). Both default to nullptr so
    // every existing bare-ParameterManager test fixture (which does not care
    // about visualizer wiring) is unaffected. The two live DSP objects these
    // visualizers sample (FroggersApp's `filterChain_.peak`/`.comb`) are
    // owned by FroggersApp, a different object than this class -- so the
    // visualizer objects themselves are FroggersApp's own members,
    // constructed before FroggersApp::Init() calls this method, and simply
    // passed in here rather than owned by this class.
    void Init(synth::ParameterManager& manager, synth::ui::Visualizer* peakVisualizer = nullptr,
              synth::ui::Visualizer* combVisualizer = nullptr) {
        manager_ = &manager;
        group_ = &manager.CreateGroup({
            .numVoices = kNumVoices,
            .numModulators = kNumModulators,
            .numScenes = kNumScenes,
            .maxParameters = kMaxParameters,
        });

        // task 4.2/4.6: Crunchy is ONE Parameter, created before any bank so
        // the identical pointer can be registered into all six banks below.
        const synth::ParameterId crunchyId = manager.RegisterParameter(*group_, synth::ParameterConfig{
            .name = "Crunchy",
            .shortName = "Crnchy",
            .baseColor = FroggersCrunchyColor(),
        });
        crunchy_ = &manager.ParameterById(crunchyId);

        // task 4.4: one shared BankSlot, 16 physical encoders (slots 0-15),
        // matching Braid4Core's wiring order (create slot -> add physical
        // encoders before any Bank::RegisterParameters call, since that call
        // requires an associated slot with a full physical layout already
        // present -- src/ParameterModulation.cpp:2559-2566 in
        // External/Sheaf).
        slot_ = &manager.CreateBankSlot();
        for (synth::PhysicalEncoderId encoderId = 0; encoderId < kFroggersSlotsPerBank; ++encoderId) {
            slot_->AddPhysicalEncoder(encoderId);
        }

        const auto& layouts = FroggersBankLayouts();
        for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
            const FroggersBankLayout& layout = layouts[bankIx];

            synth::Bank& bank = manager.CreateBank();
            bank.SetBankColor(layout.color);
            // BankSlot::SelectBank associates a bank with this slot the
            // first time it is selected (Bank::AssociateSlot, idempotent for
            // repeated association with the same slot -- it throws only on
            // a *different* slot, src/ParameterModulation.cpp:2776-2781)
            // without permanently making it the *active* bank -- the final
            // SelectBank call after this loop sets the real default.
            slot_->SelectBank(&bank);

            // task 4.2/4.3: nine page parameters at offset 0, this bank's
            // colour on every one of them (D5a/D9a colour mechanism --
            // Bank::BankColor() itself renders nothing; encoder colour reads
            // only ParameterConfig::baseColor).
            //
            // DISCOVERED STRUCTURAL FACT (flagged in the packet report, not
            // silently worked around): ParameterManager::RegisterParameter
            // enforces GLOBAL name uniqueness across the whole manager
            // (`parameterNames_`, src/ParameterModulation.cpp:3069-3071 in
            // External/Sheaf) -- a stricter check than Bank::RegisterParameters's
            // own per-call-only duplicate check (task 4.2's citation,
            // :2572-2578). D5a's per-page labels are page-LOCAL in the
            // original product and genuinely repeat across pages ("Stereo
            // width" is both a Reverb and a Delay row; "Mod depth" is both a
            // Reverb and a Delay row) -- confirmed by the independent
            // re-derivation above, not an invented collision. A verbatim
            // Parameter::Name() per spec.name would throw "duplicate
            // parameter name" the second page registers "Stereo width" or
            // "Mod depth". Resolution: qualify the internal, global-namespace
            // Name() with the bank name ("Reverb Stereo width" / "Delay
            // Stereo width"), while leaving ShortName() -- the field the
            // encoder grid actually renders (EncoderDraw.hpp:322) -- as the
            // authentic, page-local, possibly-repeated label the original
            // product uses. This changes no on-screen text and no
            // parameter's identity/semantics; it only disambiguates the
            // internal name Sheaf uses for its own bookkeeping (and, later,
            // patch JSON keys).
            std::array<synth::Parameter*, kFroggersParamsPerBank> pageParams{};
            for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
                const FroggersParamSpec& spec = layout.params[paramIx];
                const std::string qualifiedName = std::string(layout.name) + " " + spec.name;
                // Task 9.3: Filter bank paramIx 1-3 are "Peak freq/Peak
                // gain/Peak Q" (this bank's ResonantBump) and paramIx 4-6
                // are "Comb delay/Comb feedback/Comb LP" (this bank's Comb)
                // -- see FroggersBankLayouts()'s own Filter row above. Comb
                // offset (0, the PureDelay ahead of the comb -- a timing
                // control, not part of either curve's own shape),
                // Comb/Peak (7, a blend) and Scoop (8, a second
                // ResonantBump instance not in task 9's scope) deliberately
                // get no transfer-function underlay here.
                synth::ui::Visualizer* visualizer = nullptr;
                if (layout.id == FroggersBankId::Filter) {
                    if (paramIx >= 1 && paramIx <= 3) {
                        visualizer = peakVisualizer;
                    } else if (paramIx >= 4 && paramIx <= 6) {
                        visualizer = combVisualizer;
                    }
                }
                const synth::ParameterId id = manager.RegisterParameter(*group_, synth::ParameterConfig{
                    .name = qualifiedName,
                    .shortName = spec.shortName,
                    .defaultValue = spec.defaultValue,
                    .baseColor = layout.color,
                    .visualizer = visualizer,
                });
                pageParams[paramIx] = &manager.ParameterById(id);
            }
            bank.RegisterParameters(pageParams, /*offset=*/0);

            // task 4.2/4.3: local Crispy at slot 14, this bank's colour
            // (D5a: Crispy is per-bank and DOES take the bank colour, unlike
            // Crunchy). Named "<Bank> Crispy" for global uniqueness across
            // the ParameterManager's flat name space -- the six banks each
            // have their own Crispy Parameter object (unlike Crunchy).
            const std::string crispyName = std::string(layout.name) + " Crispy";
            const synth::ParameterId crispyId = manager.RegisterParameter(*group_, synth::ParameterConfig{
                .name = crispyName,
                .shortName = "Crispy",
                .baseColor = layout.color,
            });
            synth::Parameter* crispyParam = &manager.ParameterById(crispyId);
            std::array<synth::Parameter*, 1> crispyArr{crispyParam};
            bank.RegisterParameters(crispyArr, kFroggersCrispySlot);

            // task 4.2/4.6: the SAME Crunchy Parameter* at slot 15 in every
            // bank. Bank::RegisterParameters's duplicate-visible-name check
            // only looks within the span passed to a single call
            // (src/ParameterModulation.cpp:2572-2578 in External/Sheaf), so
            // one call per bank registering this shared pointer never
            // collides with itself, and there is no Parameter->Bank
            // back-pointer anywhere to object to the same Parameter
            // appearing in six banks (see task 4.6's validation).
            std::array<synth::Parameter*, 1> crunchyArr{crunchy_};
            bank.RegisterParameters(crunchyArr, kFroggersCrunchySlot);

            banks_[bankIx] = &bank;
            crispy_[bankIx] = crispyParam;
            for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
                pageParameters_[bankIx][paramIx] = pageParams[paramIx];
            }
        }

        // Default active bank (task 4.4's "bank select" needs a starting
        // selection; the surface control that actually drives bank
        // selection is packet 10).
        slot_->SelectBank(banks_[0]);

        // task 4.4: scenes wired, two endpoints matching kNumScenes above.
        // Blend defaults to 0.0 (pure left/scene-0), matching
        // ParameterManager::UIState's own default (ParameterModulation.hpp:769).
        manager.SetSceneEndpoints(0, 1);
    }

    // Drives every top-level parameter's per-sample scene-blend
    // interpolation and UI-display slew (task 4.4's "the change reaches the
    // published ring state"), plus (packet 5, task 5.2) the fuego seam
    // between the two group-wide phases -- mirroring Braid's
    // ProcessParameterPhase1() -> FilterParameterCaches() ->
    // ProcessParameterPhase2() (Braid4Core.hpp:457-459). Safe to call every
    // sample even before any of the 15 modulation sources exist (packet 6)
    // because UpdateModValues()/route processing are no-ops for unconnected
    // modulator slots.
    void ProcessSample(std::uint64_t sampleIndex) {
        group_->UpdateModValues();
        group_->ProcessSamplePhase1(sampleIndex);
        ApplyFuegoSeam();
        group_->ProcessSamplePhase2();
    }

    synth::ParameterGroup& Group() { return *group_; }
    const synth::ParameterGroup& Group() const { return *group_; }
    synth::BankSlot& Slot() { return *slot_; }
    synth::Bank& BankAt(std::size_t bankIx) { return *banks_.at(bankIx); }
    synth::Bank& BankAt(FroggersBankId id) { return BankAt(static_cast<std::size_t>(id)); }
    synth::Parameter& PageParameter(std::size_t bankIx, std::size_t paramIx) {
        return *pageParameters_.at(bankIx).at(paramIx);
    }
    synth::Parameter& PageParameter(FroggersBankId bankId, std::size_t paramIx) {
        return PageParameter(static_cast<std::size_t>(bankId), paramIx);
    }
    synth::Parameter& Crispy(std::size_t bankIx) { return *crispy_.at(bankIx); }
    synth::Parameter& Crispy(FroggersBankId bankId) { return Crispy(static_cast<std::size_t>(bankId)); }
    synth::Parameter& Crunchy() { return *crunchy_; }

private:
    // Packet 5, task 5.2/5.3/5.6 (design D6): the ONE fuego application
    // point -- called exactly once per sample from ProcessSample(), between
    // group_->ProcessSamplePhase1() (which just wrote each parameter's raw,
    // modulated value into its cached-knob slot via ProcessLitePhase1's
    // `currentKnobValues_[v] = GetRaw(v)`) and group_->ProcessSamplePhase2()
    // (which slews UIDisplayCenter toward whatever the cache holds now).
    // Mirrors Braid's FilterParameterCaches() (Braid4Core.hpp:569-627): read
    // Parameter::CachedKnobValue(), transform, write back with
    // Parameter::ReplaceCachedKnobValue() -- so the fuegoized value is what
    // both a future DSP consumer and the UI-display slew (D9a) inherit, with
    // no separate write path.
    //
    // Row identity (task 5.1, D6 resolved): each parameter's 0-based slot
    // index within its 16-slot bank -- page params are rows 0-8, Crispy is
    // row kFroggersCrispySlot (14) in every bank. Crunchy is the shared
    // global control (not itself keyed to a "row" here, task 5.6 below).
    //
    // Voice index is always 0 -- D4's mono model (kNumVoices == 1).
    void ApplyFuegoSeam() {
        constexpr std::size_t kVoiceIx = 0;

        // Task 5.6 (design D6's Crunchy-not-warped inference): global
        // Crunchy receives NO fuego stage at all. It is the source of the
        // warp, and warping Crunchy by Crunchy is self-referential; the
        // cascade (sim/V2FuegoStack.hpp:9-23) only defines the treatment of
        // ordinary parameters (both stages) and of a Crispy control (global
        // stage only, src/core/Page.hpp:203-207) -- it says nothing about
        // Crunchy itself now that D5a puts it on the grid at slot 15. This
        // is an INFERENCE from the v2 Crispy-row precedent, not a traced
        // fact, and awaits operator confirmation (task 5.6). Implemented as
        // specified: simply never call ReplaceCachedKnobValue() on
        // crunchy_ here, so its cached value stays exactly what
        // ProcessLitePhase1 already wrote (the raw, unwarped value).
        const float globalCrunchy = crunchy_->CachedKnobValue(kVoiceIx);

        for (std::size_t bankIx = 0; bankIx < kFroggersBankCount; ++bankIx) {
            synth::Parameter& crispy = *crispy_[bankIx];
            const float crispyPreFuego = crispy.CachedKnobValue(kVoiceIx);

            // Task 5.3: a Crispy control receives ONLY the global stage --
            // it is itself Crunchy-warped before use as the per-bank
            // cascade key below (sim/V2FuegoStack.hpp:9-23's ApplyGlobal
            // call on crispyKnobPreFuego; v2 wiring proof at
            // src/core/Page.hpp:203-207). This is also exactly the value a
            // DSP/UI consumer of the Crispy parameter itself would read.
            const float crispyAfterCrunchy = synth_froggers::dsp::FuegoStack::ApplyGlobal(
                crispyPreFuego, globalCrunchy, static_cast<uint8_t>(kFroggersCrispySlot));
            crispy.ReplaceCachedKnobValue(kVoiceIx, crispyAfterCrunchy);

            // Task 5.2: ordinary (page) parameters get both stages -- global
            // Crunchy then this bank's Crispy -- via the full musical-row
            // cascade (sim/V2FuegoStack.hpp:14-23). Passing crispyPreFuego
            // (not crispyAfterCrunchy) matches ApplyMusicalRow's own
            // signature: it re-derives the Crunchy-warped Crispy value
            // internally from the pre-fuego knob, identically to the
            // crispy.ReplaceCachedKnobValue() call just above.
            for (std::size_t paramIx = 0; paramIx < kFroggersParamsPerBank; ++paramIx) {
                synth::Parameter& param = *pageParameters_[bankIx][paramIx];
                const float raw = param.CachedKnobValue(kVoiceIx);
                const float fuegoized = synth_froggers::dsp::FuegoStack::ApplyMusicalRow(
                    raw, globalCrunchy, crispyPreFuego,
                    static_cast<uint8_t>(paramIx), static_cast<uint8_t>(kFroggersCrispySlot));
                param.ReplaceCachedKnobValue(kVoiceIx, fuegoized);
            }
        }
    }

    synth::ParameterManager* manager_ = nullptr;
    synth::ParameterGroup* group_ = nullptr;
    synth::BankSlot* slot_ = nullptr;
    synth::Parameter* crunchy_ = nullptr;
    std::array<synth::Bank*, kFroggersBankCount> banks_{};
    std::array<synth::Parameter*, kFroggersBankCount> crispy_{};
    std::array<std::array<synth::Parameter*, kFroggersParamsPerBank>, kFroggersBankCount> pageParameters_{};
};

}  // namespace synth_froggers
