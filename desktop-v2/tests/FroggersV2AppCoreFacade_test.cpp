#include "control/FroggersV2AppCoreFacade.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "HostParameterInventoryV2.hpp"
#include "SequencerState.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

using froggers_v2::FroggersV2AppCoreFacade;
using froggers_v2::FroggersV2AppCoreConfig;
using froggers_v2::FroggersV2ControlCore;
using froggers_v2::FroggersV2HostBridge;
using froggers_v2::FroggersV2UIState;
using froggers_v2::MessageIn;

namespace
{
constexpr float kEps = 1.0e-5f;
constexpr size_t kBlock = 512;

float peakAbs(const float* samples, size_t count)
{
    float peak = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        peak = std::max(peak, std::fabs(samples[i]));
    }
    return peak;
}

bool nearlyEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

void pushAndProcess(FroggersV2ControlCore& core, const MessageIn& message)
{
    core.bus().push(message);
    core.processBus();
}

void applyDeterministicScenario(FroggersV2ControlCore& core)
{
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 3.0f));
    pushAndProcess(core, MessageIn::ParamTurn(0, 1, -2.0f));

    MessageIn selectFilter;
    selectFilter.type = MessageIn::Type::SelectPage;
    selectFilter.page = 1;
    pushAndProcess(core, selectFilter);
    pushAndProcess(core, MessageIn::ParamTurn(1, 0, 1.5f));

    MessageIn blend;
    blend.type = MessageIn::Type::SceneBlend;
    blend.value = 0.25f;
    pushAndProcess(core, blend);
}

void applyDeterministicScenario(FroggersV2AppCoreFacade& facade)
{
    facade.ingestMessage(MessageIn::ParamTurn(0, 0, 3.0f));
    facade.ingestMessage(MessageIn::ParamTurn(0, 1, -2.0f));

    MessageIn selectFilter;
    selectFilter.type = MessageIn::Type::SelectPage;
    selectFilter.page = 1;
    facade.ingestMessage(selectFilter);
    facade.ingestMessage(MessageIn::ParamTurn(1, 0, 1.5f));

    MessageIn blend;
    blend.type = MessageIn::Type::SceneBlend;
    blend.value = 0.25f;
    facade.ingestMessage(blend);
}

