#pragma once

#include "Fuegoize.hpp"
#include "ModMgr.hpp"
#include "RGen.hpp"
#include "RuntimeParam.hpp"
#include "SimModSource.hpp"
#include "ParamDisplayNames.hpp"
#include "StereoDelay.hpp"
#include "V2FuegoStack.hpp"
#include "V2ParamDisplayNames.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct DelayState
{
    static constexpr uint8_t kDelayPageIndex = 5;
    static constexpr uint8_t kNumRows = 10;
    static constexpr uint8_t kLegacyRows = 8;
    static constexpr uint8_t kExpandedRows = 10;

    static const char* rowName(uint8_t row)
    {
        return V2ParamDisplayNames::forHostPageRow(kDelayPageIndex, row);
    }

    void init(float sampleRate)
    {
        setSampleRate(sampleRate);
        for (uint8_t i = 0; i < kNumRows; i++)
        {
            knobs[i] = 0.5f;
            modSource[i] = 255;
            modDepth[i] = 0.5f;
            effectiveOverrideActive[i] = false;
            effectiveOverrideValue[i] = 0.0f;
            smoothed[i].SetTarget(knobs[i]);
        }
        knobs[crispyRow()] = 0.0f;
        smoothed[crispyRow()].SetTarget(0.0f);
    }

    void setSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
        delay.setSampleRate(m_sampleRate);
        for (uint8_t i = 0; i < kNumRows; i++)
        {
            smoothed[i].SetSmoothingRate(m_sampleRate);
            smoothed[i].SetTarget(knobs[i]);
        }
    }

    void beginBlock(const ModMgr* modMgr)
    {
        m_modMgr = modMgr;
    }

    void setKnob(uint8_t row, float value)
    {
        if (row >= kNumRows)
        {
            return;
        }
        knobs[row] = std::min(std::max(value, 0.0f), 1.0f);
        smoothed[row].SetTarget(knobs[row]);
    }

    float getKnob(uint8_t row) const
    {
        return row < kNumRows ? knobs[row] : 0.0f;
    }

    void setModSource(uint8_t row, uint8_t modIndex)
    {
        if (row >= kNumRows || !IsValidSimModAssignment(modIndex))
        {
            return;
        }
        modSource[row] = modIndex;
    }

    uint8_t getModSource(uint8_t row) const
    {
        return row < kNumRows ? modSource[row] : 255;
    }

    void setModDepth(uint8_t row, float depth)
    {
        if (row >= kNumRows)
        {
            return;
        }
        modDepth[row] = std::min(std::max(depth, 0.0f), 1.0f);
    }

    float getModDepth(uint8_t row) const
    {
        return row < kNumRows ? modDepth[row] : 0.0f;
    }

    float getEffectiveKnob(uint8_t row) const
    {
        if (row >= activeRowCount())
        {
            return 0.0f;
        }
        if (row == crispyRow())
        {
            const float crispyPre = blendKnob(row, knobs[row]);
            if (m_useV2Layout)
            {
                return V2FuegoStack::ApplyGlobal(crispyPre, effectiveGlobalCrunchy(), row);
            }
            return crispyPre;
        }
        if (modSource[row] == 255 && !m_useV2Layout)
        {
            return Fuegoize(knobs[row], effectiveCrispy(), row);
        }
        return blendRow(row, knobs[row]);
    }

    void setEffectiveOverride(uint8_t row, float value)
    {
        if (row >= kNumRows)
        {
            return;
        }
        effectiveOverrideValue[row] = std::min(std::max(value, 0.0f), 1.0f);
        effectiveOverrideActive[row] = true;
    }

    void clearEffectiveOverride(uint8_t row)
    {
        if (row >= kNumRows)
        {
            return;
        }
        effectiveOverrideActive[row] = false;
        effectiveOverrideValue[row] = 0.0f;
    }

    void clearEffectiveOverrides()
    {
        for (uint8_t i = 0; i < kNumRows; i++)
        {
            clearEffectiveOverride(i);
        }
    }

    float blendRow(uint8_t row, float knobSmoothed) const
    {
        if (m_useV2Layout)
        {
            return V2FuegoStack::ApplyMusicalRow(
                blendKnob(row, knobSmoothed), effectiveGlobalCrunchy(), effectiveCrispy(), row, crispyRow());
        }
        return Fuegoize(blendKnob(row, knobSmoothed), effectiveCrispy(), row);
    }

    float processInsert(float bumpIn)
    {
        float knobSmoothed[9];
        for (uint8_t i = 0; i < 7; i++)
        {
            knobSmoothed[i] = smoothed[i].Process();
        }
        if (m_useV2Layout)
        {
            knobSmoothed[7] = smoothed[7].Process();
            knobSmoothed[8] = smoothed[8].Process();
        }
        smoothed[crispyRow()].Process();

        DelayParams params;
        params.dtim = blendRow(0, knobSmoothed[0]);
        params.dsnd = blendRow(1, knobSmoothed[1]);
        params.dfbk = blendRow(2, knobSmoothed[2]);
        params.dwid = blendRow(3, knobSmoothed[3]);
        params.ddet = blendRow(4, knobSmoothed[4]);
        params.dmod = blendRow(5, knobSmoothed[5]);
        params.dmix = blendRow(6, knobSmoothed[6]);
        if (m_useV2Layout)
        {
            const float color = blendRow(7, knobSmoothed[7]);
            const float halo = blendRow(8, knobSmoothed[8]);
            params.ddet = std::min(std::max(0.5f * (params.ddet + color), 0.0f), 1.0f);
            params.dmod = std::min(std::max(0.5f * (params.dmod + halo), 0.0f), 1.0f);
        }

        m_lastDmix = params.dmix;
        const WetPair wet = delay.process(bumpIn, params);
        m_lastWet = wet;
        return delay.toReverbMono(bumpIn, wet, params.dmix);
    }

    WetPair getLastWet() const
    {
        return m_lastWet;
    }

    float getLastDmix() const
    {
        return m_lastDmix;
    }

    void randomizeKnobs()
    {
        RGen rgen;
        for (uint8_t i = 0; i < activeRowCount(); i++)
        {
            if (i == crispyRow())
            {
                continue;
            }
            setKnob(i, rgen.UniGenRange(0.0f, 1.0f));
        }
        setKnob(crispyRow(), 0.0f);
    }

    void randomizeMod(const CvMidiBridge& bridge, SimHostKind hostKind)
    {
        RGen rgen;
        for (uint8_t i = 0; i < activeRowCount(); i++)
        {
            modDepth[i] = rgen.UniGenRange(0.0f, 1.0f);
            modSource[i] = DrawAssignableModLane(rgen, bridge, hostKind);
        }
    }

    void clearModRoutesForIndex(uint8_t modIndex)
    {
        for (uint8_t i = 0; i < activeRowCount(); i++)
        {
            if (modSource[i] == modIndex)
            {
                modSource[i] = 255;
                modDepth[i] = 0.0f;
            }
        }
    }

    void sanitizeModSources()
    {
        for (uint8_t i = 0; i < activeRowCount(); i++)
        {
            if (!IsValidSimModAssignment(modSource[i]))
            {
                modSource[i] = 255;
                modDepth[i] = 0.0f;
            }
        }
    }

    void softResetFx()
    {
        delay.clearBuffers();
        m_lastWet = {};
        m_lastDmix = 0.0f;
    }

    std::array<float, kNumRows> knobs{};
    std::array<uint8_t, kNumRows> modSource{};
    std::array<float, kNumRows> modDepth{};
    std::array<bool, kNumRows> effectiveOverrideActive{};
    std::array<float, kNumRows> effectiveOverrideValue{};
    std::array<RuntimeParam, kNumRows> smoothed{};
    StereoDelay delay;

    void setGlobalCrunchyPtr(const float* globalCrunchy)
    {
        m_globalCrunchy = globalCrunchy;
    }

    void setUseV2Layout(bool enabled)
    {
        m_useV2Layout = enabled;
        knobs[crispyRow()] = 0.0f;
        smoothed[crispyRow()].SetTarget(0.0f);
    }

