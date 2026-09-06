#pragma once

#include "FieldMutationQueue.hpp"
#include "FieldSwitchGuard.hpp"
#include "../core/Page.hpp"
#include "../core/RefreshGate.hpp"
#include "../core/SchmidtTrigger.hpp"
#include "daisy_field.h"
#include <cmath>
#include <cstdio>
#include <functional>

struct DaisyIO
{
    PageManager m_pageManager;
    daisy::DaisyField m_field;
    std::function<void(int)> m_buttonCallback;
    SchmidtTrigger m_gateTrigger{0.2f, 0.1f};
    float m_prevCv[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float m_cvPresence[4]{0.0f, 0.0f, 0.0f, 0.0f};
    FieldSwitchGuard m_switchGuard;
    FieldMutationQueue m_mutationQueue;
    RefreshGate m_screenGate;
    RefreshGate m_ledGate;
    float m_ledShadow[daisy::DaisyField::LED_LAST]{};

    // LED values are computed every poll; only the I2C transmit is rationed.
    // Without this the bus is driven at whatever rate MainLoop happens to spin
    // at, which is what starves the poll loop under load.
    void SetLedTracked(size_t index, float value)
    {
        if (m_ledShadow[index] != value)
        {
            m_ledShadow[index] = value;
            m_ledGate.m_dirty = true;
        }
        m_field.led_driver.SetLed(index, value);
    }

    void MarkScreenDirty()
    {
        m_screenGate.m_dirty = true;
    }

    void ProcessControls()
    {
        m_field.ProcessAllControls();

        m_switchGuard.UpdateSuppression(0, m_field.sw[0]);
        if (m_field.sw[0].RisingEdge() && m_switchGuard.AllowPageSwitch(0))
        {
            m_pageManager.PagePrevious();
            MarkScreenDirty();
        }

        m_switchGuard.UpdateSuppression(1, m_field.sw[1]);
        if (m_field.sw[1].RisingEdge() && m_switchGuard.AllowPageSwitch(1))
        {
            m_pageManager.PageNext();
            MarkScreenDirty();
        }

        if (m_pageManager.m_modIndex == 255)
        {
            if (m_field.KeyboardRisingEdge(0))
            {
                m_pageManager.RandomizeCurrentPage();
                MarkScreenDirty();
            }

            if (m_field.KeyboardRisingEdge(1))
            {
                m_mutationQueue.Enqueue(FieldMutationType::RandAll);
                MarkScreenDirty();
            }

            if (m_field.KeyboardRisingEdge(2))
            {
                m_pageManager.RandomizeCurrentPageMod();
                MarkScreenDirty();
            }

            if (m_field.KeyboardRisingEdge(3))
            {
                m_mutationQueue.Enqueue(FieldMutationType::RandAllMod);
                MarkScreenDirty();
            }
        }

        for (size_t i = 0; i < 4; i++)
        {
            if (m_field.KeyboardRisingEdge(i + 4))
            {
                m_buttonCallback(i);
            }
        }
        if (m_field.KeyboardRisingEdge(15))
        {
            m_buttonCallback(4);
        }

        if (m_gateTrigger.Process(m_field.gate_in.State()))
        {
            m_buttonCallback(0);
        }

        m_field.seed.SetLed(m_field.gate_in.State() ? 1.0f : 0.0f);

        m_field.SetCvOut1(m_pageManager.m_modMgr.m_mods[4] * 4096);
        m_field.SetCvOut2(m_pageManager.m_modMgr.m_mods[5] * 4096);

        for (size_t i = 0; i < 4; i++)
        {
            float cv = m_field.GetCvValue(i);
            float diff = std::fabs(cv - m_prevCv[i]);
            m_prevCv[i] = cv;
            float indicator = (0.02f < cv || 0.003f < diff) ? 1.0f : 0.0f;
            m_cvPresence[i] = std::max(m_cvPresence[i] * 0.98f, indicator);
            m_pageManager.m_modMgr.m_externalCvActive[i] = 0.1f < m_cvPresence[i];
            m_pageManager.m_modMgr.m_mods[i] = cv;
        }

        static constexpr uint8_t x_aAssignKeys[ModMgr::x_numMods] = {8, 9, 10, 11, 12, 13, 14};
        for (size_t i = 0; i < ModMgr::x_numMods; i++)
        {
            uint8_t key = x_aAssignKeys[i];
            if (m_field.KeyboardRisingEdge(key))
            {
                m_pageManager.StartModTracking(static_cast<uint8_t>(i));
            }
            else if (m_field.KeyboardFallingEdge(key) && m_pageManager.m_modIndex == i)
            {
                m_pageManager.StopModTracking();
            }

            SetLedTracked(i, m_pageManager.m_modIndex == i ? 1.0f : 0.0f);
        }

        SetLedTracked(7, 0.0f);

        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pageManager.KnobUpdate(i, m_field.knob[i].Process());
            SetLedTracked(i + 16, m_pageManager.IsTracking(i) ? 1.0f : 0.0f);
        }

        SetLedTracked(daisy::DaisyField::LED_SW_1, m_field.sw[0].Pressed() ? 1.0f : 0.0f);
        SetLedTracked(daisy::DaisyField::LED_SW_2, m_field.sw[1].Pressed() ? 1.0f : 0.0f);

        const uint32_t nowLed = daisy::System::GetNow();
        if (m_ledGate.Due(nowLed))
        {
            m_field.led_driver.SwapBuffersAndTransmit();
            m_ledGate.MarkSent(nowLed);
        }

        if (m_mutationQueue.DrainOne(m_pageManager))
        {
            MarkScreenDirty();
        }
    }

