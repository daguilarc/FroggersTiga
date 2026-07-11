#pragma once

#include "Parameter.hpp"
#include "ModMgr.hpp"
#include "SimModSource.hpp"
#include "PermanentModTapRack.hpp"
#include "V2FuegoStack.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>

struct PageManager;
struct V2LaneDepthStore;

// Defined in V2LaneDepthStore.hpp; forward-declared here so Page can call it
// without requiring V2LaneDepthStore's full definition (which itself needs
// PageManager::x_numPages, included at the bottom of this file).
inline float ApplyV2LaneMod(float knob,
                            uint8_t page,
                            uint8_t position,
                            const V2LaneDepthStore& depths,
                            const PermanentModTapRack& taps);

struct Page
{
    static constexpr size_t x_numParameters = Parameter::x_numParameters;
    uint8_t m_pageId;
    Parameter m_parameters[x_numParameters];
    ModMgr* m_modMgr;
    bool m_useV2Fuego = false;
    float* m_globalCrunchy = nullptr;
    const PermanentModTapRack* m_v2ModTaps = nullptr;
    const V2LaneDepthStore* m_v2LaneDepths = nullptr;
    uint8_t m_v2CrispyRow = static_cast<uint8_t>(Parameter::x_numParameters - 1);

    void InitParam(const char* name, uint8_t position, float defaultValue)
    {
        m_parameters[position].Init(name, m_pageId, position, defaultValue);
    }

    float GetParam(uint8_t position) const
    {
        if (m_useV2Fuego)
        {
            return GetParamV2(position);
        }
        return m_parameters[position].Get(m_modMgr);
    }

    void SetEffectiveOverride(uint8_t position, float value)
    {
        if (position >= x_numParameters)
        {
            return;
        }
        m_parameters[position].SetEffectiveOverride(value);
    }

    void ClearEffectiveOverride(uint8_t position)
    {
        if (position >= x_numParameters)
        {
            return;
        }
        m_parameters[position].ClearEffectiveOverride();
    }

    void ClearEffectiveOverrides()
    {
        for (size_t i = 0; i < x_numParameters; i++)
        {
            m_parameters[i].ClearEffectiveOverride();
        }
    }

    bool IsTracking(uint8_t position)
    {
        return m_parameters[position].IsTracking();
    }

    bool IsModTracking(uint8_t position)
    {
        return m_parameters[position].IsModTracking();
    }

    void PageDeSelectParameter(uint8_t position, float knobValue)
    {
        m_parameters[position].PageDeSelect(knobValue);
    }

    void PageSelectParameter(uint8_t position, float knobPosition)
    {
        m_parameters[position].PageSelect(knobPosition);
    }

    void SetFuegoization(uint8_t crispyRow = 7)
    {
        if (crispyRow >= Parameter::x_numParameters)
        {
            return;
        }
        m_parameters[crispyRow].Init("FUEG", m_pageId, crispyRow, 0.0f);
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            if (i == crispyRow)
            {
                continue;
            }
            m_parameters[i].m_fuegoizationKnob = &m_parameters[crispyRow];
        }
    }

    void ConfigureV2Fuego(float* globalCrunchy,
                          uint8_t crispyRow,
                          const PermanentModTapRack* v2ModTaps,
                          const V2LaneDepthStore* v2LaneDepths = nullptr)
    {
        m_useV2Fuego = true;
        m_globalCrunchy = globalCrunchy;
        m_v2CrispyRow = crispyRow;
        m_v2ModTaps = v2ModTaps;
        m_v2LaneDepths = v2LaneDepths;
    }

    float ApplyV2MusicalFuego(float preFuegoValue, uint8_t row) const
    {
        if (!m_useV2Fuego)
        {
            return preFuegoValue;
        }
        const float globalCrunchy = m_globalCrunchy ? *m_globalCrunchy : 0.0f;
        const float crispyKnobPre = GetPreFuegoValue(m_v2CrispyRow);
        return V2FuegoStack::ApplyMusicalRow(
            preFuegoValue, globalCrunchy, crispyKnobPre, row, m_v2CrispyRow);
    }

    void KnobUpdate(uint8_t position, float knobPosition, uint8_t modIndex)
    {
        if (modIndex == 255)
        {
            m_parameters[position].KnobUpdate(knobPosition);
        }
        else
        {
            m_parameters[position].ModUpdate(modIndex, knobPosition);
        }
    }

