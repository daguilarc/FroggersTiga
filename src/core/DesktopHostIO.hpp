#pragma once

#include "CvMidiBridge.hpp"
#include "CvPresence.hpp"
#include "DelayState.hpp"
#include "AudioPairArState.hpp"
#include "HostRandomize.hpp"
#include "SimModSource.hpp"
#include "V2EngineSetup.hpp"
#include "V2EnvelopeFollowerBank.hpp"
#include "V2ModTapBank.hpp"
#include "V2ParamDisplayNames.hpp"
#include "VcoAdsrState.hpp"
#include "SequencerState.hpp"

#include <cmath>
#include "FroggersEngine.hpp"
#include "Page.hpp"
#include "SchmidtTrigger.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

enum class HostMutationType : uint8_t
{
    NudgeMorph = 0,
    RandomizeMorphs = 1,
    RandomizePage = 2,
    RandomizePageMod = 3,
    RandomizeAllPages = 4,
    RandomizeAllMod = 5,
    SetPageModSource = 6,
    DelaySetModSource = 7,
    DelayRandomizeKnobs = 8,
    DelayRandomizeMod = 9,
    CycleMorph = 10,
    PairArSetModSource = 11
};

struct HostMutation
{
    HostMutationType type = HostMutationType::NudgeMorph;
    uint8_t page = 0;
    uint8_t row = 0;
    uint8_t modIndex = 255;
    uint8_t morphIndex = 0;
    float delta = 0.0f;
};

struct DesktopHostIO
{
    static constexpr int kMutationQueueSize = 32;

