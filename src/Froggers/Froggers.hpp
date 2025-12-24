#pragma once

#include "../common/Include.hpp"
#include "../common/Comb.hpp"
#include "../common/PolynomialDrive.hpp"
#include "../common/EQ.hpp"
#include "../common/Marbles.hpp"

#include <tuple>
#include <cstdio>

using namespace daisy;

struct Froggers
{
    Page* m_filterParams;
    Page* m_driveParams;


    RuntimeParam m_eqLowGain;
    RuntimeParam m_eqLowMidGain;
    RuntimeParam m_eqHighMidGain;
    RuntimeParam m_eqHighGain;
    RuntimeParam m_comf;
    RuntimeParam m_comq;
    RuntimeParam m_cmlp;

    RuntimeParam m_srr1;
    RuntimeParam m_srr2;
    RuntimeParam m_mix;
    RuntimeParam m_digr;
    RuntimeParam m_hash;

    EQ m_eq;
    Comb m_comFilter;    

    FrogBlock m_frogBlock;

    Marbles m_marbles;

    float Alpha(float natFreq)
    {
        return 1.0f - std::exp(-2.0f * M_PI * natFreq);
    }

    void ReadParamsBlock()
    {
        // Convert 0-1 param to -24dB to +12dB using ExpParam
        // -24dB = 10^(-24/20) ≈ 0.0631, +12dB = 10^(12/20) ≈ 3.981
        //
        m_eqLowGain.SetTarget(PhaseUtils::ExpParam::Compute(0.0631f, 3.981f, m_filterParams->GetParam(0)));
        m_eqLowMidGain.SetTarget(PhaseUtils::ExpParam::Compute(0.0631f, 3.981f, m_filterParams->GetParam(1)));
        m_eqHighMidGain.SetTarget(PhaseUtils::ExpParam::Compute(0.0631f, 3.981f, m_filterParams->GetParam(2)));
        m_eqHighGain.SetTarget(PhaseUtils::ExpParam::Compute(0.0631f, 3.981f, m_filterParams->GetParam(3)));
        float comf = PhaseUtils::ExpParam::Compute(20 / 48000.0, 10000.0 / 48000.0, m_filterParams->GetParam(4));
        m_comf.SetTarget(comf);
        m_comq.SetTarget(Comb::GetFeedback(m_filterParams->GetParam(5)));
        float cmlp = PhaseUtils::ExpParam::Compute(20.0 / 48000.0, 20000.0 / 48000.0, m_filterParams->GetParam(6));
        m_cmlp.SetTarget(Alpha(cmlp));

        m_srr1.SetTarget(1e-2 + PhaseUtils::ZeroedExpParam::Compute(10.0,  1 - m_driveParams->GetParam(2)));
        m_srr2.SetTarget(1e-2 + PhaseUtils::ZeroedExpParam::Compute(10.0,  1 - m_driveParams->GetParam(3)));
        m_digr.SetTarget(m_driveParams->GetParam(4));
        m_hash.SetTarget(m_driveParams->GetParam(5));
        m_mix.SetTarget(m_driveParams->GetParam(6));

        m_frogBlock.m_polynomialDrive.SetGain(m_driveParams->GetParam(0));
        m_frogBlock.m_polynomialDrive.SetCoefs(m_driveParams->GetParam(1));
    }

    void UpdateParams()
    {
        m_eq.SetLowGain(m_eqLowGain.Process());
        m_eq.SetLowMidGain(m_eqLowMidGain.Process());
        m_eq.SetHighMidGain(m_eqHighMidGain.Process());
        m_eq.SetHighGain(m_eqHighGain.Process());
        m_comFilter.m_delaySamples = Comb::GetDelaySamples(m_comf.Process());
        m_comFilter.m_feedback = m_comq.Process();
        m_comFilter.SetCutoffAlpha(m_cmlp.Process());

        m_frogBlock.m_sampleRateReducer1.SetFreq(m_srr1.Process());
        m_frogBlock.m_sampleRateReducer2.SetFreq(m_srr2.Process());
        m_frogBlock.m_digitalReorganizer.SetFlip(m_digr.Process());
        m_frogBlock.m_digitalReorganizer.SetHash(m_hash.Process());
        m_frogBlock.m_mix = m_mix.Process();

        m_marbles.UpdateParams();
    }

    Froggers()
        : m_filterParams(nullptr)
    {
    }

    void Config(PageManager* pageManager)
    {
        m_filterParams = pageManager->AddPage();
        // Initial param value for flat EQ (0dB = 1.0 linear gain)
        // Solve: 1.0 = 0.0631 * (3.981/0.0631)^param
        // param ≈ 0.6667 (2/3)
        //
        float flatParam = 0.6667f;
        m_filterParams->InitParam("EQLW", 0, flatParam);
        m_filterParams->InitParam("EQLM", 1, flatParam);
        m_filterParams->InitParam("EQHM", 2, flatParam);
        m_filterParams->InitParam("EQHI", 3, flatParam);
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
        m_driveParams->InitParam("MIX", 6, 1.0f);

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

        output = m_comFilter.Process(output);
        output = m_eq.Process(output);

        return output;
    }
};