private:
    uint8_t activeRowCount() const
    {
        return m_useV2Layout ? kExpandedRows : kLegacyRows;
    }

    uint8_t crispyRow() const
    {
        return m_useV2Layout ? V2ParamDisplayNames::CrispyRowForPage(kDelayPageIndex) : 7;
    }

    float effectiveGlobalCrunchy() const
    {
        if (!m_globalCrunchy)
        {
            return 0.0f;
        }
        return std::min(std::max(*m_globalCrunchy, 0.0f), 1.0f);
    }

    float blendKnob(uint8_t row, float base) const
    {
        if (row < kNumRows && effectiveOverrideActive[row])
        {
            return effectiveOverrideValue[row];
        }
        if (!m_modMgr || row >= kNumRows || modSource[row] == 255 || modDepth[row] <= 0.0f)
        {
            return base;
        }
        return m_modMgr->Modulate(base, static_cast<int>(modSource[row]), modDepth[row]);
    }

    float effectiveCrispy() const
    {
        return blendKnob(crispyRow(), knobs[crispyRow()]);
    }

    float m_sampleRate = 44100.0f;
    const ModMgr* m_modMgr = nullptr;
    WetPair m_lastWet{};
    float m_lastDmix = 0.0f;
    bool m_useV2Layout = false;
    const float* m_globalCrunchy = nullptr;
};