    PageManager m_pageManager;
    FroggersEngine m_engine;
    AudioPairArState m_pairAr;
    CvMidiBridge m_midiBridge;
    DelayState* m_delay = nullptr;
    SimHostKind m_hostKind = SimHostKind::Desktop;
    std::function<void(int)> m_buttonCallback;
    std::function<void(uint8_t, uint8_t, uint8_t)> m_midiOut;
    SchmidtTrigger m_gateTrigger{0.2f, 0.1f};
    float m_prevCv[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float m_cvPresence[4]{0.0f, 0.0f, 0.0f, 0.0f};
    bool m_gateHigh{false};
    std::array<HostMutation, kMutationQueueSize> m_mutationQueue{};
    std::atomic<int> m_mutationWrite{0};
    std::atomic<int> m_mutationRead{0};
    std::atomic<uint32_t> m_modRoutesVersion{0};

    std::function<void(uint8_t)> m_onBeforeClearModRoutes;
    std::function<void()> m_onSequencerStepAdvance;

    struct MarblesScopeAccum
    {
        float min = 0.0f;
        float max = 0.0f;
        bool initialized = false;
    };

    MarblesScopeAccum m_marblesScopeAccum[2]{};
    bool m_marblesScopeBlockReady = false;
    V2EnvelopeFollowerBank m_v2EfBank;
    V2ModTapBank m_v2ModTaps;
    SequencerState m_sequencer;
    VcoAdsrState m_vcoAdsr;
    float m_globalCrunchy = 0.0f;
    float m_sampleRate = 44100.0f;

    static void v2OscHook(float v1, float v2, float v3, void* ctx)
    {
        auto* self = static_cast<DesktopHostIO*>(ctx);
        self->m_v2EfBank.Process(v1, v2, v3, self->m_v2ModTaps);
    }

    static void v2MarblesHook(float marbles1, float marbles2, void* ctx)
    {
        (void)marbles1;
        (void)marbles2;
        auto* self = static_cast<DesktopHostIO*>(ctx);
        self->m_v2ModTaps.SyncMarblesFromModMgr(self->m_pageManager.m_modMgr);
    }

    void configureV2ModTaps()
    {
        m_v2EfBank.setSampleRate(m_sampleRate);
        FroggersEngine::V2ModTapHooks hooks{};
        hooks.processOsc = v2OscHook;
        hooks.syncMarbles = v2MarblesHook;
        hooks.ctx = this;
        m_engine.SetV2ModTapLayout(true, hooks);
    }

    void setDelayState(DelayState* delay)
    {
        m_delay = delay;
    }

    void enqueueMutation(HostMutation mutation)
    {
        const int w = m_mutationWrite.load(std::memory_order_relaxed);
        const int r = m_mutationRead.load(std::memory_order_acquire);
        if (w > r)
        {
            const HostMutation& last = m_mutationQueue[static_cast<size_t>((w - 1) % kMutationQueueSize)];
            if (last.type == mutation.type
                && (mutation.type == HostMutationType::RandomizeAllMod
                    || mutation.type == HostMutationType::RandomizeAllPages))
            {
                return;
            }
        }
        if (w - r >= kMutationQueueSize)
        {
            return;
        }
        m_mutationQueue[static_cast<size_t>(w % kMutationQueueSize)] = mutation;
        m_mutationWrite.store(w + 1, std::memory_order_release);
    }

    static bool isModRouteMutation(HostMutationType type)
    {
        switch (type)
        {
            case HostMutationType::RandomizePageMod:
            case HostMutationType::RandomizeAllMod:
            case HostMutationType::SetPageModSource:
            case HostMutationType::DelaySetModSource:
            case HostMutationType::DelayRandomizeMod:
            case HostMutationType::PairArSetModSource:
                return true;
            default:
                return false;
        }
    }

    uint32_t modRoutesVersion() const
    {
        return m_modRoutesVersion.load(std::memory_order_acquire);
    }

    void applyMutation(const HostMutation& mutation)
    {
        switch (mutation.type)
        {
            case HostMutationType::NudgeMorph:
                m_engine.NudgeVcoMorph(mutation.morphIndex, mutation.delta);
                break;
            case HostMutationType::RandomizeMorphs:
                m_engine.RandomizeVcoMorphs();
                break;
            case HostMutationType::RandomizePage:
                if (IsV2SimHostKind(m_hostKind))
                {
                    m_pageManager.RandomizePage(mutation.page);
                }
                else
                {
                    RandomizePageWithExtras(m_pageManager, mutation.page, m_pairAr);
                }
                break;
            case HostMutationType::RandomizePageMod:
                if (IsV2SimHostKind(m_hostKind))
                {
                    m_pageManager.RandomizePageModSim(mutation.page, m_midiBridge, m_hostKind);
                }
                else
                {
                    RandomizePageModWithExtras(
                        m_pageManager, mutation.page, m_pairAr, m_midiBridge, m_hostKind);
                }
                break;
            case HostMutationType::RandomizeAllPages:
                if (IsV2SimHostKind(m_hostKind))
                {
                    m_pageManager.RandomizeAllPagesIndependent();
                }
                else
                {
                    RandomizeAllPagesIndependentWithPairAr(m_pageManager, m_pairAr);
                }
                if (m_delay)
                {
                    m_delay->randomizeKnobs();
                }
                break;
            case HostMutationType::RandomizeAllMod:
                m_pageManager.RandomizeAllPagesModSim(m_midiBridge, m_hostKind);
                if (!IsV2SimHostKind(m_hostKind))
                {
                    m_pairAr.randomizeMod(m_midiBridge, m_hostKind);
                }
                if (m_delay)
                {
                    m_delay->randomizeMod(m_midiBridge, m_hostKind);
                }
                break;
            case HostMutationType::SetPageModSource:
                if (IsValidSimModAssignment(mutation.modIndex, m_hostKind)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge, m_hostKind)))
                {
                    m_pageManager.SetPageModSource(mutation.page, mutation.row, mutation.modIndex);
                }
                break;
            case HostMutationType::DelaySetModSource:
                if (m_delay && IsValidSimModAssignment(mutation.modIndex, m_hostKind)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge, m_hostKind)))
                {
                    m_delay->setModSource(mutation.row, mutation.modIndex);
                }
                break;
            case HostMutationType::DelayRandomizeKnobs:
                if (m_delay)
                {
                    m_delay->randomizeKnobs();
                }
                break;
            case HostMutationType::DelayRandomizeMod:
                if (m_delay)
                {
                    m_delay->randomizeMod(m_midiBridge, m_hostKind);
                }
                break;
            case HostMutationType::CycleMorph:
                m_engine.CycleVcoMorph(mutation.morphIndex);
                break;
            case HostMutationType::PairArSetModSource:
                if (!IsV2SimHostKind(m_hostKind)
                    && IsValidSimModAssignment(mutation.modIndex, m_hostKind)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge, m_hostKind)))
                {
                    m_pairAr.setModSource(mutation.row, mutation.modIndex);
                }
                break;
        }
        if (isModRouteMutation(mutation.type))
        {
            m_modRoutesVersion.fetch_add(1, std::memory_order_release);
        }
    }

    void DrainPendingMutations()
    {
        drainMutationQueue();
    }

    void drainMutationQueue()
    {
        while (true)
        {
            const int r = m_mutationRead.load(std::memory_order_relaxed);
            const int w = m_mutationWrite.load(std::memory_order_acquire);
            if (r >= w)
            {
                break;
            }
            applyMutation(m_mutationQueue[static_cast<size_t>(r % kMutationQueueSize)]);
            m_mutationRead.store(r + 1, std::memory_order_release);
        }
    }

    void Init()
    {
        m_engine.Config(&m_pageManager);
        if (UsesV2Fuego(m_hostKind))
        {
            V2EngineSetup::configure(m_pageManager, IsV2SimHostKind(m_hostKind));
            configureV2FuegoPages();
        }
        m_pairAr.init(44100.0f);
        m_pairAr.sanitizeModSources();
        m_pairAr.setV2FuegoConfig(&m_pageManager.m_pages[0], m_hostKind);
        if (IsV2SimHostKind(m_hostKind))
        {
            m_vcoAdsr.init(44100.0f);
            m_engine.SetAudioPairArState(nullptr);
            m_engine.SetVcoAdsrState(&m_vcoAdsr, &m_pageManager.m_pages[6]);
        }
        else
        {
            m_engine.SetAudioPairArState(&m_pairAr);
            m_engine.SetVcoAdsrState(nullptr, nullptr);
        }
        m_engine.SetUseV2FilterParallel(UsesV2Fuego(m_hostKind));
        if (m_delay)
        {
            m_delay->setUseV2Layout(UsesV2Fuego(m_hostKind));
            m_delay->setGlobalCrunchyPtr(&m_globalCrunchy);
            m_delay->sanitizeModSources();
        }
        m_engine.SetSimWaveMorph(true);
        m_engine.SetSimDedicatedPm3Knob(true);
        m_pageManager.SetAllParamsTracking();
        m_pageManager.SanitizeSimModAssignments();
        m_gateTrigger.Reset(m_gateHigh);
        if (IsV2SimHostKind(m_hostKind))
        {
            configureV2ModTaps();
        }
    }

    void SetSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate;
        m_engine.SetSampleRate(sampleRate);
        m_pairAr.setSampleRate(sampleRate);
        m_vcoAdsr.setSampleRate(sampleRate);
        if (IsV2SimHostKind(m_hostKind))
        {
            m_v2EfBank.setSampleRate(sampleRate);
        }
    }

    void SetPageKnob(uint8_t page, uint8_t position, float value)
    {
        if (page >= m_pageManager.m_numPages || position >= Parameter::x_numParameters)
        {
            return;
        }
        m_pageManager.m_knobPositions[position] = value;
        m_pageManager.m_pages[page].m_parameters[position].KnobUpdate(value);
    }

    float GetPageParam(uint8_t page, uint8_t position)
    {
        float value = m_pageManager.GetParam(page, position);
        if (!std::isfinite(value))
        {
            return 0.0f;
        }
        return value;
    }

    void SetGlobalCrunchy(float value)
    {
        m_globalCrunchy = std::min(std::max(value, 0.0f), 1.0f);
    }

    float GetGlobalCrunchy() const
    {
        return m_globalCrunchy;
    }

    const char* GetPageParamName(uint8_t page, uint8_t position)
    {
        return m_pageManager.m_pages[page].m_parameters[position].GetName();
    }

    void EnqueueRandomizePanel(uint8_t page)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::RandomizePage;
        mutation.page = page;
        enqueueMutation(mutation);
    }

    void EnqueueRandomizePanelMod(uint8_t page)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::RandomizePageMod;
        mutation.page = page;
        enqueueMutation(mutation);
    }

    void EnqueueRandomizeAllPages()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::RandomizeAllPages;
        enqueueMutation(mutation);
    }

    void EnqueueRandomizeAllMod()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::RandomizeAllMod;
        enqueueMutation(mutation);
    }

    void EnqueueDelayRandomizeKnobs()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::DelayRandomizeKnobs;
        enqueueMutation(mutation);
    }

    void EnqueueDelayRandomizeMod()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::DelayRandomizeMod;
        enqueueMutation(mutation);
    }

    void EnqueueSetPageModSource(uint8_t page, uint8_t row, uint8_t modIndex)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::SetPageModSource;
        mutation.page = page;
        mutation.row = row;
        mutation.modIndex = modIndex;
        enqueueMutation(mutation);
    }

    void EnqueueDelaySetModSource(uint8_t row, uint8_t modIndex)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::DelaySetModSource;
        mutation.row = row;
        mutation.modIndex = modIndex;
        enqueueMutation(mutation);
    }

    void EnqueuePairArSetModSource(uint8_t index, uint8_t modIndex)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::PairArSetModSource;
        mutation.row = index;
        mutation.modIndex = modIndex;
        enqueueMutation(mutation);
    }

    void SetAudioPairArKnob(uint8_t index, float value)
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return;
        }
        m_pairAr.setKnob(index, value);
    }

    float GetAudioPairArKnob(uint8_t index) const
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return 0.0f;
        }
        return m_pairAr.getKnob(index);
    }

    float GetAudioPairArEffective(uint8_t index) const
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return 0.0f;
        }
        return m_pairAr.getEffectiveKnob(index, &m_pageManager.m_modMgr);
    }

    uint8_t GetAudioPairArModSource(uint8_t index) const
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return 255;
        }
        return m_pairAr.getModSource(index);
    }

    float GetAudioPairArModDepth(uint8_t index) const
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return 0.0f;
        }
        return m_pairAr.getModDepth(index);
    }

    void SetAudioPairArModDepth(uint8_t index, float depth)
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return;
        }
        m_pairAr.setModDepth(index, depth);
    }

    void SetAudioPairArModSource(uint8_t index, uint8_t modIndex)
    {
        if (IsV2SimHostKind(m_hostKind))
        {
            return;
        }
        EnqueuePairArSetModSource(index, modIndex);
    }

    void SetVcoMorph(size_t index, float value)
    {
        m_engine.SetVcoMorph(index, value);
    }

    float GetVcoMorph(size_t index)
    {
        return m_engine.GetVcoMorph(index);
    }

    void RandomizeVcoMorphs()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::RandomizeMorphs;
        enqueueMutation(mutation);
    }

    void NudgeVco3Morph()
    {
        HostMutation mutation;
        mutation.type = HostMutationType::NudgeMorph;
        mutation.morphIndex = 2;
        mutation.delta = 0.1f;
        enqueueMutation(mutation);
    }

    void CycleVcoMorph(size_t index)
    {
        HostMutation mutation;
        mutation.type = HostMutationType::CycleMorph;
        mutation.morphIndex = static_cast<uint8_t>(index);
        enqueueMutation(mutation);
    }

    float GetVcoDisplayMorph(size_t index) const
    {
        return m_engine.GetVcoDisplayMorph(index);
    }

    bool consumeModScopeRange(uint8_t modIndex, float& outMin, float& outMax)
    {
        if (modIndex != 5 && modIndex != 6)
        {
            return false;
        }
        if (!m_marblesScopeBlockReady)
        {
            return false;
        }
        const size_t slot = static_cast<size_t>(modIndex - 5);
        MarblesScopeAccum& accum = m_marblesScopeAccum[slot];
        outMin = accum.min;
        outMax = accum.max;
        const float current = GetCvOut(modIndex);
        accum.min = current;
        accum.max = current;
        accum.initialized = true;
        m_marblesScopeBlockReady = false;
        return true;
    }

    void updateMarblesScopeAccum()
    {
        static constexpr uint8_t kMarblesMods[] = {5, 6};
        for (size_t i = 0; i < 2; ++i)
        {
            const float value = m_pageManager.m_modMgr.m_mods[kMarblesMods[i]];
            MarblesScopeAccum& accum = m_marblesScopeAccum[i];
            if (!accum.initialized)
            {
                accum.min = value;
                accum.max = value;
                accum.initialized = true;
                continue;
            }
            if (value < accum.min)
            {
                accum.min = value;
            }
            if (value > accum.max)
            {
                accum.max = value;
            }
        }
        m_marblesScopeBlockReady = true;
    }

    uint8_t GetPageModSource(uint8_t page, uint8_t position) const
    {
        return m_pageManager.GetPageModSource(page, position);
    }

    float GetPageModDepth(uint8_t page, uint8_t position) const
    {
        return m_pageManager.GetPageModDepth(page, position);
    }

    void SetPageModSource(uint8_t page, uint8_t position, uint8_t modIndex)
    {
        EnqueueSetPageModSource(page, position, modIndex);
    }

    void SetPageModDepth(uint8_t page, uint8_t position, float depth)
    {
        m_pageManager.SetPageModDepth(page, position, depth);
    }

    void SetMidiCcPairEnabled(uint8_t pairIndex, bool enabled)
    {
        if (pairIndex >= 2)
        {
            return;
        }
        m_midiBridge.setCcPairEnabled(pairIndex, enabled);
        if (!enabled)
        {
            const uint8_t modIndex = CvMidiBridge::kCcModIndices[pairIndex];
            if (m_onBeforeClearModRoutes)
            {
                m_onBeforeClearModRoutes(modIndex);
            }
            m_pageManager.ClearModRoutesForIndex(modIndex);
            if (m_delay)
            {
                m_delay->clearModRoutesForIndex(modIndex);
            }
            m_pairAr.clearModRoutesForIndex(modIndex);
        }
    }

    bool IsModSourceAvailable(uint8_t modIndex) const
    {
        return IsSimModSourceAvailable(modIndex, m_midiBridge, m_hostKind);
    }

    void PressButton(int button)
    {
        m_engine.ButtonCallback(button);
    }

    void SetCv(size_t index, float value)
    {
        if (index < 4)
        {
            m_pageManager.m_modMgr.m_mods[index] = value;
        }
    }

    void SetGate(bool high)
    {
        m_gateHigh = high;
    }

    void tickControls()
    {
        drainMutationQueue();
        m_midiBridge.drainMidiIn(m_pageManager.m_modMgr.m_mods, ModMgr::x_numMods);
        applyCvPresence(m_prevCv, m_cvPresence, m_pageManager.m_modMgr);
        if (IsV2SimHostKind(m_hostKind))
        {
            m_vcoAdsr.setGate(resolveVcoGate());
        }

        if (m_gateTrigger.Process(m_gateHigh ? 1.0f : 0.0f))
        {
            if (m_buttonCallback)
            {
                m_buttonCallback(0);
            }
            else
            {
                m_engine.ButtonCallback(0);
            }
        }
    }

    void ProcessBlock(const float* in, float* out, size_t n)
    {
        tickControls();
        if (IsV2SimHostKind(m_hostKind) && m_sequencer.advanceOnSamples(n, m_sampleRate))
        {
            m_vcoAdsr.setGate(resolveVcoGate());
            if (m_onSequencerStepAdvance)
            {
                m_onSequencerStepAdvance();
            }
        }
        if (!IsV2SimHostKind(m_hostKind))
        {
            m_pairAr.beginBlock(&m_pageManager.m_modMgr);
        }
        m_engine.ProcessBlock(in, out, n);
        updateMarblesScopeAccum();
        m_midiBridge.tickMidiOut(m_engine.GetEnvelopeLevel(), m_midiOut);
    }

    float GetCvOut(size_t modIndex) const
    {
        return GetSimCvOut(m_hostKind, m_pageManager.m_modMgr, m_v2ModTaps, modIndex);
    }

private:
    bool resolveVcoGate() const
    {
        return m_sequencer.m_playing
            ? (m_gateHigh || m_sequencer.activeStepGate())
            : true;
    }

    void configureV2FuegoPages()
    {
        for (uint8_t page = 0; page < m_pageManager.m_numPages; ++page)
        {
            const uint8_t crispyRow = V2ParamDisplayNames::CrispyRowForPage(page);
            m_pageManager.m_pages[page].ConfigureV2Fuego(&m_globalCrunchy, crispyRow, &m_v2ModTaps);
        }
    }
};
