#pragma once

#include "CvMidiBridge.hpp"
#include "CvPresence.hpp"
#include "DelayState.hpp"
#include "AudioPairArState.hpp"
#include "SimModSource.hpp"

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
    std::function<void(int)> m_buttonCallback;
    std::function<void(uint8_t, uint8_t, uint8_t)> m_midiOut;
    SchmidtTrigger m_gateTrigger{0.2f, 0.1f};
    float m_prevCv[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float m_cvPresence[4]{0.0f, 0.0f, 0.0f, 0.0f};
    bool m_gateHigh{false};
    std::array<HostMutation, kMutationQueueSize> m_mutationQueue{};
    std::atomic<int> m_mutationWrite{0};
    std::atomic<int> m_mutationRead{0};

    std::function<void(uint8_t)> m_onBeforeClearModRoutes;

    struct MarblesScopeAccum
    {
        float min = 0.0f;
        float max = 0.0f;
        bool initialized = false;
    };

    MarblesScopeAccum m_marblesScopeAccum[2]{};
    bool m_marblesScopeBlockReady = false;

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
                m_pageManager.RandomizePage(mutation.page);
                break;
            case HostMutationType::RandomizePageMod:
                m_pageManager.RandomizePageModSim(mutation.page, m_midiBridge);
                break;
            case HostMutationType::RandomizeAllPages:
                m_pageManager.RandomizeAllPagesIndependent();
                if (m_delay)
                {
                    m_delay->randomizeKnobs();
                }
                break;
            case HostMutationType::RandomizeAllMod:
                m_pageManager.RandomizeAllPagesModSim(m_midiBridge);
                m_pairAr.randomizeMod(m_midiBridge);
                if (m_delay)
                {
                    m_delay->randomizeMod(m_midiBridge);
                }
                break;
            case HostMutationType::SetPageModSource:
                if (IsValidSimModAssignment(mutation.modIndex)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge)))
                {
                    m_pageManager.SetPageModSource(mutation.page, mutation.row, mutation.modIndex);
                }
                break;
            case HostMutationType::DelaySetModSource:
                if (m_delay && IsValidSimModAssignment(mutation.modIndex)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge)))
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
                    m_delay->randomizeMod(m_midiBridge);
                }
                break;
            case HostMutationType::CycleMorph:
                m_engine.CycleVcoMorph(mutation.morphIndex);
                break;
            case HostMutationType::PairArSetModSource:
                if (IsValidSimModAssignment(mutation.modIndex)
                    && (mutation.modIndex == 255
                        || IsSimModSourceAvailable(mutation.modIndex, m_midiBridge)))
                {
                    m_pairAr.setModSource(mutation.row, mutation.modIndex);
                }
                break;
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
        m_pageManager.SetAllParamsTracking();
        m_pageManager.SanitizeSimModAssignments();
        m_pairAr.init(44100.0f);
        m_pairAr.sanitizeModSources();
        m_engine.SetAudioPairArState(&m_pairAr);
        if (m_delay)
        {
            m_delay->sanitizeModSources();
        }
        m_engine.SetSimWaveMorph(true);
        m_engine.SetSimDedicatedPm3Knob(true);
        m_gateTrigger.Reset(m_gateHigh);
    }

    void SetSampleRate(float sampleRate)
    {
        m_engine.SetSampleRate(sampleRate);
        m_pairAr.setSampleRate(sampleRate);
    }

    void SetPageKnob(uint8_t page, uint8_t position, float value)
    {
        m_pageManager.KnobUpdateOnPage(page, position, value);
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
        m_pairAr.setKnob(index, value);
    }

    float GetAudioPairArKnob(uint8_t index) const
    {
        return m_pairAr.getKnob(index);
    }

    float GetAudioPairArEffective(uint8_t index) const
    {
        return m_pairAr.getEffectiveKnob(index, m_pageManager.m_modMgr.m_mods);
    }

    uint8_t GetAudioPairArModSource(uint8_t index) const
    {
        return m_pairAr.getModSource(index);
    }

    float GetAudioPairArModDepth(uint8_t index) const
    {
        return m_pairAr.getModDepth(index);
    }

    void SetAudioPairArModDepth(uint8_t index, float depth)
    {
        m_pairAr.setModDepth(index, depth);
    }

    void SetAudioPairArModSource(uint8_t index, uint8_t modIndex)
    {
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
        return IsSimModSourceAvailable(modIndex, m_midiBridge);
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
        m_pairAr.beginBlock(m_pageManager.m_modMgr.m_mods);
        m_engine.ProcessBlock(in, out, n);
        updateMarblesScopeAccum();
        m_midiBridge.tickMidiOut(m_engine.GetEnvelopeLevel(), m_midiOut);
    }

    float GetCvOut(size_t modIndex) const
    {
        if (modIndex < ModMgr::x_numMods)
        {
            return m_pageManager.m_modMgr.m_mods[modIndex];
        }
        return 0.0f;
    }
};
