#pragma once

#include "CvMidiBridge.hpp"
#include "CvPresence.hpp"
#include "ParamDisplayNames.hpp"
#include "SimModSource.hpp"

#include <cmath>
#include "FroggersEngine.hpp"
#include "Page.hpp"
#include "SchmidtTrigger.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

struct PagedHostIO
{
    PageManager m_pageManager;
    FroggersEngine m_engine;
    CvMidiBridge m_midiBridge;
    std::function<void(int)> m_buttonCallback;
    SchmidtTrigger m_gateTrigger{0.2f, 0.1f};
    float m_prevCv[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float m_cvPresence[4]{0.0f, 0.0f, 0.0f, 0.0f};
    bool m_sw1Down{false};
    bool m_sw2Down{false};
    bool m_gateHigh{false};
    bool m_sw1Pulse{false};
    bool m_sw2Pulse{false};
    bool m_marblesPulse{false};

    void Init()
    {
        m_engine.Config(&m_pageManager);
        m_pageManager.Finalize();
        m_pageManager.SanitizeSimModAssignments();
        m_engine.SetSimWaveMorph(true);
        m_engine.SetSimDedicatedPm3Knob(true);
        m_gateTrigger.Reset(m_gateHigh);
    }

    void SetSampleRate(float sampleRate)
    {
        m_engine.SetSampleRate(sampleRate);
    }

    void SetKnob(size_t index, float value)
    {
        if (index < Parameter::x_numParameters)
        {
            m_pageManager.m_knobPositions[index] = value;
        }
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

    void SetPageModSource(uint8_t page, uint8_t position, uint8_t modIndex)
    {
        if (!IsValidSimModAssignment(modIndex))
        {
            return;
        }
        m_pageManager.SetPageModSource(page, position, modIndex);
    }

    void SetPageModDepth(uint8_t page, uint8_t position, float depth)
    {
        m_pageManager.SetPageModDepth(page, position, depth);
    }

    uint8_t GetPageModSource(uint8_t page, uint8_t position) const
    {
        return m_pageManager.GetPageModSource(page, position);
    }

    float GetPageModDepth(uint8_t page, uint8_t position) const
    {
        return m_pageManager.GetPageModDepth(page, position);
    }

    void SetVcoMorph(size_t index, float value)
    {
        m_engine.SetVcoMorph(index, value);
    }

    float GetVcoMorph(size_t index) const
    {
        return m_engine.GetVcoMorph(index);
    }

    void RandomizeVcoMorphs()
    {
        m_engine.RandomizeVcoMorphs();
    }

    void RandomizeAllPages()
    {
        m_pageManager.RandomizeAllPages();
    }

    void RandomizeAllMod()
    {
        m_pageManager.RandomizeAllPagesModSim();
    }

    void RandomizePage(uint8_t page)
    {
        m_pageManager.RandomizePage(page);
    }

    void RandomizePageMod(uint8_t page)
    {
        m_pageManager.RandomizePageModSim(page);
    }

    void NudgeVco3Morph()
    {
        m_engine.NudgeVcoMorph(2, 0.1f);
    }

    void CycleVcoMorph(size_t index)
    {
        m_engine.CycleVcoMorph(index);
    }

    float GetVcoDisplayMorph(size_t index) const
    {
        return m_engine.GetVcoDisplayMorph(index);
    }

    uint8_t GetRowModSource(size_t row) const
    {
        return m_pageManager.GetPageModSource(m_pageManager.m_currentPage, static_cast<uint8_t>(row));
    }

    float GetRowModDepth(size_t row) const
    {
        return m_pageManager.GetPageModDepth(m_pageManager.m_currentPage, static_cast<uint8_t>(row));
    }

    void SetRowModSource(size_t row, uint8_t modIndex)
    {
        if (modIndex == 0)
        {
            return;
        }
        if (!IsValidSimModAssignment(modIndex))
        {
            return;
        }
        m_pageManager.SetPageModSource(m_pageManager.m_currentPage, static_cast<uint8_t>(row), modIndex);
    }

    void SetRowModDepth(size_t row, float depth)
    {
        m_pageManager.SetPageModDepth(m_pageManager.m_currentPage, static_cast<uint8_t>(row), depth);
    }

    void SetSw1(bool down)
    {
        if (down && !m_sw1Down)
        {
            m_sw1Pulse = true;
        }
        m_sw1Down = down;
    }

    void SetSw2(bool down)
    {
        if (down && !m_sw2Down)
        {
            m_sw2Pulse = true;
        }
        m_sw2Down = down;
    }

    void PulsePagePrevious()
    {
        m_sw1Pulse = true;
    }

    void PulsePageNext()
    {
        m_sw2Pulse = true;
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

    void PulseMarbles()
    {
        m_marblesPulse = true;
    }

    void tickControls()
    {
        if (m_sw1Pulse)
        {
            m_pageManager.PagePrevious();
            m_sw1Pulse = false;
        }

        if (m_sw2Pulse)
        {
            m_pageManager.PageNext();
            m_sw2Pulse = false;
        }

        if (m_marblesPulse)
        {
            m_engine.ButtonCallback(0);
            m_marblesPulse = false;
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

        m_midiBridge.drainMidiIn(m_pageManager.m_modMgr.m_mods, ModMgr::x_numMods);
        applyCvPresence(m_prevCv, m_cvPresence, m_pageManager.m_modMgr);

        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pageManager.KnobUpdate(i, m_pageManager.m_knobPositions[i]);
        }
    }

    void ProcessBlock(const float* in, float* out, size_t n)
    {
        tickControls();
        m_engine.ProcessBlock(in, out, n);
    }

    const char* GetRowName(size_t row) const
    {
        return ParamDisplayNames::forHostPageRow(
            m_pageManager.m_currentPage, static_cast<uint8_t>(row));
    }

    const char* GetPageRowName(uint8_t page, uint8_t row) const
    {
        return ParamDisplayNames::forHostPageRow(page, row);
    }

    float GetRowValue(size_t row) const
    {
        float value = m_pageManager.GetParamCurrentPageOrMod(static_cast<uint8_t>(row));
        if (!std::isfinite(value))
        {
            return 0.0f;
        }
        return value;
    }

    char GetRowTrackingBadge(size_t row) const
    {
        return m_pageManager.TrackingBadge(static_cast<uint8_t>(row));
    }

    uint8_t GetCurrentPage() const
    {
        return m_pageManager.m_currentPage;
    }

    uint8_t GetNumPages() const
    {
        return m_pageManager.m_numPages;
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