bool uiStateEquivalent(const FroggersV2UIState& reference, const FroggersV2UIState& candidate)
{
    if (reference.activePage.load(std::memory_order_acquire)
        != candidate.activePage.load(std::memory_order_acquire))
    {
        return false;
    }
    if (reference.modViewTargetRow.load(std::memory_order_acquire)
        != candidate.modViewTargetRow.load(std::memory_order_acquire))
    {
        return false;
    }
    if (reference.visibleCount.load(std::memory_order_acquire)
        != candidate.visibleCount.load(std::memory_order_acquire))
    {
        return false;
    }
    if (reference.leftSceneOrdinal.load(std::memory_order_acquire)
        != candidate.leftSceneOrdinal.load(std::memory_order_acquire))
    {
        return false;
    }
    if (reference.rightSceneOrdinal.load(std::memory_order_acquire)
        != candidate.rightSceneOrdinal.load(std::memory_order_acquire))
    {
        return false;
    }
    if (!nearlyEqual(reference.sceneBlend.load(std::memory_order_acquire),
                     candidate.sceneBlend.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    if (reference.activeGesture.load(std::memory_order_acquire)
        != candidate.activeGesture.load(std::memory_order_acquire))
    {
        return false;
    }

    const uint8_t visible = reference.visibleCount.load(std::memory_order_acquire);
    for (uint8_t slot = 0; slot < visible && slot < froggers_v2::kUiSlots; ++slot)
    {
        if (!nearlyEqual(reference.sceneLeft[slot].load(std::memory_order_acquire),
                         candidate.sceneLeft[slot].load(std::memory_order_acquire),
                         1.0e-4f))
        {
            return false;
        }
        if (!nearlyEqual(reference.sceneRight[slot].load(std::memory_order_acquire),
                         candidate.sceneRight[slot].load(std::memory_order_acquire),
                         1.0e-4f))
        {
            return false;
        }
        if (!nearlyEqual(reference.effective[slot].load(std::memory_order_acquire),
                         candidate.effective[slot].load(std::memory_order_acquire),
                         1.0e-4f))
        {
            return false;
        }
        if (!nearlyEqual(reference.arcMin[slot].load(std::memory_order_acquire),
                         candidate.arcMin[slot].load(std::memory_order_acquire),
                         1.0e-4f))
        {
            return false;
        }
        if (!nearlyEqual(reference.arcMax[slot].load(std::memory_order_acquire),
                         candidate.arcMax[slot].load(std::memory_order_acquire),
                         1.0e-4f))
        {
            return false;
        }
        if (reference.modulatorsMask[slot].load(std::memory_order_acquire)
            != candidate.modulatorsMask[slot].load(std::memory_order_acquire))
        {
            return false;
        }
        if (reference.gesturesMask[slot].load(std::memory_order_acquire)
            != candidate.gesturesMask[slot].load(std::memory_order_acquire))
        {
            return false;
        }
    }

    if (!nearlyEqual(reference.crunchySceneLeft.load(std::memory_order_acquire),
                     candidate.crunchySceneLeft.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    if (!nearlyEqual(reference.crunchySceneRight.load(std::memory_order_acquire),
                     candidate.crunchySceneRight.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    if (!nearlyEqual(reference.crunchyEffective.load(std::memory_order_acquire),
                     candidate.crunchyEffective.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    if (!nearlyEqual(reference.crunchyArcMin.load(std::memory_order_acquire),
                     candidate.crunchyArcMin.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    if (!nearlyEqual(reference.crunchyArcMax.load(std::memory_order_acquire),
                     candidate.crunchyArcMax.load(std::memory_order_acquire),
                     1.0e-4f))
    {
        return false;
    }
    return true;
}

bool test_facade_config_defaults()
{
    FroggersV2AppCoreConfig config;
    if (config.pluginHosted)
    {
        std::printf("FAIL: default facade config expected standalone\n");
        return false;
    }
    if (!nearlyEqual(config.sampleRate, 44100.0f))
    {
        std::printf("FAIL: default facade sample rate expected 44100\n");
        return false;
    }
    if (config.maxBlockSize != 512)
    {
        std::printf("FAIL: default facade block size expected 512\n");
        return false;
    }
    if (config.defaultPage != 0)
    {
        std::printf("FAIL: default facade page expected Audio/VCO page 0\n");
        return false;
    }
    return true;
}

bool test_audio_equivalence()
{
    DesktopHostIO referenceHost;
    DelayState referenceDelay;
    referenceHost.setDelayState(&referenceDelay);
    referenceHost.m_hostKind = SimHostKind::VstV2;
    referenceHost.Init();
    referenceHost.SetSampleRate(44100.0f);

    FroggersV2ControlCore referenceCore;
    SequencerState referenceSequencer;
    referenceCore.setSequencerState(&referenceSequencer);
    FroggersV2HostBridge referenceBridge(referenceCore, referenceHost);

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = 0;
    pushAndProcess(referenceCore, selectPage);
    referenceBridge.syncToHost();

    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine facadeAudio(true);
    FroggersV2AppCoreFacade facade(facadeAudio, config);
    facade.initialize();

    applyDeterministicScenario(referenceCore);
    applyDeterministicScenario(facade.controlCore());
    referenceBridge.syncToHost();
    facade.hostBridge().syncToHost();

    for (uint8_t row = 0; row < 3; ++row)
    {
        if (!nearlyEqual(referenceHost.GetPageParam(0, row),
                         facade.host().GetPageParam(0, row),
                         2.0e-3f))
        {
            std::printf("FAIL: facade host param mismatch on audio row %u\n", static_cast<unsigned>(row));
            return false;
        }
    }

    float referenceOut[kBlock] = {};
    float facadeOut[kBlock] = {};
    float silenceIn[kBlock] = {};

    constexpr int kBlocks = 64;
    for (int block = 0; block < kBlocks; ++block)
    {
        referenceHost.ProcessBlock(silenceIn, referenceOut, kBlock);
        facade.host().ProcessBlock(silenceIn, facadeOut, kBlock);
    }

    const float referencePeak = peakAbs(referenceOut, kBlock);
    const float facadePeak = peakAbs(facadeOut, kBlock);
    if (!nearlyEqual(referencePeak, facadePeak, 1.0e-3f))
    {
        std::printf(
            "FAIL: facade host audio peak mismatch reference=%f facade=%f\n",
            referencePeak,
            facadePeak);
        return false;
    }

    for (size_t i = 0; i < kBlock; ++i)
    {
        if (!nearlyEqual(referenceOut[i], facadeOut[i], 1.0e-3f))
        {
            std::printf("FAIL: facade host audio sample mismatch at %zu\n", i);
            return false;
        }
    }
    return true;
}

bool test_ui_state_equivalence()
{
    FroggersV2ControlCore referenceCore;
    applyDeterministicScenario(referenceCore);

    MessageIn adsrPage;
    adsrPage.type = MessageIn::Type::SelectPage;
    adsrPage.page = 2;
    pushAndProcess(referenceCore, adsrPage);
    pushAndProcess(referenceCore, MessageIn::ParamTurn(2, 0, 2.0f));

    MessageIn crunchyTurn = MessageIn::ParamTurn(froggers_v2::kNumHostPages, 0, 1.0f);
    pushAndProcess(referenceCore, crunchyTurn);

    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine facadeAudio(true);
    FroggersV2AppCoreFacade facade(facadeAudio, config);
    facade.initialize();
    applyDeterministicScenario(facade.controlCore());
    facade.ingestMessage(adsrPage);
    facade.ingestMessage(MessageIn::ParamTurn(2, 0, 2.0f));
    facade.ingestMessage(crunchyTurn);
    facade.publishUiFrame(false);

    if (!uiStateEquivalent(referenceCore.uiState(), facade.uiState()))
    {
        std::printf("FAIL: facade UI state diverged from direct control-core path\n");
        return false;
    }
    return true;
}

bool test_rand_all_immediate_command()
{
    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine audio(true);
    FroggersV2AppCoreFacade facade(audio, config);
    facade.initialize();

    MessageIn selectGesture;
    selectGesture.type = MessageIn::Type::GestureSelect;
    selectGesture.index = 1;
    facade.ingestMessage(selectGesture);

    const uint8_t leftBefore = facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = facade.uiState().sceneBlend.load(std::memory_order_acquire);

    MessageIn randAll;
    randAll.type = MessageIn::Type::RandAll;
    facade.ingestMessage(randAll);

    if (facade.uiState().activeGesture.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: RandAll left gesture selection active\n");
        return false;
    }

    const uint8_t leftAfter = facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightAfter = facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendAfter = facade.uiState().sceneBlend.load(std::memory_order_acquire);
    const bool metadataChanged = leftAfter != leftBefore || rightAfter != rightBefore
                                 || !nearlyEqual(blendAfter, blendBefore);
    if (!metadataChanged)
    {
        std::printf("FAIL: RandAll did not randomize scene endpoints or blend\n");
        return false;
    }
    if (nearlyEqual(facade.controlCore().globalCrunchy(), 0.0f))
    {
        std::printf("FAIL: RandAll did not randomize crunchy scene slots\n");
        return false;
    }
    return true;
}

bool test_rand_mod_reads_scope_and_preserves_scene_sliders()
{
    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine audio(true);
    FroggersV2AppCoreFacade facade(audio, config);
    facade.initialize();

    const uint8_t leftBefore = facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = facade.uiState().sceneBlend.load(std::memory_order_acquire);

    MessageIn randMods;
    randMods.type = MessageIn::Type::RandSequencerMods;
    randMods.page = froggers_v2::kRandSeqScopeStep;
    facade.ingestMessage(randMods);

    if (facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire) != leftBefore
        || facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire) != rightBefore
        || !nearlyEqual(facade.uiState().sceneBlend.load(std::memory_order_acquire), blendBefore))
    {
        std::printf("FAIL: RandSequencerMods changed scene slider selections\n");
        return false;
    }
    return true;
}

bool test_rand_page_preserves_scene_sliders()
{
    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine audio(true);
    FroggersV2AppCoreFacade facade(audio, config);
    facade.initialize();

    const uint8_t leftBefore = facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = facade.uiState().sceneBlend.load(std::memory_order_acquire);
    const float audioBefore = facade.controlCore().effectiveRow(0, 0).effective;

    MessageIn randPage;
    randPage.type = MessageIn::Type::RandPage;
    randPage.page = 1;
    facade.ingestMessage(randPage);

    if (facade.uiState().leftSceneOrdinal.load(std::memory_order_acquire) != leftBefore
        || facade.uiState().rightSceneOrdinal.load(std::memory_order_acquire) != rightBefore
        || !nearlyEqual(facade.uiState().sceneBlend.load(std::memory_order_acquire), blendBefore))
    {
        std::printf("FAIL: RandPage changed scene endpoints or blend\n");
        return false;
    }
    if (!nearlyEqual(facade.controlCore().effectiveRow(0, 0).effective, audioBefore, 2.0e-3f))
    {
        std::printf("FAIL: RandPage changed a different page\n");
        return false;
    }
    return true;
}

bool test_sequencer_lock_preservation()
{
    FroggersV2AppCoreConfig config;
    config.pluginHosted = true;
    AudioEngine audio(true);
    FroggersV2AppCoreFacade facade(audio, config);
    facade.initialize();

    SequencerState& seq = audio.getSequencer();
    seq.m_slots[0].written = true;
    seq.m_slots[0].locked = true;
    seq.m_slots[0].payload.sceneCenter[0][0][0] = 0.33f;

    const float lockedValue = seq.m_slots[0].payload.sceneCenter[0][0][0];

    MessageIn randAll;
    randAll.type = MessageIn::Type::RandAll;
    facade.ingestMessage(randAll);

    if (!seq.m_slots[0].written)
    {
        std::printf("FAIL: RandAll cleared written sequencer step\n");
        return false;
    }
    if (!seq.m_slots[0].locked)
    {
        std::printf("FAIL: RandAll cleared sequencer step lock\n");
        return false;
    }
    if (!nearlyEqual(seq.m_slots[0].payload.sceneCenter[0][0][0], lockedValue, 1.0e-4f))
    {
        std::printf("FAIL: RandAll mutated locked sequencer parameter value\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    if (!test_facade_config_defaults())
    {
        return 1;
    }
    if (!test_audio_equivalence())
    {
        return 1;
    }
    if (!test_ui_state_equivalence())
    {
        return 1;
    }
    if (!test_rand_all_immediate_command())
    {
        return 1;
    }
    if (!test_rand_mod_reads_scope_and_preserves_scene_sliders())
    {
        return 1;
    }
    if (!test_rand_page_preserves_scene_sliders())
    {
        return 1;
    }
    if (!test_sequencer_lock_preservation())
    {
        return 1;
    }
    std::printf("PASS: FroggersV2AppCoreFacade_test\n");
    return 0;
}
