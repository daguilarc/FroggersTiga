#pragma once

#include "manifest/FroggersV2AppManifest.hpp"
#include "SequencerState.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace froggers_v2
{
constexpr uint8_t kNumHostPages = 7;
constexpr uint8_t kNumRows = 10;
constexpr uint8_t kNumScenes = 3;
constexpr uint8_t kNumGestures = 2;
constexpr uint8_t kNumModSources =
    static_cast<uint8_t>(manifest::kPermanentModulationSources.size());
constexpr int kMenuModMidiCcA = 15;
constexpr int kMenuModMidiCcB = 16;
constexpr uint8_t kNoSelection = 255;

constexpr uint8_t kRandSeqScopeStep = 0;
constexpr uint8_t kRandSeqScopePattern = 1;
constexpr uint8_t kRandSeqScopeFullStep = 2;
// kRandSceneScopeAll == 0 matches MessageIn's default-constructed `page`
// field so a RandAll message pushed without an explicit scope (legacy
// call sites, and FroggersV2AppCoreFacade_test.cpp /
// GlobalControlParity_test.cpp, which construct bare RandAll messages)
// keeps randomizing scene endpoints/blend exactly as before scope support
// existed. Current-scene targeting is the opt-in, non-default value.
constexpr uint8_t kRandSceneScopeAll = 0;
constexpr uint8_t kRandSceneScopeCurrent = 1;
constexpr uint8_t kModDetailCellCount = manifest::kModDetailCellCount;
constexpr uint8_t kUiSlots = kModDetailCellCount;
// The parameter-detail grid enumerates every permanent modulation lane plus one
// dedicated target/Crispy cell; the manifest-owned lane count must leave room
// for that extra cell inside the fixed kUiSlots (4x4) grid.
static_assert(kNumModSources < kUiSlots,
              "mod detail grid requires kUiSlots > kNumModSources for the dedicated target cell");

struct MessageIn
{
    enum class Type : uint8_t
    {
        ParamIncDec,
        ParamPress,
        ModDrillIn,
        ShiftHeld,
        SceneSelect,
        SceneBlend,
        GestureSelect,
        GestureWeight,
        ModSourceAssign,
        SelectPage,
        RandAll,
        RandMods,
        RandPage,
        ResetSequencerStep,
        RandSequencerStep,
        RandSequencerMods,
        Clock,
    };

    Type type = Type::Clock;
    uint8_t page = 0;
    uint8_t slot = 0;
    uint8_t index = 0;
    float value = 0.0f;

    static MessageIn ParamTurn(uint8_t pageIn, uint8_t slotIn, float delta)
    {
        MessageIn m;
        m.type = Type::ParamIncDec;
        m.page = pageIn;
        m.slot = slotIn;
        m.value = delta;
        return m;
    }

    static MessageIn ModDrillIn(uint8_t pageIn, uint8_t slotIn)
    {
        MessageIn m;
        m.type = Type::ModDrillIn;
        m.page = pageIn;
        m.slot = slotIn;
        return m;
    }

    static MessageIn SequencerStepClock(uint8_t playheadStep)
    {
        MessageIn m;
        m.type = Type::Clock;
        m.slot = playheadStep;
        m.index = kNoSelection;
        return m;
    }
};

class MessageInBus
{
public:
    static constexpr size_t kCapacity = 256;

    bool push(const MessageIn& message);
    bool pop(MessageIn& message);
    void clear();

private:
    std::array<MessageIn, kCapacity> m_queue{};
    std::atomic<uint32_t> m_write{0};
    std::atomic<uint32_t> m_read{0};
};

struct FroggersV2UIState
{
    std::atomic<uint32_t> version{0};
    std::atomic<uint8_t> activePage{0};
    std::atomic<uint8_t> modViewTargetRow{kNoSelection};
    std::atomic<uint8_t> visibleCount{0};
    std::atomic<uint8_t> leftSceneOrdinal{0};
    std::atomic<uint8_t> rightSceneOrdinal{1};
    std::atomic<float> sceneBlend{0.5f};
    std::atomic<uint8_t> activeGesture{kNoSelection};
    std::array<std::atomic<float>, kUiSlots> sceneLeft{};
    std::array<std::atomic<float>, kUiSlots> sceneRight{};
    std::array<std::atomic<float>, kUiSlots> effective{};
    std::array<std::atomic<float>, kUiSlots> arcMin{};
    std::array<std::atomic<float>, kUiSlots> arcMax{};
    std::array<std::atomic<uint16_t>, kUiSlots> modulatorsMask{};
    std::array<std::atomic<uint8_t>, kUiSlots> gesturesMask{};
    std::atomic<float> crunchySceneLeft{0.0f};
    std::atomic<float> crunchySceneRight{0.0f};
    std::atomic<float> crunchyEffective{0.0f};
    std::atomic<float> crunchyArcMin{0.0f};
    std::atomic<float> crunchyArcMax{1.0f};
};

class FroggersV2ControlCore
{
public:
    // Single authority for randomization commands. UI components (GlobalStripV2
    // and, transitively, anything wired through DesktopV2HostCallbacks) MUST
    // route Rand All / Rand Mods through executeRandomization() below rather
    // than constructing and pushing MessageIn values themselves, so there is
    // exactly one place that resolves scope into control-core mutation.
    enum class RandomizationCommand : uint8_t
    {
        RandAll,
        RandMods,
    };

    struct EffectiveRow
    {
        float sceneLeft = 0.0f;
        float sceneRight = 0.0f;
        float effective = 0.0f;
        float arcMin = 0.0f;
        float arcMax = 1.0f;
        uint16_t modulatorsMask = 0;
        uint8_t gesturesMask = 0;
    };

    FroggersV2ControlCore();

    MessageInBus& bus()
    {
        return m_bus;
    }

    const FroggersV2UIState& uiState() const
    {
        return m_uiState;
    }

    uint8_t activePage() const
    {
        return m_activePage;
    }

    uint8_t visibleCount() const
    {
        return m_visibleCount;
    }

    uint8_t visibleRowForSlot(uint8_t slot) const;
    uint8_t visibleModIndexForSlot(uint8_t slot) const;
    bool visibleSlotIsTarget(uint8_t slot) const;
    void setExternalAudioAvailable(bool available);
    bool externalAudioAvailable() const;
    float globalCrunchy() const;
    void setGlobalCrunchy(float value);
    float sceneBlend() const;
    float gestureWeight(uint8_t lane) const;
    void setSceneBlend(float value);
    void setGestureWeight(uint8_t lane, float value);
    uint8_t assignedModSource(uint8_t page, uint8_t row) const;
    float assignedModDepth(uint8_t page, uint8_t row) const;
    // Packet 15.2a: ParamState collapsed to lane identity -- modDepth[i] IS
    // lane i's signed depth, so this indexes modDepth directly by lane. All
    // kNumModSources lanes are independently addressable/non-zero at once
    // (see ParamState below); assignedModSource/assignedModDepth above are
    // now single-value *representative* accessors (first non-zero lane) kept
    // for the host/sequencer single-route projection, not the live model.
    float laneDepth(uint8_t page, uint8_t row, uint8_t lane) const;
    void applyHostModRoute(uint8_t page, uint8_t row, uint8_t engineModIndex, float depth);
    void processBus();
    bool consumeContentMutated();
    void compute();
    void populateUiState();
    void setSequencerState(SequencerState* sequencer);
    void applySequencerSlotPayload(const SequencerSlotPayload& snapshot);
    void captureSequencerSlotPayload(SequencerSlotPayload& out) const;
    void captureFactorySlotPayload(SequencerSlotPayload& out) const;
    void randomizeFullSlotPayload(SequencerSlotPayload& out);
    void randomizeSceneSlotsInto(SequencerSlotPayload& snapshot);
    void setExternalMidiMod(uint8_t slot, float value);
    float externalMidiMod(uint8_t slot) const;
    EffectiveRow effectiveRow(uint8_t page, uint8_t row) const;

    // The sole randomization mutator entry point for UI callers. sceneScope is
    // consulted for RandAll (kRandSceneScopeAll / kRandSceneScopeCurrent);
    // stepScope is consulted for RandMods (kRandSeqScopeStep /
    // kRandSeqScopePattern). Unused scope arguments are ignored by the
    // relevant command.
    void executeRandomization(RandomizationCommand command, uint8_t sceneScope, uint8_t stepScope);

private:
    struct ParamState
    {
        float sceneCenter[kNumScenes]{0.0f, 0.0f, 0.0f};
        // Packet 15.2a: lane identity model. modDepth[i] is the signed depth
        // (-1..+1) of modulation source i; lane i's source IS i. "Lane on" =
        // modDepth[i] != 0. All kNumModSources lanes are independent and any
        // number may be non-zero at once -- this is the single authority for
        // a row's live modulation state (the parallel modSource[] identity
        // array this used to carry alongside it is retired).
        float modDepth[kNumModSources]{0.0f};
        float gestureDepth[kNumGestures]{0.0f, 0.0f};
    };

    struct VisibleSlot
    {
        bool isTarget = false;
        uint8_t row = 0;
        uint8_t modIndex = 0;
    };

    struct ModViewState
    {
        bool open = false;
        uint8_t targetRow = kNoSelection;
    };

    static float clamp01(float value);
    static float clampSigned(float value);
    static bool isAdsrPage(uint8_t page);
    static uint8_t crispyRowForPage(uint8_t page);

    void applyMessage(const MessageIn& message);
    void onParamTurn(uint8_t page, uint8_t slot, float delta);
    void onParamPress(uint8_t page, uint8_t slot);
    void onModDrillIn(uint8_t page, uint8_t slot);
    void onSceneSelect(uint8_t sceneOrdinal);
    void onModSourceAssign(uint8_t page, uint8_t row, uint8_t source);
    void onRandAll(uint8_t sceneScope);
    void onRandMods(uint8_t stepScope);
    void onRandPage(uint8_t page);
    void onResetSequencerStep(uint8_t step);
    void onRandSequencerStep(uint8_t step, uint8_t scope);
    void onRandSequencerMods(uint8_t scope);
    void randomizeModIntoSnapshot(SequencerSlotPayload& snapshot);
    void randomizeSceneSlotValues(float (&scenes)[kNumScenes]);
    void randomizeSceneSlotValues(std::array<float, kNumScenes>& scenes);
    void randomizeSceneSlotValues(ParamState& param, uint8_t sceneScope);
    void randomizeSceneSlotsInto(uint8_t page);
    void randomizeSceneSlotsInto(uint8_t page, uint8_t sceneScope);
    // Shared authority for the "live mod depth" randomization transform used
    // by both Rand All's mod portion and Rand Mods (4.4): one implementation,
    // two callers, instead of duplicating the per-row/per-source loop.
    void randomizeLiveModDepths(uint8_t page);
    void randomizeLiveModDepths();
    void captureModRoutesIntoSnapshot(SequencerSlotPayload& snapshot) const;
    void randomizeSceneEndpointsAndBlend();
    void zeroStepGestures(SequencerSlotPayload& snapshot);
    void resetCrunchy();
    uint8_t activeSceneOrdinal() const;
    void advanceRandState();
    float sourceValue(uint8_t source) const;
    uint8_t rowsForPage(uint8_t page) const;
    uint8_t slotToRow(uint8_t page, uint8_t slot) const;
    float blendedSceneCenter(const ParamState& state) const;
    void resetParameter(uint8_t page, uint8_t row);
    void seedSceneCentersFromDefaults();
    void rebuildVisibleSlots();
    EffectiveRow computeEffective(const ParamState& state) const;
    EffectiveRow slotViewEffective(const VisibleSlot& visible) const;
    void storeSlotState(uint8_t slot, const EffectiveRow& row);

    SequencerState* m_sequencer = nullptr;
    MessageInBus m_bus;
    FroggersV2UIState m_uiState;
    std::array<std::array<ParamState, kNumRows>, kNumHostPages> m_params{};
    std::array<float, kNumModSources> m_sourceValues{};
    std::array<float, 2> m_externalMidiMods{};
    std::array<float, kNumGestures> m_gestureWeights{};
    std::array<VisibleSlot, kUiSlots> m_visibleSlots{};
    ModViewState m_modView{};
    uint8_t m_activePage = 0;
    uint8_t m_visibleCount = 0;
    uint8_t m_sceneLeftOrdinal = 0;
    uint8_t m_sceneRightOrdinal = 1;
    uint8_t m_sceneSelectFlip = 0;
    uint8_t m_activeGestureLane = kNoSelection;
    float m_sceneBlend = 0.5f;
    ParamState m_crunchy{};
    std::atomic<bool> m_contentMutated{false};
    uint32_t m_randState = 0x12345678u;
    bool m_externalAudioAvailable = false;
};
} // namespace froggers_v2
