#pragma once

#include "Page.hpp"
#include "SchmidtTrigger.hpp"
#include "daisy_field.h"
#include "daisysp.h"
#include <functional>

struct DaisyIO
{
    PageManager m_pageManager;
    daisy::DaisyField m_field;
    std::function<void(int)> m_buttonCallback;
    SchmidtTrigger m_gateTrigger{0.2f, 0.1f};

    void ProcessControls()
    {
        m_field.ProcessAllControls();

        if (m_pageManager.m_modIndex == 255)
        {
            if (m_field.sw[0].RisingEdge())
            {
                m_pageManager.PagePrevious();
            }
            
            if (m_field.sw[1].RisingEdge())
            {
                m_pageManager.PageNext();
            }

            if (m_field.KeyboardRisingEdge(0))
            {
                m_pageManager.RandomizeCurrentPage();
            }

            if (m_field.KeyboardRisingEdge(1))
            {
                m_pageManager.RandomizeAllPages();
            }

            if (m_field.KeyboardRisingEdge(2))
            {
                m_pageManager.RandomizeCurrentPageMod();
            }

            if (m_field.KeyboardRisingEdge(3))
            {
                m_pageManager.RandomizeAllPagesMod();
            }
        }

        for (size_t i = 0; i < 4; i++)
        {
            if (m_field.KeyboardRisingEdge(i + 4))
            {
                m_buttonCallback(i);
            }
        }

        // Check analog gate input for rising edge
        //
        if (m_gateTrigger.Process(m_field.gate_in.State()))
        {
            m_buttonCallback(0);
        }

        m_field.seed.SetLed(m_field.gate_in.State() ? 1.0f : 0.0f);

        m_field.SetCvOut1(m_pageManager.m_modMgr.m_mods[4] * 4096);
        m_field.SetCvOut2(m_pageManager.m_modMgr.m_mods[5] * 4096);

        for (size_t i = 0; i < ModMgr::x_numMods; i++)
        {
            if (i < 4)
            {
                m_pageManager.m_modMgr.m_mods[i] = m_field.GetCvValue(i);
            }

            if (m_field.KeyboardRisingEdge(i + 8))
            {
                m_pageManager.StartModTracking(i);
            }
            else if (m_field.KeyboardFallingEdge(i + 8))
            {
                m_pageManager.StopModTracking();
            }

            m_field.led_driver.SetLed(i, m_pageManager.m_modIndex == i ? 1.0f : 0.0f);
        }

        for (size_t i = 0; i < Parameter::x_numParameters; i++)
        {
            m_pageManager.KnobUpdate(i, m_field.knob[i].Process());
            m_field.led_driver.SetLed(i + 16, m_pageManager.IsTracking(i) ? 1.0f : 0.0f);
        }
        
        m_field.led_driver.SwapBuffersAndTransmit();
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
    }

    void MainLoop()
    {
        while (true)
        {
            ProcessControls();
            UpdateScreen();
        }
    }
};