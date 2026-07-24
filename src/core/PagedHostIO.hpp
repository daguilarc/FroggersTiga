#pragma once

#include "CvMidiBridge.hpp"
#include "CvPresence.hpp"
#include "AudioPairArState.hpp"
#include "HostRandomize.hpp"
#include "ParamDisplayNames.hpp"
#include "SimModSource.hpp"
#include "V2EngineSetup.hpp"
#include "PermanentModTapRack.hpp"
#include "V2FuegoStack.hpp"
#include "V2LaneDepthStore.hpp"
#include "V2ParamDisplayNames.hpp"
#include "VcoAdsrState.hpp"

#include <cmath>
#include "FroggersEngine.hpp"
#include "Page.hpp"
#include "SchmidtTrigger.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>

struct PagedHostIO
{
    PageManager m_pageManager;
    FroggersEngine m_engine;
    AudioPairArState m_pairAr;
    CvMidiBridge m_midiBridge;
    SimHostKind m_hostKind = SimHostKind::Desktop;
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
    PermanentModTapRack m_v2ModTaps;
    V2LaneDepthStore m_v2LaneDepths;
    VcoAdsrState m_vcoAdsr;
    float m_globalCrunchy = 0.0f;
    bool m_enableCurrentPageKnobReplay = true;

    void Init()
    {
        m_engine.Config(&m_pageManager);
        if (UsesV2Fuego(m_hostKind))
        {
            V2EngineSetup::configure(m_pageManager, IsV2SimHostKind(m_hostKind));
            configureV2FuegoPages();
        }
        m_enableCurrentPageKnobReplay = m_hostKind != SimHostKind::Vcv;
        m_pairAr.init(44100.0f);
        m_pairAr.sanitizeModSources();
        m_pairAr.setV2FuegoConfig(&m_pageManager.m_pages[0], m_hostKind);
        if (IsV2SimHostKind(m_hostKind))
        {
            m_vcoAdsr.init(44100.0f);
            m_engine.SetAudioPairArState(nullptr);
            // Task 7.5 prerequisite fix: this must be the same PM page the
            // host writes ADSR knobs to. The shared engine PageManager layout
            // is Audio=0, Marbles=1, Reverb=2, Filter=3, Drive=4, ADSR=5
            // (desktop-v2's HostParameterInventoryV2::kPmAdsrPage == 5;
            // duplicated here as a literal rather than an include because
            // src/core/ must stay free of desktop-v2/ dependencies for the
            // Daisy/sim/VCV builds). Page index 6 is never configured, so the
            // envelope was previously wired to an inert page -- Attack/
            // Sustain/Release knobs did nothing.
            m_engine.SetVcoAdsrState(&m_vcoAdsr, &m_pageManager.m_pages[5]);
        }
        else
        {
            m_engine.SetAudioPairArState(&m_pairAr);
            m_engine.SetVcoAdsrState(nullptr, nullptr);
        }
        m_engine.SetUseV2FilterParallel(UsesV2Fuego(m_hostKind));
        m_engine.SetSimIndependentPm(UsesV2Fuego(m_hostKind));
        m_engine.SetSimWaveMorph(true);
        m_engine.SetSimDedicatedPm3Knob(true);
        m_pageManager.Finalize();
        m_pageManager.SanitizeSimModAssignments();
        m_gateTrigger.Reset(m_gateHigh);
    }

