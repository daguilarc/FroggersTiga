#pragma once

#include "../common/Include.hpp"
#include "../common/Comb.hpp"
#include "../common/PolynomialDrive.hpp"
#include "../common/ResonantBump.hpp"
#include "../common/Marbles.hpp"

#include <tuple>
#include <cstdio>

using namespace daisy;

struct Froggers
{
    Page* m_filterParams;
    Page* m_driveParams;


    RuntimeParam m_pureDelayFreq;
    RuntimeParam m_bumpFreq;
    RuntimeParam m_bumpResonance;
    RuntimeParam m_bumpWidth;
    RuntimeParam m_comf;
    RuntimeParam m_comq;
    RuntimeParam m_cmlp;

    RuntimeParam m_srr1;
    RuntimeParam m_srr2;
    RuntimeParam m_fuzz;
    RuntimeParam m_digr;
    RuntimeParam m_hash;

    ResonantBump m_resonantBump;
    Comb m_comFilter;    
    PureDelay m_pureDelay;

    FrogBlock m_frogBlock;

    Marbles m_marbles;

    float Alpha(float natFreq)
    {
        return 1.0f - std::exp(-2.0f * M_PI * natFreq);
    }

    void ReadParamsBlock()
    {
        m_pureDelayFreq.SetTarget(PhaseUtils::ExpParam::Compute(20.0f / 48000.0, 20000.0f / 48000.0, m_filterParams->GetParam(0)));

        // Resonant bump parameters
        // Frequency: 20Hz to 20000Hz
        //
        float bumpFreq = PhaseUtils::ExpParam::Compute(20.0f / 48000, 20000.0f / 48000, m_filterParams->GetParam(1));
        m_bumpFreq.SetTarget(bumpFreq);
        
        // Resonance: 0.0 = transparent (gain 1.0), 1.0 = +20dB boost (gain 10.0)
        // Controls height of the bump
        //
        float resonanceKnob = m_filterParams->GetParam(2);
        float bumpGain = PhaseUtils::ExpParam::Compute(1.0f, 10.0f, resonanceKnob);
        m_bumpResonance.SetTarget(bumpGain);
        
        // Width: 0.0 = wide (Q 0.1), 1.0 = narrow (Q 10.0)
        // Controls width/bandwidth of the bump
        //
        float bumpQ = PhaseUtils::ExpParam::Compute(0.1f, 10.0f, m_filterParams->GetParam(3));
        m_bumpWidth.SetTarget(bumpQ);
        
        float comf = PhaseUtils::ExpParam::Compute(20 / 48000.0, 10000.0 / 48000.0, m_filterParams->GetParam(4));
        m_comf.SetTarget(comf);
        m_comq.SetTarget(Comb::GetFeedback(m_filterParams->GetParam(5)));
        float cmlp = PhaseUtils::ExpParam::Compute(4 * comf, 20000.0 / 48000.0, m_filterParams->GetParam(6));
        m_cmlp.SetTarget(Alpha(cmlp));

        m_srr1.SetTarget(1e-2 + PhaseUtils::ZeroedExpParam::Compute(10.0,  1 - m_driveParams->GetParam(2)));
        m_srr2.SetTarget(1e-2 + PhaseUtils::ZeroedExpParam::Compute(10.0,  1 - m_driveParams->GetParam(3)));
        m_digr.SetTarget(m_driveParams->GetParam(4));
        m_hash.SetTarget(m_driveParams->GetParam(5));
        m_fuzz.SetTarget(m_driveParams->GetParam(6));

        m_frogBlock.m_polynomialDrive.SetGain(m_driveParams->GetParam(0));
        m_frogBlock.m_polynomialDrive.SetCoefs(m_driveParams->GetParam(1));
    }

    void UpdateParams()
    {
        m_pureDelay.SetDelaySamples(m_pureDelayFreq.Process());
        m_resonantBump.SetFreq(m_bumpFreq.Process());
        m_resonantBump.SetHeight(m_bumpResonance.Process());
        m_resonantBump.SetWidth(m_bumpWidth.Process());
        m_comFilter.m_delaySamples = Comb::GetDelaySamples(m_comf.Process());
        m_comFilter.m_feedback = m_comq.Process();
        m_comFilter.SetCutoffAlpha(m_cmlp.Process());

        m_frogBlock.m_sampleRateReducer1.SetFreq(m_srr1.Process());
        m_frogBlock.m_sampleRateReducer2.SetFreq(m_srr2.Process());
        m_frogBlock.m_digitalReorganizer.SetFlip(m_digr.Process());
        m_frogBlock.m_digitalReorganizer.SetHash(m_hash.Process());
        m_frogBlock.m_fuzz = m_fuzz.Process();

        m_marbles.UpdateParams();
    }

    Froggers()
        : m_filterParams(nullptr)
    {
    }

    void Config(PageManager* pageManager)
    {
        m_filterParams = pageManager->AddPage();
        // Resonant bump parameters
        // Frequency: default 1000Hz (param 0.5 maps to ~1000Hz in 20-20000 range)
        //
        m_filterParams->InitParam("DELF", 0, 0.5f);
        m_filterParams->InitParam("BUPF", 1, 0.5f);
        m_filterParams->InitParam("BUPR", 2, 0.0f);
        m_filterParams->InitParam("BUPW", 3, 0.5f);        
        m_filterParams->InitParam("COMF", 4, 0.5f);
        m_filterParams->InitParam("COMQ", 5, 0.5f);
        m_filterParams->InitParam("CMLP", 6, 1.0f);

        m_driveParams = pageManager->AddPage();
        m_driveParams->InitParam("GAIN", 0, 0.0f);
        m_driveParams->InitParam("SHAPE", 1, 0.0f);
        m_driveParams->InitParam("SRR1", 2, 0.0f);
        m_driveParams->InitParam("SRR2", 3, 0.0f);
        m_driveParams->InitParam("DIGR", 4, 0.0f);
        m_driveParams->InitParam("HASH", 5, 0.0f);
        m_driveParams->InitParam("FUZZ", 6, 0.0f);

        m_filterParams->SetFuegoization();
        m_driveParams->SetFuegoization();

        m_marbles.Config(pageManager);
    }

    void Process(AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out, size_t size)
    {
        ReadParamsBlock();
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = Process(in[0][i]);
            out[1][i] = 0;                 
        }
    }

    void ButtonCallback(int button)
    {
        if (button == 0)
        {
            m_marbles.Increment();
        }
    }

    float Process(float input)
    {    
        UpdateParams();
        m_marbles.Process();
        float output = m_frogBlock.Process(input);

        output = m_pureDelay.Process(output);
        output = m_comFilter.Process(output);
        output = m_resonantBump.Process(output);

        return output;
    }
};