    void UpdateScreen()
    {
        m_field.display.Fill(0);

        for (size_t row = 0; row < 8; row++)
        {
            const char* name = m_pageManager.GetNameCurrentPage(row);
            float paramValue = m_pageManager.GetParamCurrentPageOrMod(row);
            uint8_t xName = 0;
            uint8_t xValue = 4 * 6 + 1;
            uint8_t yPos = row * 8;
            uint8_t xValueEnd = xValue + 72 * paramValue;
            uint8_t yValueEnd = yPos + 8;
            m_field.display.SetCursor(xName, yPos);
            m_field.display.WriteString(name, Font_6x8, true);

            m_field.display.DrawRect(xValue, yPos, xValueEnd, yValueEnd, true, true);

            m_field.display.SetCursor(xValue + 74, yPos);
            char buf[5];
            memset(buf, ' ', 5);
            if (m_pageManager.GetModIndex(row) != 255)
            {
                buf[0] = 'M';
                buf[1] = '1' + m_pageManager.GetModIndex(row);
            }

            buf[3] = m_pageManager.TrackingBadge(row);
            buf[4] = '\0';
            m_field.display.WriteString(buf, Font_6x8, true);
        }

        m_field.display.Update();
    }

    void Init(daisy::AudioHandle::AudioCallback process)
    {
        m_field.Init();

        daisy::System::Delay(100);

        m_field.display.Fill(0);
        m_field.display.Update();

        daisy::System::Delay(100);

        m_switchGuard.RunBootAudit(m_field);

        m_field.StartAdc();
        m_field.StartAudio(process);

        daisy::System::Delay(100);

        m_field.ProcessAllControls();
        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pageManager.m_knobPositions[i] = m_field.knob[i].Process();
        }

        m_gateTrigger.Reset(m_field.gate_in.State());

        m_pageManager.Finalize();
        m_screenGate.m_dirty = true;
        m_screenGate.m_lastMs = daisy::System::GetNow();
    }

    void MainLoop()
    {
        while (true)
        {
            ProcessControls();

            const uint32_t now = daisy::System::GetNow();
            if (m_screenGate.Due(now))
            {
                UpdateScreen();
                m_screenGate.MarkSent(now);
            }
        }
    }
};