inline float simDelayInsertCallback(float bumpIn, void* ctx)
{
    return static_cast<DelayState*>(ctx)->processInsert(bumpIn);
}

struct StereoFxSpread
{
    float delayMix = 0.0f;
    float delayDeltaL = 0.0f;
    float delayDeltaR = 0.0f;
    float reverbMix = 0.0f;
    float reverbDeltaL = 0.0f;
    float reverbDeltaR = 0.0f;
};

inline StereoFxSpread makeStereoFxSpread(const DelayState& delay,
                                         float reverbDeltaL,
                                         float reverbDeltaR,
                                         float reverbMix)
{
    const WetPair wet = delay.getLastWet();
    const float dmix = std::min(std::max(delay.getLastDmix(), 0.0f), 1.0f);
    const float monoWet = (wet.l + wet.r) * 0.5f;
    StereoFxSpread spread;
    spread.delayMix = dmix;
    spread.delayDeltaL = wet.l - monoWet;
    spread.delayDeltaR = wet.r - monoWet;
    spread.reverbMix = reverbMix;
    spread.reverbDeltaL = reverbDeltaL;
    spread.reverbDeltaR = reverbDeltaR;
    return spread;
}

inline void applyStereoBus(const float* coreMono,
                           float* outL,
                           float* outR,
                           size_t n,
                           const StereoFxSpread& spread,
                           int numOutputChannels)
{
    const float dmix = spread.delayMix;
    const float delayL = spread.delayDeltaL;
    const float delayR = spread.delayDeltaR;
    const float rvMix = spread.reverbMix;
    const float revL = spread.reverbDeltaL;
    const float revR = spread.reverbDeltaR;

    if (numOutputChannels >= 2 && outR)
    {
        for (size_t i = 0; i < n; i++)
        {
            const float mono = coreMono[i];
            outL[i] = mono + dmix * delayL + rvMix * revL;
            outR[i] = mono + dmix * delayR + rvMix * revR;
        }
        return;
    }

    for (size_t i = 0; i < n; i++)
    {
        const float mono = coreMono[i];
        const float left = mono + dmix * delayL + rvMix * revL;
        const float right = mono + dmix * delayR + rvMix * revR;
        outL[i] = (left + right) * 0.5f;
    }
}
