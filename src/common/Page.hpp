#pragma once

#include "Parameter.hpp"
#include "ModMgr.hpp"
#include <cstddef>
#include <cstdint>

struct PageManager;

struct Page
{
    static constexpr size_t x_numParameters = 8;
    uint8_t m_pageId;
    Parameter m_parameters[x_numParameters];
    ModMgr* m_modMgr;

    void InitParam(const char* name, uint8_t position, float defaultValue)
    {
        m_parameters[position].Init(name, m_pageId, position, defaultValue);
    }

    float GetParam(uint8_t position)
    {
        return m_parameters[position].Get(m_modMgr);
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

    void SetFuegoization()
    {
        m_parameters[Parameter::x_numParameters - 1].Init("FUEG", m_pageId, Parameter::x_numParameters - 1, 0.0f);
        for (size_t i = 0; i < Parameter::x_numParameters - 1; i++)
        {
            m_parameters[i].m_fuegoizationKnob = &m_parameters[Parameter::x_numParameters - 1];
        }
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

    char TrackingBadge(uint8_t position)
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

    float GetParamCurrentPage(uint8_t position)
    {
        return m_pages[m_currentPage].GetParam(position);
    }

    float GetParamCurrentPageOrMod(uint8_t position)
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

    const char* GetNameCurrentPage(uint8_t position)
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
        SelectPage((m_currentPage + 1) % m_numPages);
    }

    void PagePrevious()
    {
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