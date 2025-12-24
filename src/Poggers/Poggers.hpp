#include "../common/Include.hpp"
#include "../common/App.hpp"
#define IOS_BUILD
#include "../../External/theallelectricsmartgrid/private/src/Resynthesis.hpp"
#include <tuple>
#include <cstdio>

using namespace daisy;

struct Poggers
{
    Page* m_poggers;
    BasicWaveTable m_buffer;
    const WaveTable* m_cosTable;
    size_t m_index;

    Resynthesizer m_resynthesizer;
    Resynthesizer::Grain m_grain[4];
    size_t m_grainIndex;

    Poggers()
        : m_poggers(nullptr)
        , m_cosTable(&WaveTable::GetCosine())
        , m_index(0)
        , m_grainIndex(0)
    {
        for (size_t i = 0; i < 4; i++)
        {
            m_grain[i].m_windowTable = m_cosTable;
        }
    }

    void Config(PageManager* pageManager)
    {
        m_poggers = pageManager->AddPage();
        m_index = 0;
        m_cosTable = &WaveTable::GetCosine();
        for (size_t i = 0; i < 4; i++)
        {
            m_grain[i].m_windowTable = m_cosTable;
            m_grain[i].m_running = false;
        }
    }

    void Process(AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out, size_t size)
    {
        memset(out[0], 0, size * sizeof(float));
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = Process(in[0][i]);
            out[1][i] = 0;                 
        }
    }

    float Process(float input)
    {    
        m_buffer.m_table[m_index] = input;
        m_index = (m_index + 1) % BasicWaveTable::x_tableSize;

        if (m_index % Resynthesizer::GetGrainLaunchSamples() == 0)
        {
            for (size_t i = 0; i < BasicWaveTable::x_tableSize; i++)
            {
                size_t bufferIndex = (m_index + i) % BasicWaveTable::x_tableSize;
                m_grain[m_grainIndex].m_buffer.m_table[i] = (0.5f - 0.5f * m_cosTable->m_table[i]) * m_buffer.m_table[bufferIndex];
            }
            
            Resynthesizer::Input input;
            input.m_startTime = m_resynthesizer.m_startTime + Resynthesizer::GetGrainLaunchSamples();
            m_resynthesizer.StartGrain(&m_grain[m_grainIndex], input);
            m_grainIndex = (m_grainIndex + 1) % 4;
        }

        float result = 0;
        for (size_t i = 0; i < 4; i++)
        {
            if (m_grain[i].m_running)
            {
                result += m_grain[i].Process();
            }
        }

        return input;
    }
};