    void SetSampleRate(float sampleRate)
    {
        m_engine.SetSampleRate(sampleRate);
        m_pairAr.setSampleRate(sampleRate);
        m_vcoAdsr.setSampleRate(sampleRate);
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

    void SetPageModSource(uint8_t page, uint8_t position, uint8_t modIndex)
    {
        if (!IsValidSimModAssignment(modIndex, m_hostKind))
        {
            return;
        }
        if (modIndex != 255 && !IsSimModSourceAvailable(modIndex, m_midiBridge, m_hostKind))
        {
            return;
        }
        m_pageManager.SetPageModSource(page, position, modIndex);
    }

    void SetPageModDepth(uint8_t page, uint8_t position, float depth)
    {
        m_pageManager.SetPageModDepth(page, position, depth);
    }

    // Packet 15-B: V2-only per-lane depth write, mirroring SetPageModDepth's
    // direct (non-queued) write discipline in this file. Feeds the additive
    // multi-tap store (sim/V2LaneDepthStore.hpp) that Page::GetPreFuegoValue
    // reads via ApplyV2LaneMod.
    void SetPageLaneDepth(uint8_t page, uint8_t position, uint8_t lane, float depth)
    {
        m_v2LaneDepths.Set(page, position, lane, depth);
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
        RandomizeAllPagesWithPairAr(m_pageManager, m_pairAr);
    }

    void RandomizeAllMod()
    {
        m_pageManager.RandomizeAllPagesModSim(m_midiBridge, m_hostKind);
        m_pairAr.randomizeMod(m_midiBridge, m_hostKind);
    }

    void RandomizePage(uint8_t page)
    {
        RandomizePageWithExtras(m_pageManager, page, m_pairAr);
    }

    void RandomizePageMod(uint8_t page)
    {
        RandomizePageModWithExtras(m_pageManager, page, m_pairAr, m_midiBridge, m_hostKind);
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
            m_pageManager.ClearModRoutesForIndex(modIndex);
            m_pairAr.clearModRoutesForIndex(modIndex);
        }
    }

    bool IsModSourceAvailable(uint8_t modIndex) const
    {
        return IsSimModSourceAvailable(modIndex, m_midiBridge, m_hostKind);
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
        if (!IsValidSimModAssignment(modIndex, m_hostKind))
        {
            return;
        }
        if (modIndex != 255 && !IsSimModSourceAvailable(modIndex, m_midiBridge, m_hostKind))
        {
            return;
        }
        m_pageManager.SetPageModSource(m_pageManager.m_currentPage, static_cast<uint8_t>(row), modIndex);
    }

    void SetRowModDepth(size_t row, float depth)
    {
        m_pageManager.SetPageModDepth(m_pageManager.m_currentPage, static_cast<uint8_t>(row), depth);
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
        return m_pairAr.getEffectiveKnob(index, &m_pageManager.m_modMgr);
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
        if (!IsValidSimModAssignment(modIndex, m_hostKind))
        {
            return;
        }
        if (modIndex != 255 && !IsSimModSourceAvailable(modIndex, m_midiBridge, m_hostKind))
        {
            return;
        }
        m_pairAr.setModSource(index, modIndex);
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
        if (IsV2SimHostKind(m_hostKind))
        {
            m_vcoAdsr.setGate(m_gateHigh);
        }

        if (m_enableCurrentPageKnobReplay)
        {
            for (size_t i = 0; i < Parameter::x_numParameters; i++)
            {
                m_pageManager.KnobUpdate(i, m_pageManager.m_knobPositions[i]);
            }
        }
    }

    void ProcessBlock(const float* in, float* out, size_t n)
    {
        tickControls();
        m_pairAr.beginBlock(&m_pageManager.m_modMgr);
        m_engine.ProcessBlock(in, out, n);
    }

    const char* GetRowName(size_t row) const
    {
        if (UsesV2Fuego(m_hostKind))
        {
            return V2ParamDisplayNames::forHostPageRow(
                m_pageManager.m_currentPage, static_cast<uint8_t>(row));
        }
        return ParamDisplayNames::forHostPageRow(
            m_pageManager.m_currentPage, static_cast<uint8_t>(row));
    }

    const char* GetPageRowName(uint8_t page, uint8_t row) const
    {
        if (UsesV2Fuego(m_hostKind))
        {
            return V2ParamDisplayNames::forHostPageRow(page, row);
        }
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
        return GetSimCvOut(m_hostKind, m_pageManager.m_modMgr, m_v2ModTaps, modIndex);
    }

private:
    void configureV2FuegoPages()
    {
        for (uint8_t page = 0; page < m_pageManager.m_numPages; ++page)
        {
            const uint8_t crispyRow = V2ParamDisplayNames::CrispyRowForPage(page);
            m_pageManager.m_pages[page].ConfigureV2Fuego(
                &m_globalCrunchy,
                crispyRow,
                &m_v2ModTaps,
                V2FuegoFns{&ApplyV2FuegoOpaque, &V2FuegoStack::ApplyGlobal, &V2FuegoStack::ApplyMusicalRow},
                &m_v2LaneDepths);
        }
    }
};
