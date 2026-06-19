#pragma once

#include "CvMidiBridge.hpp"
#include "ModMgr.hpp"
#include "RGen.hpp"
#include "RuntimeParam.hpp"
#include "SimModSource.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct AudioPairArState
{
    static constexpr uint8_t kCount = 4;

    void init(float sampleRate)
    {
        setSampleRate(sampleRate);
        for (uint8_t i = 0; i < kCount; i++)
        {
            knobs[i] = 0.5f;
            modSource[i] = 255;
            modDepth[i] = 0.5f;
            smoothed[i].SetTarget(knobs[i]);
        }
    }

    void setSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
        for (uint8_t i = 0; i < kCount; i++)
        {
            smoothed[i].SetSmoothingRate(m_sampleRate);
            smoothed[i].SetTarget(knobs[i]);
        }
    }

    void beginBlock(const ModMgr* modMgr)
    {
        m_modMgr = modMgr;
    }

    void tickSmoothers()
    {
        for (uint8_t i = 0; i < kCount; i++)
        {
            m_effectiveSmoothed[i] = blendKnob(i, smoothed[i].Process());
        }
    }

    float getEffectiveSmoothed(uint8_t index) const
    {
        return index < kCount ? m_effectiveSmoothed[index] : 0.5f;
    }

    void setKnob(uint8_t index, float value)
    {
        if (index >= kCount)
        {
            return;
        }
        knobs[index] = std::min(std::max(value, 0.0f), 1.0f);
        smoothed[index].SetTarget(knobs[index]);
    }

    float getKnob(uint8_t index) const
    {
        return index < kCount ? knobs[index] : 0.0f;
    }

    void setModSource(uint8_t index, uint8_t modIndex)
    {
        if (index >= kCount || !IsValidSimModAssignment(modIndex))
        {
            return;
        }
        modSource[index] = modIndex;
    }

    uint8_t getModSource(uint8_t index) const
    {
        return index < kCount ? modSource[index] : 255;
    }

    void setModDepth(uint8_t index, float depth)
    {
        if (index >= kCount)
        {
            return;
        }
        modDepth[index] = std::min(std::max(depth, 0.0f), 1.0f);
    }

    float getModDepth(uint8_t index) const
    {
        return index < kCount ? modDepth[index] : 0.0f;
    }

    float getEffectiveKnob(uint8_t index, const ModMgr* modMgr) const
    {
        if (index >= kCount)
        {
            return 0.0f;
        }
        return blendKnob(index, knobs[index], modMgr);
    }

    float getEffectiveKnob(uint8_t index) const
    {
        return getEffectiveKnob(index, m_modMgr);
    }

    void randomizeMod(const CvMidiBridge& bridge, SimHostKind hostKind)
    {
        RGen rgen;
        for (uint8_t i = 0; i < kCount; i++)
        {
            modDepth[i] = rgen.UniGenRange(0.0f, 1.0f);
            modSource[i] = PickSimRandomModIndex(rgen, bridge, hostKind);
        }
    }

    void randomizeKnobs()
    {
        RGen rgen;
        for (uint8_t i = 0; i < kCount; i++)
        {
            setKnob(i, rgen.UniGenRange(0.0f, 1.0f));
        }
    }

    void clearModRoutesForIndex(uint8_t modIndex)
    {
        for (uint8_t i = 0; i < kCount; i++)
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
        for (uint8_t i = 0; i < kCount; i++)
        {
            if (!IsValidSimModAssignment(modSource[i]))
            {
                modSource[i] = 255;
                modDepth[i] = 0.0f;
            }
        }
    }

    std::array<float, kCount> knobs{};
    std::array<uint8_t, kCount> modSource{};
    std::array<float, kCount> modDepth{};
    std::array<RuntimeParam, kCount> smoothed{};

private:
    float blendKnob(uint8_t index, float knobValue, const ModMgr* modMgr) const
    {
        if (!modMgr || index >= kCount || modSource[index] == 255 || modDepth[index] <= 0.0f)
        {
            return knobValue;
        }
        return modMgr->Modulate(
            knobValue, static_cast<int>(modSource[index]), modDepth[index]);
    }

    float blendKnob(uint8_t index, float knobValue) const
    {
        return blendKnob(index, knobValue, m_modMgr);
    }

    float m_sampleRate = 44100.0f;
    const ModMgr* m_modMgr = nullptr;
    std::array<float, kCount> m_effectiveSmoothed{};
};