private:
    float GetPreFuegoValue(uint8_t position) const
    {
        if (position >= x_numParameters)
        {
            return 0.0f;
        }
        const Parameter& param = m_parameters[position];
        if (m_useV2Fuego && m_v2LaneDepths && m_v2ModTaps)
        {
            return ApplyV2LaneMod(param.m_knobValue, m_pageId, position, *m_v2LaneDepths, *m_v2ModTaps);
        }
        if (!m_modMgr || param.m_modIndex == 255)
        {
            return param.m_knobValue;
        }
        if (m_useV2Fuego && m_v2ModTaps && param.m_modIndex <= PermanentModTapRack::kLastIndex)
        {
            const float tap = m_v2ModTaps->GetTap(param.m_modIndex);
            return std::min(
                std::max(param.m_knobValue * (1.0f - param.m_modAmount) + tap * param.m_modAmount, 0.0f), 1.0f);
        }
        if (param.m_modIndex <= 6)
        {
            return m_modMgr->Modulate(param.m_knobValue, param.m_modIndex, param.m_modAmount);
        }
        return param.m_knobValue;
    }

    float GetParamV2(uint8_t position) const
    {
        const float value = GetPreFuegoValue(position);
        if (position == m_v2CrispyRow || position >= x_numParameters)
        {
            const float globalCrunchy = m_globalCrunchy ? *m_globalCrunchy : 0.0f;
            return V2FuegoStack::ApplyGlobal(value, globalCrunchy, position);
        }
        return ApplyV2MusicalFuego(value, position);
    }
};

struct PageManager
{
    static constexpr size_t x_numPages = 8;
    Page m_pages[x_numPages];
    float m_knobPositions[Parameter::x_numParameters];
    uint8_t m_numPages;
    uint8_t m_currentPage;
    ModMgr m_modMgr;
    uint8_t m_modIndex;
    
