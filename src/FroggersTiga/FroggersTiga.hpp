#pragma once

#include "../common/Include.hpp"
#include "../common/Comb.hpp"
#include "../common/PolynomialDrive.hpp"
#include "../common/ResonantBump.hpp"
#include "../common/Marbles.hpp"

#include <tuple>
#include <cstdio>
#include <algorithm>

using namespace daisy;

struct FroggersTiga
{
    ModMgr* m_modMgr;
    Page* m_audioGenParams;
    Page* m_reverbParams;
    Page* m_filterParams;
    Page* m_driveParams;

    RuntimeParam m_v1vo;
    RuntimeParam m_v2vo;
    RuntimeParam m_v3vo;
    RuntimeParam m_xcpl;
    RuntimeParam m_pm1;
    RuntimeParam m_pm2;
    RuntimeParam m_oscLvl;

    float m_ph1;
    float m_ph2;
    float m_ph3;
    uint8_t m_vco1Wave;
    uint8_t m_vco2Wave;
    float m_m5Hold;
    float m_m5Out;
    uint32_t m_m5Counter;

    RuntimeParam m_rvMix;
    RuntimeParam m_rvSize;
    RuntimeParam m_rvDecay;
    RuntimeParam m_rvPre;
    RuntimeParam m_rvDamp;
    RuntimeParam m_rvModDepth;
    RuntimeParam m_rvModRate;
    float m_rvLfoPhase;
    size_t m_rvIndexA;
    size_t m_rvIndexB;
    size_t m_rvPreIndex;
    static constexpr size_t x_rvSize = 4096;
    float m_rvLineA[x_rvSize];
    float m_rvLineB[x_rvSize];
    float m_rvPreLine[x_rvSize];
    OPLowPassFilter m_rvDampFilter;

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

    static float WrapPhase(float p)
    {
        return p - std::floor(p);
    }

    static float EvalWave(float phaseWrapped01, uint8_t wave)
    {
        if (wave == 0)
        {
            return SDDSine::Evaluate(phaseWrapped01);
        }
        if (wave == 1)
        {
            return 2.0f * phaseWrapped01 - 1.0f;
        }
        return (phaseWrapped01 < 0.5f) ? 1.0f : -1.0f;
    }

    float ExpMap(float min, float max, float value)
    {
        return PhaseUtils::ExpParam::Compute(min, max, value);
    }

    float ZeroedExp(float value)
    {
        return PhaseUtils::ZeroedExpParam::Compute(10.0f, value);
    }

    float CouplingMagnitude(float value)
    {
        float centered = 2.0f * value - 1.0f;
        return ZeroedExp(std::abs(centered));
    }

    void UpdateM5FromVco(float v1, float v2, float v3)
    {
        if (!m_modMgr)
        {
            return;
        }
        uint32_t count = m_m5Counter++;
        if ((count % 64u) == 0u)
        {
            float feature = std::fabs((v1 + v2 + v3) * (1.0f / 3.0f));
            m_m5Hold = feature;
        }
        float compressed = std::tanh(2.5f * m_m5Hold);
        m_m5Out += 0.03f * (compressed - m_m5Out);
        m_modMgr->m_mods[4] = std::min(std::max(m_m5Out, 0.0f), 1.0f);
    }

    float Alpha(float natFreq)
    {
        return 1.0f - std::exp(-2.0f * M_PI * natFreq);
    }

