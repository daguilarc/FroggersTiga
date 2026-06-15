#pragma once

#include "DelayState.hpp"
#include "PagedHostIO.hpp"

#include <array>
#include <cstddef>
#include <vector>

struct WasmSimHost
{
    static constexpr size_t kScopeSize = 96;
    static constexpr std::array<uint8_t, 5> kScopeModIndices = {0, 1, 4, 5, 6};

    PagedHostIO io;
    DelayState delay;

    WasmSimHost()
    {
        io.Init();
        delay.init(44100.0f);
        io.m_engine.SetSimFxInsert(simDelayInsertCallback, &delay);
    }

    void setSampleRate(float sampleRate)
    {
        io.SetSampleRate(sampleRate);
        delay.setSampleRate(sampleRate);
    }

    void selectPage(uint8_t page)
    {
        if (page < io.GetNumPages())
        {
            io.m_pageManager.m_currentPage = page;
        }
    }

    void processBlock(const float* in, float* outL, float* outR, size_t n, int numOutputChannels)
    {
        delay.beginBlock(&io.m_pageManager.m_modMgr);
        std::vector<float> mono(n);
        io.ProcessBlock(in, mono.data(), n);
        pushScopeSamples();
        const StereoFxSpread spread = makeStereoFxSpread(
            delay,
            io.m_engine.getReverbStereoDeltaL(),
            io.m_engine.getReverbStereoDeltaR(),
            io.m_engine.getLastRvMix());
        applyStereoBus(mono.data(), outL, outR, n, spread, numOutputChannels);
    }

    size_t copyScopeSamples(uint8_t modIndex, float* out, size_t maxCount) const
    {
        const int slot = scopeSlot(modIndex);
        if (slot < 0 || !out || maxCount == 0)
        {
            return 0;
        }
        const size_t count = std::min(maxCount, kScopeSize);
        for (size_t i = 0; i < count; i++)
        {
            const size_t idx = (m_scopeWrite[slot] + i) % kScopeSize;
            out[i] = m_scopeRing[slot][idx];
        }
        return count;
    }

    void randomizeAllIncludingDelay()
    {
        io.RandomizeAllPages();
        delay.randomizeKnobs();
    }

    void randomizeAllModIncludingDelay()
    {
        io.RandomizeAllMod();
        delay.randomizeMod(io.m_midiBridge);
    }

    void setMidiCcPairEnabled(uint8_t pairIndex, bool enabled)
    {
        if (pairIndex >= 2)
        {
            return;
        }
        io.m_midiBridge.setCcPairEnabled(pairIndex, enabled);
        if (!enabled)
        {
            const uint8_t modIndex = CvMidiBridge::kCcModIndices[pairIndex];
            io.m_pageManager.ClearModRoutesForIndex(modIndex);
            delay.clearModRoutesForIndex(modIndex);
            io.m_pairAr.clearModRoutesForIndex(modIndex);
        }
    }

    bool isModSourceAvailable(uint8_t modIndex) const
    {
        return IsSimModSourceAvailable(modIndex, io.m_midiBridge);
    }

private:
    std::array<std::array<float, kScopeSize>, 5> m_scopeRing{};
    std::array<size_t, 5> m_scopeWrite{};

    int scopeSlot(uint8_t modIndex) const
    {
        for (size_t i = 0; i < kScopeModIndices.size(); i++)
        {
            if (kScopeModIndices[i] == modIndex)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void pushScopeSamples()
    {
        const float* mods = io.m_pageManager.m_modMgr.m_mods;
        if (!mods)
        {
            return;
        }
        for (size_t slot = 0; slot < kScopeModIndices.size(); slot++)
        {
            const uint8_t modIndex = kScopeModIndices[slot];
            m_scopeRing[slot][m_scopeWrite[slot]] = mods[modIndex];
            m_scopeWrite[slot] = (m_scopeWrite[slot] + 1) % kScopeSize;
        }
    }
};