    void StartModTracking(int modIndex)
    {
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].m_parameters[i].StartModTracking(modIndex, m_knobPositions[i]);
        }

        m_modIndex = modIndex;
    }

    void StopModTracking()
    {
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].m_parameters[i].StopModTracking(m_knobPositions[i]);
        }

        m_modIndex = 255;
    }

    PageManager()
    {
        m_numPages = 0;
        m_currentPage = 0;
        m_modIndex = 255;
        for (size_t i = 0; i < x_numPages; i++)
        {
            m_pages[i].m_modMgr = &m_modMgr;
        }
    }

    Page* AddPage()
    {
        m_pages[m_numPages].m_pageId = m_numPages;
        m_pages[m_numPages].m_modMgr = &m_modMgr;
        m_numPages++;
        return &m_pages[m_numPages - 1];
    }

    void InitParam(const char* name, uint8_t page, uint8_t position, float defaultValue)
    {
        m_pages[page].InitParam(name, position, defaultValue);
    }

    float GetParam(uint8_t page, uint8_t position)
    {
        return m_pages[page].GetParam(position);
    }

    uint8_t GetModIndex(uint8_t position)
    {
        return m_pages[m_currentPage].m_parameters[position].m_modIndex;
    }

    bool IsTracking(uint8_t position)
    {
        if (m_modIndex != 255)
        {
            return m_pages[m_currentPage].IsModTracking(position);
        }
        else
        {
            return m_pages[m_currentPage].IsTracking(position);
        }
    }

    char TrackingBadge(uint8_t position) const
    {
        if (m_modIndex != 255)
        {
            return m_pages[m_currentPage].m_parameters[position].ModTrackingBadge();
        }
        else
        {
            return m_pages[m_currentPage].m_parameters[position].TrackingBadge();
        }
    }

    void KnobUpdate(uint8_t position, float knobPosition)
    {
        m_knobPositions[position] = knobPosition;
        m_pages[m_currentPage].KnobUpdate(position, knobPosition, m_modIndex);
    }

    void KnobUpdateOnPage(uint8_t page, uint8_t position, float knobPosition)
    {
        m_pages[page].KnobUpdate(position, knobPosition, m_modIndex);
    }

    void RandomizePage(uint8_t page)
    {
        for (size_t j = 0; j < Parameter::x_numParameters; j++)
        {
            float knobPos = m_pages[page].m_parameters[j].m_knobValue;
            m_pages[page].m_parameters[j].Randomize(knobPos);
        }
    }

    void RandomizeAllPagesIndependent()
    {
        for (uint8_t i = 0; i < m_numPages; i++)
        {
            RandomizePage(i);
        }
    }

    void ClearEffectiveOverrides()
    {
        for (uint8_t p = 0; p < m_numPages; p++)
        {
            m_pages[p].ClearEffectiveOverrides();
        }
    }

    void RandomizePageMod(uint8_t page)
    {
        for (size_t j = 0; j < Parameter::x_numParameters; j++)
        {
            float knobPos = m_pages[page].m_parameters[j].m_knobValue;
            m_pages[page].m_parameters[j].RandomizeMod(knobPos);
        }
    }

    void RandomizeAllPagesModIndependent()
    {
        for (uint8_t i = 0; i < m_numPages; i++)
        {
            RandomizePageMod(i);
        }
    }

    void RandomizePageModSim(uint8_t page, const CvMidiBridge& bridge, SimHostKind hostKind)
    {
        for (size_t j = 0; j < Parameter::x_numParameters; j++)
        {
            float knobPos = m_pages[page].m_parameters[j].m_knobValue;
            m_pages[page].m_parameters[j].RandomizeModSim(knobPos, bridge, hostKind);
        }
    }

    void RandomizeAllPagesModSim(const CvMidiBridge& bridge, SimHostKind hostKind)
    {
        for (uint8_t i = 0; i < m_numPages; i++)
        {
            RandomizePageModSim(i, bridge, hostKind);
        }
    }

    void ClearModRoutesForIndex(uint8_t modIndex)
    {
        for (uint8_t p = 0; p < m_numPages; p++)
        {
            for (size_t i = 0; i < Parameter::x_numParameters; i++)
            {
                Parameter& param = m_pages[p].m_parameters[i];
                if (param.m_modIndex == modIndex)
                {
                    param.m_modIndex = 255;
                    param.m_modAmount = 0.0f;
                }
            }
        }
    }

    void SanitizeSimModAssignments()
    {
        for (uint8_t p = 0; p < m_numPages; p++)
        {
            for (size_t i = 0; i < Parameter::x_numParameters; i++)
            {
                Parameter& param = m_pages[p].m_parameters[i];
                if (param.m_modIndex >= 1 && param.m_modIndex <= 3)
                {
                    param.m_modIndex = 255;
                    param.m_modAmount = 0.0f;
                }
            }
        }
    }

    void SetAllParamsTracking()
    {
        for (uint8_t p = 0; p < m_numPages; p++)
        {
            for (size_t i = 0; i < Parameter::x_numParameters; i++)
            {
                m_pages[p].m_parameters[i].ForceTracking();
            }
        }
    }

    uint8_t GetPageModSource(uint8_t page, uint8_t position) const
    {
        return m_pages[page].m_parameters[position].m_modIndex;
    }

    float GetPageModDepth(uint8_t page, uint8_t position) const
    {
        const Parameter& param = m_pages[page].m_parameters[position];
        if (param.m_modIndex == 255)
        {
            return 0.0f;
        }
        return param.m_modAmount;
    }

    void SetPageModSource(uint8_t page, uint8_t position, uint8_t modIndex)
    {
        Parameter& param = m_pages[page].m_parameters[position];
        const bool valid = modIndex == 255
            || IsSimAssignableModIndex(modIndex, SimHostKind::Desktop)
            || IsPermanentModSourceIndex(modIndex);
        if (!valid)
        {
            param.m_modIndex = 255;
            param.m_modAmount = 0.0f;
        }
        else
        {
            param.m_modIndex = modIndex;
            if (param.m_modAmount < Parameter::x_knobEpsilon)
            {
                param.m_modAmount = 0.5f;
            }
        }
        param.m_modTrackingState = Parameter::TrackingState::Tracking;
    }

    void SetPageModDepth(uint8_t page, uint8_t position, float depth)
    {
        Parameter& param = m_pages[page].m_parameters[position];
        if (param.m_modIndex == 255)
        {
            return;
        }
        param.m_modAmount = depth;
        param.m_modTrackingState = Parameter::TrackingState::Tracking;
    }

    float GetParamCurrentPage(uint8_t position) const
    {
        return m_pages[m_currentPage].GetParam(position);
    }

    float GetParamCurrentPageOrMod(uint8_t position) const
    {
        if (m_modIndex != 255)
        {
            return m_pages[m_currentPage].m_parameters[position].GetModAmount(m_modIndex);
        }
        else
        {
            return m_pages[m_currentPage].GetParam(position);
        }
    }

    const char* GetNameCurrentPage(uint8_t position) const
    {
        return m_pages[m_currentPage].m_parameters[position].GetName();
    }

    void Finalize()
    {
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].PageSelectParameter(i, m_knobPositions[i]);
        }
    }

    void RandomizeCurrentPage()
    {
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].m_parameters[i].Randomize(m_knobPositions[i]);
        }
    }

    void RandomizeCurrentPageMod()
    {
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].m_parameters[i].RandomizeMod(m_knobPositions[i]);
        }
    }

    void RandomizeAllPages()
    {
        for (size_t i = 0; i < m_numPages; i++)
        {
            for (size_t j = 0; j < Parameter::x_numParameters; j++)
            {
                m_pages[i].m_parameters[j].Randomize(m_knobPositions[j]);
            }
        }
    }

    void RandomizeAllPagesMod()
    {
        for (size_t i = 0; i < m_numPages; i++)
        {
            for (size_t j = 0; j < Parameter::x_numParameters; j++)
            {
                m_pages[i].m_parameters[j].RandomizeMod(m_knobPositions[j]);
            }
        }
    }

    void SelectPage(uint8_t page)
    {
        m_modIndex = 255;
        m_currentPage = page;
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pages[m_currentPage].PageDeSelectParameter(i, m_knobPositions[i]);
            m_pages[m_currentPage].PageSelectParameter(i, m_knobPositions[i]);
        }
    }

    void PageNext()
    {
        if (m_modIndex != 255)
        {
            StopModTracking();
        }
        SelectPage((m_currentPage + 1) % m_numPages);
    }

    void PagePrevious()
    {
        if (m_modIndex != 255)
        {
            StopModTracking();
        }
        if (m_currentPage == 0)
        {
            SelectPage(m_numPages - 1);
        }
        else
        {
            SelectPage(m_currentPage - 1);
        }
    }
};

// Included at the bottom (not the top) of this file on purpose: V2LaneDepthStore
// sizes its array from PageManager::x_numPages, so it must be defined after
// PageManager above. Page itself only needs the forward declaration + the
// ApplyV2LaneMod prototype declared near the top of this file.
#include "V2LaneDepthStore.hpp"