    void ReadParamsBlock()
    {
        constexpr float sr = 48000.0f;

        m_v1vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(0)));
        m_v2vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(1)));
        m_v3vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(2)));
        m_xcpl.SetTarget(m_audioGenParams->GetParam(3));
        m_pm1.SetTarget(ZeroedExp(m_audioGenParams->GetParam(4)));
        m_pm2.SetTarget(ZeroedExp(m_audioGenParams->GetParam(5)));
        m_oscLvl.SetTarget(ExpMap(0.01f, 1.0f, m_audioGenParams->GetParam(6)));

        m_rvMix.SetTarget(m_reverbParams->GetParam(0));
        m_rvSize.SetTarget(ExpMap(0.05f, 1.0f, m_reverbParams->GetParam(1)));
        m_rvDecay.SetTarget(ExpMap(0.1f, 0.98f, m_reverbParams->GetParam(2)));
        m_rvPre.SetTarget(ExpMap(1.0f / sr, 100.0f / sr, m_reverbParams->GetParam(3)));
        m_rvDamp.SetTarget(ExpMap(0.001f, 0.2f, 1.0f - m_reverbParams->GetParam(4)));
        m_rvModDepth.SetTarget(ExpMap(0.0f, 0.05f, m_reverbParams->GetParam(5)));
        m_rvModRate.SetTarget(ExpMap(0.05f / sr, 8.0f / sr, m_reverbParams->GetParam(6)));

        m_pureDelayFreq.SetTarget(PhaseUtils::ExpParam::Compute(20.0f / 48000.0f, 20000.0f / 48000.0f, m_filterParams->GetParam(0)));

        float bumpFreq = PhaseUtils::ExpParam::Compute(20.0f / 48000, 20000.0f / 48000, m_filterParams->GetParam(1));
        m_bumpFreq.SetTarget(bumpFreq);

        float resonanceKnob = m_filterParams->GetParam(2);
        float bumpGain = PhaseUtils::ExpParam::Compute(1.0f, 10.0f, resonanceKnob);
        m_bumpResonance.SetTarget(bumpGain);

        float bumpQ = PhaseUtils::ExpParam::Compute(0.1f, 10.0f, m_filterParams->GetParam(3));
        m_bumpWidth.SetTarget(bumpQ);

        float comf = PhaseUtils::ExpParam::Compute(20 / 48000.0, 10000.0 / 48000.0, m_filterParams->GetParam(4));
        m_comf.SetTarget(comf);
        m_comq.SetTarget(Comb::GetFeedback(m_filterParams->GetParam(5)));
        float cmlp = PhaseUtils::ExpParam::Compute(4 * comf, 20000.0 / 48000.0, m_filterParams->GetParam(6));
        m_cmlp.SetTarget(Alpha(cmlp));

        m_srr1.SetTarget(1e-2f + PhaseUtils::ZeroedExpParam::Compute(10.0f, 1 - m_driveParams->GetParam(2)));
        m_srr2.SetTarget(1e-2f + PhaseUtils::ZeroedExpParam::Compute(10.0f, 1 - m_driveParams->GetParam(3)));
        m_digr.SetTarget(m_driveParams->GetParam(4));
        m_hash.SetTarget(m_driveParams->GetParam(5));
        m_fuzz.SetTarget(m_driveParams->GetParam(6));

        m_frogBlock.m_polynomialDrive.SetGain(m_driveParams->GetParam(0));
        m_frogBlock.m_polynomialDrive.SetCoefs(m_driveParams->GetParam(1));
    }

    float ProcessReverb(float input)
    {
        float preNorm = m_rvPre.Process();
        size_t preDelay = static_cast<size_t>(std::round(preNorm * 48000.0f));
        if (preDelay >= x_rvSize)
        {
            preDelay = x_rvSize - 1;
        }
        m_rvPreLine[m_rvPreIndex] = input;
        size_t preRead = (m_rvPreIndex + x_rvSize - preDelay) % x_rvSize;
        float preOut = m_rvPreLine[preRead];
        m_rvPreIndex = (m_rvPreIndex + 1) % x_rvSize;

        float sizeNorm = m_rvSize.Process();
        size_t baseA = static_cast<size_t>(180.0f + sizeNorm * 1300.0f);
        size_t baseB = static_cast<size_t>(260.0f + sizeNorm * 1800.0f);
        float lfo = SDDSine::Evaluate(m_rvLfoPhase);
        float mod = m_rvModDepth.Process() * lfo;
        m_rvLfoPhase = WrapPhase(m_rvLfoPhase + m_rvModRate.Process());

        size_t dA = std::min(x_rvSize - 1, static_cast<size_t>(std::max(1.0f, static_cast<float>(baseA) * (1.0f + mod))));
        size_t dB = std::min(x_rvSize - 1, static_cast<size_t>(std::max(1.0f, static_cast<float>(baseB) * (1.0f - mod))));
        size_t readA = (m_rvIndexA + x_rvSize - dA) % x_rvSize;
        size_t readB = (m_rvIndexB + x_rvSize - dB) % x_rvSize;

        float fb = m_rvDecay.Process();
        float aIn = preOut + m_rvLineB[readB] * fb;
        float bIn = preOut + m_rvLineA[readA] * fb;
        float aOut = m_rvDampFilter.Process(m_rvLineA[readA]);
        float bOut = m_rvDampFilter.Process(m_rvLineB[readB]);

        m_rvLineA[m_rvIndexA] = aIn;
        m_rvLineB[m_rvIndexB] = bIn;
        m_rvIndexA = (m_rvIndexA + 1) % x_rvSize;
        m_rvIndexB = (m_rvIndexB + 1) % x_rvSize;
        return 0.5f * (aOut + bOut);
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
        m_rvDampFilter.m_alpha = m_rvDamp.Process();

        m_marbles.UpdateParams();
    }

    FroggersTiga()
        : m_modMgr(nullptr)
        , m_audioGenParams(nullptr)
        , m_reverbParams(nullptr)
        , m_filterParams(nullptr)
        , m_driveParams(nullptr)
        , m_ph1(0.0f)
        , m_ph2(0.0f)
        , m_ph3(0.0f)
        , m_vco1Wave(0)
        , m_vco2Wave(0)
        , m_m5Hold(0.0f)
        , m_m5Out(0.0f)
        , m_m5Counter(0)
        , m_rvLfoPhase(0.0f)
        , m_rvIndexA(0)
        , m_rvIndexB(0)
        , m_rvPreIndex(0)
        , m_rvLineA{0.0f}
        , m_rvLineB{0.0f}
        , m_rvPreLine{0.0f}
        , m_rvDampFilter()
    {
    }

    void Config(PageManager* pageManager)
    {
        m_modMgr = &pageManager->m_modMgr;
        m_audioGenParams = pageManager->AddPage();
        m_audioGenParams->InitParam("V1VO", 0, 0.35f);
        m_audioGenParams->InitParam("V2VO", 1, 0.4f);
        m_audioGenParams->InitParam("V3VO", 2, 0.45f);
        m_audioGenParams->InitParam("XCPL", 3, 0.5f);
        m_audioGenParams->InitParam("PM1A", 4, 0.0f);
        m_audioGenParams->InitParam("PM2A", 5, 0.0f);
        m_audioGenParams->InitParam("OLVL", 6, 0.4f);
        m_audioGenParams->SetFuegoization();

        Page* marblesPage = pageManager->AddPage();
        m_marbles.InitPage(pageManager, marblesPage);

        m_reverbParams = pageManager->AddPage();
        m_reverbParams->InitParam("RVMX", 0, 0.2f);
        m_reverbParams->InitParam("RSIZ", 1, 0.4f);
        m_reverbParams->InitParam("RDEC", 2, 0.5f);
        m_reverbParams->InitParam("RPRE", 3, 0.1f);
        m_reverbParams->InitParam("RDMP", 4, 0.6f);
        m_reverbParams->InitParam("RMOD", 5, 0.2f);
        m_reverbParams->InitParam("RRAT", 6, 0.2f);
        m_reverbParams->SetFuegoization();

        m_filterParams = pageManager->AddPage();
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
        else if (button == 1)
        {
            m_audioGenParams->m_parameters[3].m_knobValue = 0.5f * RGen().UniGen();
        }
        else if (button == 2)
        {
            m_audioGenParams->m_parameters[3].m_knobValue = 0.5f + 0.5f * RGen().UniGen();
        }
        else if (button == 3)
        {
            m_vco2Wave = static_cast<uint8_t>((m_vco2Wave + 1u) % 3u);
        }
        else if (button == 4)
        {
            m_vco1Wave = static_cast<uint8_t>((m_vco1Wave + 1u) % 3u);
        }
    }

    float StepOscillatorsAndMix()
    {
        float xc = m_xcpl.Process();
        float centered = 2.0f * xc - 1.0f;
        float c12 = (centered < 0.0f) ? CouplingMagnitude(xc) : 0.0f;
        float c23 = (0.0f < centered) ? CouplingMagnitude(xc) : 0.0f;

        float pm1d = m_pm1.Process();
        float pm2d = m_pm2.Process();
        float fuegKnob = m_audioGenParams->GetParam(7);
        float pm3d = ZeroedExp(fuegKnob);

        float f1 = m_v1vo.Process();
        float f2 = m_v2vo.Process();
        float f3 = m_v3vo.Process();

        float u1 = EvalWave(m_ph1, m_vco1Wave);
        float u2 = EvalWave(m_ph2, m_vco2Wave);
        float u3 = SDDSine::Evaluate(m_ph3);

        float pmOff1 = pm1d * c12 * u2;
        float pmOff2 = pm2d * (c12 * u1 + c23 * u3);
        float pmOff3 = pm3d * c23 * u2;

        float v1 = EvalWave(WrapPhase(m_ph1 + pmOff1), m_vco1Wave);
        float v2 = EvalWave(WrapPhase(m_ph2 + pmOff2), m_vco2Wave);
        float v3 = SDDSine::Evaluate(WrapPhase(m_ph3 + pmOff3));
        UpdateM5FromVco(v1, v2, v3);

        m_ph1 = WrapPhase(m_ph1 + f1);
        m_ph2 = WrapPhase(m_ph2 + f2);
        m_ph3 = WrapPhase(m_ph3 + f3);

        return m_oscLvl.Process() * ((v1 + v2 + v3) * (1.0f / 3.0f));
    }

    float ApplyOutputFx(float output)
    {
        output = m_pureDelay.Process(output);
        output = m_comFilter.Process(output);
        output = m_resonantBump.Process(output);
        float rvb = ProcessReverb(output);
        float rvMix = m_rvMix.Process();
        return (1.0f - rvMix) * output + rvMix * rvb;
    }

    float Process(float input)
    {
        UpdateParams();
        m_marbles.Process();

        float chainIn = input + StepOscillatorsAndMix();
        float output = m_frogBlock.Process(chainIn);
        return ApplyOutputFx(output);
    }
};
