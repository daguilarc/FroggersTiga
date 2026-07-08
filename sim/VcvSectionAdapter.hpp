#pragma once

#include "DelayState.hpp"
#include "Fuegoize.hpp"
#include "HostRandomize.hpp"
#include "PagedHostIO.hpp"
#include "VcvModJack.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

enum class VcvSection : uint8_t
{
    Audio,
    Random,
    Filter,
    Drive,
    Reverb,
    Delay,
    Global,
    VcoAr,
};

struct VcvSectionRowState
{
    float base = 0.5f;
    uint8_t internalRoute = 255;
    float routeDepth = 0.0f;
    bool cvConnected = false;
    float cvVoltage = 0.0f;
};

struct VcvSectionSnapshot
{
    VcvSection section = VcvSection::Audio;
    std::array<VcvSectionRowState, Parameter::x_numParameters> rows{};
};

inline float effectiveVcvGlobalCrunchy(float knob, bool cvConnected, float cvVoltage)
{
    if (!cvConnected)
    {
        return std::clamp(knob, 0.0f, 1.0f);
    }
    return std::clamp(knob + cvVoltage / 10.0f, 0.0f, 1.0f);
}

struct VcvSectionAdapter
{
    static constexpr uint8_t kNoHostPage = 255;

    explicit VcvSectionAdapter(PagedHostIO& io, DelayState& delay)
        : m_io(io)
        , m_delay(delay)
    {
    }

    static constexpr uint8_t hostPageForSection(VcvSection section)
    {
        switch (section)
        {
        case VcvSection::Audio:
            return 0;
        case VcvSection::Random:
            return 1;
        case VcvSection::Reverb:
            return 2;
        case VcvSection::Filter:
            return 3;
        case VcvSection::Drive:
            return 4;
        case VcvSection::Delay:
            return DelayState::kDelayPageIndex;
        case VcvSection::Global:
        case VcvSection::VcoAr:
            return kNoHostPage;
        }
        return kNoHostPage;
    }

    void setSectionBaseValue(VcvSection section, uint8_t row, float value)
    {
        const uint8_t page = hostPageForSection(section);
        if (row >= Parameter::x_numParameters || page == kNoHostPage)
        {
            return;
        }
        if (page == DelayState::kDelayPageIndex)
        {
            m_delay.setKnob(row, value);
            return;
        }
        if (page >= m_io.m_pageManager.m_numPages)
        {
            return;
        }
        Parameter& param = m_io.m_pageManager.m_pages[page].m_parameters[row];
        param.m_knobValue = std::clamp(value, 0.0f, 1.0f);
        param.ForceTracking();
    }

    void setSectionInternalRoute(VcvSection section, uint8_t row, uint8_t modIndex, float depth)
    {
        const uint8_t page = hostPageForSection(section);
        if (row >= Parameter::x_numParameters || page == kNoHostPage)
        {
            return;
        }
        if (page == DelayState::kDelayPageIndex)
        {
            m_delay.setModSource(row, modIndex);
            m_delay.setModDepth(row, depth);
            return;
        }
        m_io.SetPageModSource(page, row, modIndex);
        m_io.SetPageModDepth(page, row, depth);
    }

    void applyRowEffective(VcvSection section,
                           uint8_t row,
                           float base,
                           bool cvConnected,
                           float cvVoltage,
                           float globalCrunchy)
    {
        const uint8_t page = hostPageForSection(section);
        if (row >= Parameter::x_numParameters || page == kNoHostPage)
        {
            return;
        }

        uint8_t modIndex = 255;
        float depth = 0.0f;
        if (page == DelayState::kDelayPageIndex)
        {
            modIndex = m_delay.getModSource(row);
            depth = m_delay.getModDepth(row);
        }
        else
        {
            modIndex = m_io.GetPageModSource(page, row);
            depth = m_io.GetPageModDepth(page, row);
        }

        const float withCv = applyVcvSectionCv(base,
                                               modIndex,
                                               depth,
                                               m_io.m_pageManager.m_modMgr,
                                               cvConnected,
                                               cvVoltage);
        const float preSectionCrispy = Fuegoize(withCv, std::clamp(globalCrunchy, 0.0f, 1.0f), row);
        applyPreSectionCrispyOverride(section, row, preSectionCrispy);
    }

    float effectiveSectionValue(VcvSection section,
                                uint8_t row,
                                float base,
                                bool cvConnected,
                                float cvVoltage,
                                float globalCrunchy) const
    {
        const uint8_t page = hostPageForSection(section);
        if (row >= Parameter::x_numParameters || page == kNoHostPage)
        {
            return 0.0f;
        }
        const uint8_t modIndex = page == DelayState::kDelayPageIndex
            ? m_delay.getModSource(row)
            : m_io.GetPageModSource(page, row);
        const float depth = page == DelayState::kDelayPageIndex
            ? m_delay.getModDepth(row)
            : m_io.GetPageModDepth(page, row);
        const float withCv = applyVcvSectionCv(base,
                                               modIndex,
                                               depth,
                                               m_io.m_pageManager.m_modMgr,
                                               cvConnected,
                                               cvVoltage);
        return Fuegoize(withCv, std::clamp(globalCrunchy, 0.0f, 1.0f), row);
    }

    void clearEffectiveOverrides()
    {
        m_io.m_pageManager.ClearEffectiveOverrides();
        m_delay.clearEffectiveOverrides();
    }

    void randomizeAllSections()
    {
        RandomizeAllPagesIndependentWithPairAr(m_io.m_pageManager, m_io.m_pairAr);
        m_delay.randomizeKnobs();
    }

    void randomizeAllSectionMods()
    {
        m_io.m_pageManager.RandomizeAllPagesModSim(m_io.m_midiBridge, m_io.m_hostKind);
        m_io.m_pairAr.randomizeMod(m_io.m_midiBridge, m_io.m_hostKind);
        m_delay.randomizeMod(m_io.m_midiBridge, m_io.m_hostKind);
    }

private:
    static float applyVcvSectionCv(float base,
                                   uint8_t modIndex,
                                   float depth,
                                   const ModMgr& modMgr,
                                   bool cvConnected,
                                   float cvVoltage)
    {
        const float internal = vcvInternalEffective(base, modIndex, depth, modMgr);
        if (!cvConnected)
        {
            return std::clamp(internal, 0.0f, 1.0f);
        }
        return std::clamp(internal + cvVoltage / 10.0f, 0.0f, 1.0f);
    }

    void applyPreSectionCrispyOverride(VcvSection section, uint8_t row, float value)
    {
        const uint8_t page = hostPageForSection(section);
        if (page == DelayState::kDelayPageIndex)
        {
            m_delay.setEffectiveOverride(row, value);
            return;
        }
        if (page >= m_io.m_pageManager.m_numPages)
        {
            return;
        }
        m_io.m_pageManager.m_pages[page].SetEffectiveOverride(row, value);
    }

    PagedHostIO& m_io;
    DelayState& m_delay;
};
