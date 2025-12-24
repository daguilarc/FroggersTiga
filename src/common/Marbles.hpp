#pragma once

#include "Page.hpp"
#include "SmartGridInclude.hpp"

struct Marbles
{
    static constexpr size_t x_numMarbles = 8;
    float m_marbles[2][x_numMarbles];
    RGen m_rgen;
    OPLowPassFilter m_filter[2];
    uint8_t m_size[2];
    uint8_t m_index[2];
    float m_dejaVuKnob[2];
    float m_probability;
    float* m_output[2];

    Page* m_page;

    void Config(PageManager* pageManager)
    {
        m_output[0] = &pageManager->m_modMgr.m_mods[4];
        m_output[1] = &pageManager->m_modMgr.m_mods[5];

        m_page = pageManager->AddPage();
        m_page->InitParam("PROB", 0, 1.0);
        m_page->InitParam("DJV1", 1, 0.5f);
        m_page->InitParam("SZ1", 2, 1);
        m_page->InitParam("SLW1", 3, 0.0);
        m_page->InitParam("DJV2", 4, 0.5f);
        m_page->InitParam("SZ2", 5, 1);
        m_page->InitParam("SLW2", 6, 0.0);
        m_page->SetFuegoization();
    }

    void UpdateParams()
    {
        m_probability = m_page->GetParam(0);
        m_dejaVuKnob[0] = m_page->GetParam(1);
        m_size[0] = 2 + std::round(m_page->GetParam(2) * (x_numMarbles - 2));
        m_filter[0].SetAlphaFromNatFreq(PhaseUtils::ExpParam::Compute(0.05 / 48000.0, 10000 / 48000.0, 1 - m_page->GetParam(3)));
        m_dejaVuKnob[1] = m_page->GetParam(4);
        m_size[1] = 2 + std::round(m_page->GetParam(5) * (x_numMarbles - 2));
        m_filter[1].SetAlphaFromNatFreq(PhaseUtils::ExpParam::Compute(0.05 / 48000.0, 10000 / 48000.0, 1 - m_page->GetParam(6)));
    }

    void Increment()
    {
        for (size_t i = 0; i < 2; i++)
        {
            if (m_probability < m_rgen.UniGen())
            {
                continue;
            }

            if (0.5 < m_dejaVuKnob[i])
            {
                if (m_rgen.UniGen() < 2 * (m_dejaVuKnob[i] - 0.5f))
                {
                    m_index[i] = m_rgen.RangeGen(m_size[i]);                    
                }
                else
                {
                    m_index[i] = (m_index[i] + 1) % m_size[i];
                }
            }
            else
            {
                m_index[i] = (m_index[i] + 1) % m_size[i];
                if (m_rgen.UniGen() < 2 * (0.5f - m_dejaVuKnob[i]))
                {
                    m_marbles[i][m_index[i]] = m_rgen.UniGenRange(0, 1.0f);
                }
            }
        }
    }

    Marbles()
    {
        for (size_t i = 0; i < 2; i++)
        {
            for (size_t j = 0; j < x_numMarbles; j++)
            {
                m_marbles[i][j] = m_rgen.UniGenRange(0, 1.0f);                
            }

            m_size[i] = x_numMarbles;
            m_index[i] = 0;
            m_dejaVuKnob[i] = 0.5f;
            m_probability = 0.5f;
            m_output[i] = &m_marbles[i][0];
        }
    }
 
    void Process()
    {
        for (size_t i = 0; i < 2; i++)
        {
            *m_output[i] = m_filter[i].Process(m_marbles[i][m_index[i]]);
        }
    }
};