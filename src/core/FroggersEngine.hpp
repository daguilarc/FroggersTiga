#pragma once

#include "Comb.hpp"
#include "Marbles.hpp"
#include "Page.hpp"
#include "PairArEnvelope.hpp"
#include "AudioPairArState.hpp"
#include "PolynomialDrive.hpp"
#include "ResonantBump.hpp"
#include "RGen.hpp"
#include "SchmidtTrigger.hpp"
#include "SDDSine.hpp"
#include "TanhSaturator.hpp"
#include "VcoAdsrState.hpp"
#include "VcoWaveEval.hpp"
#include "VcoWaveMorph.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <tuple>

using SimFxInsertFn = float (*)(float bumpIn, void* ctx);

struct FroggersEngine
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
    RuntimeParam m_pm3;
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
    RuntimeParam m_rvWidth;
    RuntimeParam m_rvDiffusion;
    float m_reverbWetL = 0.0f;
    float m_reverbWetR = 0.0f;
    float m_lastRvMix = 0.0f;
    size_t m_rvIndexA;
    size_t m_rvIndexB;
    size_t m_rvPreIndex;
    static constexpr size_t x_rvSize = 4096;
    float m_rvLineA[x_rvSize];
    float m_rvLineB[x_rvSize];
    float m_rvPreLine[x_rvSize];
    OPLowPassFilter m_rvDampFilter;

    RuntimeParam m_pureDelaySeconds;
    RuntimeParam m_bumpFreq;
    RuntimeParam m_bumpResonance;
    RuntimeParam m_bumpWidth;
    RuntimeParam m_comf;
    RuntimeParam m_comq;
    RuntimeParam m_cmlp;
    RuntimeParam m_filterCombPeak;
    RuntimeParam m_filterScoop;

    RuntimeParam m_srr1;
    RuntimeParam m_srr2;
    RuntimeParam m_fuzz;
    RuntimeParam m_digr;
    RuntimeParam m_hash;

    ResonantBump m_resonantBump;
    ResonantBump m_scoopNotch;
    Comb m_comFilter;
    PureDelay m_pureDelay;

    FrogBlock m_frogBlock;

    Marbles m_marbles;

    OPLowPassFilter m_extEnvFilter;
    SchmidtTrigger m_extGate;
    TanhSaturator<true> m_extInputLimiter;
    static constexpr float x_extInputLimiterDrive = 5.0f;

    float m_sampleRate = 44100.0f;
    bool m_simWaveMorph = false;
    bool m_simDedicatedPm3Knob = false;
    VcoWaveMorph m_vcoMorph[3];
    float m_envelopeLevel = 0.0f;
    SimFxInsertFn m_simFxInsert = nullptr;
    void* m_simFxInsertCtx = nullptr;
    AudioPairArState* m_pairAr = nullptr;
    VcoAdsrState* m_vcoAdsr = nullptr;
    Page* m_adsrParams = nullptr;
    PairArEnvelope m_pair12;
    PairArEnvelope m_pair23;
    bool m_useV2FilterParallel = false;

    // D11/D12/D14 (task 7.4): V2-hosts-only (desktop-v2 + web) flag. Daisy/v1
    // never set this (default false), so their StepOscillators path is the
    // untouched, coupler-gated legacy branch below. When true: no XCPL
    // coupler terms at all (c12/c23 dropped entirely); each VCO's phase is
    // instead modulated by its own dedicated sine LFO running at a frequency
    // derived from that VCO's PM knob value, with a fixed modulation index
    // (see x_pmLfoDepth -- an implementer default, flagged for operator
    // tuning). No self-feedback, no cross-VCO terms.
    bool m_simIndependentPm = false;
    float m_pmLfoPh1 = 0.0f;
    float m_pmLfoPh2 = 0.0f;
    float m_pmLfoPh3 = 0.0f;

    // Implementer defaults for the self-contained PM sine LFO (D14 "resolved"
    // note: "phase-mod index/depth is an implementer default, flag it for
    // later tuning"). PM knob value (0..1, already ZeroedExp-curved by
    // ReadParamsBlock) maps exponentially across this Hz range; the sine's
    // peak phase excursion is x_pmLfoDepth cycles. FLAGGED FOR OPERATOR
    // TUNING -- untested against the musical target, chosen only to make the
    // knob audibly effective end-to-end.
    static constexpr float x_pmLfoMinHz = 0.05f;
    static constexpr float x_pmLfoMaxHz = 20.0f;
    static constexpr float x_pmLfoDepth = 0.15f;

    // D14 zero-off (operator 2026-07-23, "the knob's minimum position = PM
    // fully OFF"): below/at x_pmLfoFloor the phase-mod depth is exactly zero
    // (no modulation applied at all, independent of the LFO frequency that
    // knob value would otherwise map to). From the floor up to
    // x_pmLfoFloor + x_pmLfoRampWidth, depth ramps 0 -> 1 (smoothstep, for a
    // click-free transition into audible PM); above that it is the normal
    // fixed x_pmLfoDepth. FLAGGED FOR OPERATOR TUNING, same as the Hz/depth
    // constants above -- chosen only to give the knob a real off position.
    static constexpr float x_pmLfoFloor = 0.02f;
    static constexpr float x_pmLfoRampWidth = 0.08f;

    // Returns 0 at/below x_pmLfoFloor, 1 at/above x_pmLfoFloor +
    // x_pmLfoRampWidth, smoothstep-interpolated between.
    static float PmDepthScale(float pmKnobValue)
    {
        if (pmKnobValue <= x_pmLfoFloor)
        {
            return 0.0f;
        }
        const float rampTop = x_pmLfoFloor + x_pmLfoRampWidth;
        if (pmKnobValue >= rampTop)
        {
            return 1.0f;
        }
        const float t = (pmKnobValue - x_pmLfoFloor) / x_pmLfoRampWidth;
        return t * t * (3.0f - 2.0f * t);
    }

    struct V2ModTapHooks
    {
        void (*processOsc)(float v1, float v2, float v3, void* ctx) = nullptr;
        void (*syncMarbles)(float marbles1, float marbles2, void* ctx) = nullptr;
        void* ctx = nullptr;
    };

    V2ModTapHooks m_v2ModTapHooks{};
    bool m_v2ModTapLayout = false;

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

    static float EvalWaveMorph(float phaseWrapped01, float morph)
    {
        return ::EvalWaveMorph(phaseWrapped01, morph);
    }

    void ApplySmoothingRates()
    {
        RuntimeParam* params[] = {
            &m_v1vo, &m_v2vo, &m_v3vo, &m_xcpl, &m_pm1, &m_pm2, &m_pm3, &m_oscLvl,
            &m_rvMix, &m_rvSize, &m_rvDecay, &m_rvPre, &m_rvDamp, &m_rvWidth, &m_rvDiffusion,
            &m_pureDelaySeconds, &m_bumpFreq, &m_bumpResonance, &m_bumpWidth,
            &m_comf, &m_comq, &m_cmlp, &m_filterCombPeak, &m_filterScoop,
            &m_srr1, &m_srr2, &m_fuzz, &m_digr, &m_hash,
        };
        for (RuntimeParam* param : params)
        {
            param->SetSmoothingRate(m_sampleRate);
        }
    }

    void SetSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate;
        m_extEnvFilter.SetAlphaFromNatFreq(5.0f / m_sampleRate);
        m_extInputLimiter.SetInputGain(x_extInputLimiterDrive);
        m_marbles.SetSampleRate(m_sampleRate);
        ApplySmoothingRates();
    }

    void SetSimWaveMorph(bool enabled)
    {
        m_simWaveMorph = enabled;
    }

    void SetSimDedicatedPm3Knob(bool enabled)
    {
        m_simDedicatedPm3Knob = enabled;
    }

    void SetSimFxInsert(SimFxInsertFn fn, void* ctx)
    {
        m_simFxInsert = fn;
        m_simFxInsertCtx = ctx;
    }

    void SetAudioPairArState(AudioPairArState* state)
    {
        m_pairAr = state;
        m_pair12.Reset();
        m_pair23.Reset();
    }

    void SetVcoAdsrState(VcoAdsrState* state, Page* adsrPage)
    {
        m_vcoAdsr = state;
        m_adsrParams = adsrPage;
    }

    // Test-only accessor (task 7.5 prerequisite fix): reads back the ADSR
    // page currently wired via SetVcoAdsrState, so a unit test can drive a
    // host-param write to the ADSR page and confirm the engine's m_adsrParams
    // pointer actually targets that same PageManager page. Returns -1.0f if
    // no page is wired (matches Daisy/v1, where m_adsrParams stays null).
    float GetAdsrParamForTest(uint8_t position) const
    {
        return m_adsrParams ? m_adsrParams->GetParam(position) : -1.0f;
    }

    void SetUseV2FilterParallel(bool enabled)
    {
        m_useV2FilterParallel = enabled;
    }

    void SetSimIndependentPm(bool enabled)
    {
        m_simIndependentPm = enabled;
    }

    void SetV2ModTapLayout(bool enabled, V2ModTapHooks hooks)
    {
        m_v2ModTapLayout = enabled;
        m_v2ModTapHooks = hooks;
    }

    float GetEnvelopeLevel() const
    {
        return m_envelopeLevel;
    }

    void SetVcoMorph(size_t index, float value)
    {
        if (index < 3)
        {
            m_vcoMorph[index].SetKnob(value);
        }
    }

    void SetVcoMorphMod(size_t index, uint8_t modIndex, float modAmount)
    {
        if (index < 3)
        {
            m_vcoMorph[index].SetMod(modIndex, modAmount);
        }
    }

    float GetVcoMorph(size_t index) const
    {
        if (index < 3)
        {
            return m_vcoMorph[index].m_knobValue;
        }
        return 0.0f;
    }

    void RandomizeVcoMorphs()
    {
        RGen rgen;
        for (size_t i = 0; i < 3; i++)
        {
            m_vcoMorph[i].SetKnob(rgen.UniGenRange(0.0f, 1.0f));
        }
    }

    void NudgeVcoMorph(size_t index, float delta)
    {
        if (index < 3)
        {
            float v = m_vcoMorph[index].m_knobValue + delta;
            if (v < 0.0f)
            {
                v = 0.0f;
            }
            if (v > 1.0f)
            {
                v = 1.0f;
            }
            m_vcoMorph[index].SetKnob(v);
        }
    }

    void CycleVcoMorph(size_t index)
    {
        if (index >= 3)
        {
            return;
        }
        const float v = m_vcoMorph[index].m_knobValue;
        float next = 0.0f;
        if (v < 0.25f)
        {
            next = 0.5f;
        }
        else if (v < 0.75f)
        {
            next = 1.0f;
        }
        m_vcoMorph[index].SetKnob(next);
    }

    float GetVcoDisplayMorph(size_t index) const
    {
        return ModulatedMorph(index);
    }

    float ModulatedMorph(size_t index) const
    {
        if (index < 3)
        {
            const float morph = m_vcoMorph[index].GetMorph(m_modMgr);
            if (!std::isfinite(morph))
            {
                return 0.0f;
            }
            return morph;
        }
        return 0.0f;
    }

    void SoftResetFxState()
    {
        for (size_t i = 0; i < x_rvSize; i++)
        {
            m_rvLineA[i] = 0.0f;
            m_rvLineB[i] = 0.0f;
            m_rvPreLine[i] = 0.0f;
        }
        m_rvIndexA = 0;
        m_rvIndexB = 0;
        m_rvPreIndex = 0;
        m_reverbWetL = 0.0f;
        m_reverbWetR = 0.0f;
        m_rvDampFilter = OPLowPassFilter();
        m_comFilter.Reset();
        for (size_t i = 0; i < 3; i++)
        {
            if (!std::isfinite(m_vcoMorph[i].m_knobValue))
            {
                m_vcoMorph[i].SetKnob(0.0f);
            }
        }
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
        const float sr = m_sampleRate;

        m_v1vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(0)));
        m_v2vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(1)));
        m_v3vo.SetTarget(ExpMap(20.0f / sr, 20000.0f / sr, m_audioGenParams->GetParam(2)));
        m_xcpl.SetTarget(m_audioGenParams->GetParam(3));
        m_pm1.SetTarget(ZeroedExp(m_audioGenParams->GetParam(4)));
        m_pm2.SetTarget(ZeroedExp(m_audioGenParams->GetParam(5)));
        if (m_simDedicatedPm3Knob)
        {
            m_pm3.SetTarget(ZeroedExp(m_audioGenParams->GetParam(6)));
            m_oscLvl.SetTarget(ExpMap(0.01f, 1.0f, 0.4f));
        }
        else
        {
            m_oscLvl.SetTarget(ExpMap(0.01f, 1.0f, m_audioGenParams->GetParam(6)));
        }

        m_rvMix.SetTarget(m_reverbParams->GetParam(0));
        m_rvSize.SetTarget(ExpMap(0.05f, 1.0f, m_reverbParams->GetParam(1)));
        m_rvDecay.SetTarget(ExpMap(0.1f, 0.98f, m_reverbParams->GetParam(2)));
        m_rvPre.SetTarget(ExpMap(1.0f / sr, 100.0f / sr, m_reverbParams->GetParam(3)));
        m_rvDamp.SetTarget(ExpMap(0.001f, 0.2f, 1.0f - m_reverbParams->GetParam(4)));
        m_rvWidth.SetTarget(m_reverbParams->GetParam(5));
        m_rvDiffusion.SetTarget(m_reverbParams->GetParam(6));

        m_pureDelaySeconds.SetTarget(PhaseUtils::ExpParam::Compute(0.001f, 0.1f, m_filterParams->GetParam(0)));

        float bumpFreq = PhaseUtils::ExpParam::Compute(20.0f / sr, 20000.0f / sr, m_filterParams->GetParam(1));
        m_bumpFreq.SetTarget(bumpFreq);

        float resonanceKnob = m_filterParams->GetParam(2);
        float bumpGain = PhaseUtils::ExpParam::Compute(1.0f, 10.0f, resonanceKnob);
        m_bumpResonance.SetTarget(bumpGain);

        float bumpQ = PhaseUtils::ExpParam::Compute(0.1f, 10.0f, m_filterParams->GetParam(3));
        m_bumpWidth.SetTarget(bumpQ);

        float comf = PhaseUtils::ExpParam::Compute(20.0f / sr, 10000.0f / sr, m_filterParams->GetParam(4));
        m_comf.SetTarget(comf);
        m_comq.SetTarget(Comb::GetFeedback(m_filterParams->GetParam(5)));
        float cmlp = PhaseUtils::ExpParam::Compute(4.0f * comf, 20000.0f / sr, m_filterParams->GetParam(6));
        m_cmlp.SetTarget(Alpha(cmlp));
        m_filterCombPeak.SetTarget(m_filterParams->GetParam(7));
        m_filterScoop.SetTarget(m_filterParams->GetParam(8));

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
        size_t preDelay = static_cast<size_t>(std::round(preNorm * m_sampleRate));
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
        size_t dA = std::min(x_rvSize - 1, std::max(static_cast<size_t>(1), baseA));
        size_t dB = std::min(x_rvSize - 1, std::max(static_cast<size_t>(1), baseB));
        size_t readA = (m_rvIndexA + x_rvSize - dA) % x_rvSize;
        size_t readB = (m_rvIndexB + x_rvSize - dB) % x_rvSize;

        const float valA = m_rvLineA[readA];
        const float valB = m_rvLineB[readB];
        const float fb = m_rvDecay.Process();
        const float diffusion = m_rvDiffusion.Process();
        const float cross = diffusion * 0.5f;
        const float aFb = valB * (1.0f - cross) + valA * cross;
        const float bFb = valA * (1.0f - cross) + valB * cross;
        const float aIn = preOut + aFb * fb;
        const float bIn = preOut + bFb * fb;
        const float aOut = m_rvDampFilter.Process(valA);
        const float bOut = m_rvDampFilter.Process(valB);

        m_rvLineA[m_rvIndexA] = aIn;
        m_rvLineB[m_rvIndexB] = bIn;
        m_rvIndexA = (m_rvIndexA + 1) % x_rvSize;
        m_rvIndexB = (m_rvIndexB + 1) % x_rvSize;

        const float mid = 0.5f * (aOut + bOut);
        const float width = m_rvWidth.Process();
        m_reverbWetL = mid + width * (aOut - mid);
        m_reverbWetR = mid + width * (bOut - mid);
        return 0.5f * (m_reverbWetL + m_reverbWetR);
    }

    float getReverbStereoDeltaL() const
    {
        const float mono = 0.5f * (m_reverbWetL + m_reverbWetR);
        return m_reverbWetL - mono;
    }

    float getReverbStereoDeltaR() const
    {
        const float mono = 0.5f * (m_reverbWetL + m_reverbWetR);
        return m_reverbWetR - mono;
    }

    float getLastRvMix() const
    {
        return m_lastRvMix;
    }

    void UpdateParams()
    {
        m_pureDelay.SetDelaySeconds(m_pureDelaySeconds.Process(), m_sampleRate);
        m_resonantBump.SetFreq(m_bumpFreq.Process());
        m_resonantBump.SetHeight(m_bumpResonance.Process());
        m_resonantBump.SetWidth(m_bumpWidth.Process());
        m_scoopNotch.SetFreq(m_bumpFreq.Process());
        m_scoopNotch.SetWidth(m_bumpWidth.Process());
        const float scoop = m_filterScoop.Process();
        m_scoopNotch.SetHeight(std::max(0.05f, 1.0f - 0.95f * scoop));
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

    FroggersEngine()
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
        , m_rvIndexA(0)
        , m_rvIndexB(0)
        , m_rvPreIndex(0)
        , m_rvLineA{0.0f}
        , m_rvLineB{0.0f}
        , m_rvPreLine{0.0f}
        , m_rvDampFilter()
        , m_extGate(0.01f, 0.005f)
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
        SetSampleRate(m_sampleRate);

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

    void ProcessBlock(const float* in, float* out, size_t n)
    {
        ReadParamsBlock();
        for (size_t i = 0; i < n; i++)
        {
            out[i] = ProcessSample(in[i]);
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
            if (m_simWaveMorph)
            {
                NudgeVcoMorph(1, 0.1f);
            }
            else
            {
                m_vco2Wave = static_cast<uint8_t>((m_vco2Wave + 1u) % 3u);
            }
        }
        else if (button == 4)
        {
            if (m_simWaveMorph)
            {
                NudgeVcoMorph(0, 0.1f);
            }
            else
            {
                m_vco1Wave = static_cast<uint8_t>((m_vco1Wave + 1u) % 3u);
            }
        }
    }

    // D14 self-contained PM (V2-only, flag-on): advances one VCO's dedicated
    // PM LFO by one sample and returns its current (pre-advance) sine value.
    // pmKnobValue (0..1, already ZeroedExp-curved) maps exponentially to a
    // frequency in [x_pmLfoMinHz, x_pmLfoMaxHz]; the LFO phase is completely
    // independent of the VCO's own phase (no self-feedback, no cross-VCO
    // terms).
    float StepIndependentPmLfo(float& lfoPhase, float pmKnobValue)
    {
        const float hz = x_pmLfoMinHz * std::pow(x_pmLfoMaxHz / x_pmLfoMinHz, pmKnobValue);
        const float lfoValue = SDDSine::Evaluate(lfoPhase);
        lfoPhase = WrapPhase(lfoPhase + hz / m_sampleRate);
        return lfoValue;
    }

    std::tuple<float, float, float> StepOscillators(float fuegKnob)
    {
        float pm1d = m_pm1.Process();
        float pm2d = m_pm2.Process();
        float pm3d = m_simDedicatedPm3Knob ? m_pm3.Process() : ZeroedExp(fuegKnob);

        float f1 = m_v1vo.Process();
        float f2 = m_v2vo.Process();
        float f3 = m_v3vo.Process();

        float morph1 = ModulatedMorph(0);
        float morph2 = ModulatedMorph(1);
        float morph3 = ModulatedMorph(2);

        float u1 = m_simWaveMorph ? EvalWaveMorph(m_ph1, morph1) : EvalWave(m_ph1, m_vco1Wave);
        float u2 = m_simWaveMorph ? EvalWaveMorph(m_ph2, morph2) : EvalWave(m_ph2, m_vco2Wave);
        float u3 = m_simWaveMorph ? EvalWaveMorph(m_ph3, morph3) : SDDSine::Evaluate(m_ph3);

        float pmOff1;
        float pmOff2;
        float pmOff3;
        if (m_simIndependentPm)
        {
            // D11/D12/D14 (V2 hosts only): no coupler, zero cross-VCO terms.
            // Each VCO's phase is modulated by its own dedicated sine LFO,
            // gated to exactly zero depth at/below the knob's zero-off floor
            // (PmDepthScale) so the knob's minimum position is truly inert.
            pmOff1 = x_pmLfoDepth * PmDepthScale(pm1d) * StepIndependentPmLfo(m_pmLfoPh1, pm1d);
            pmOff2 = x_pmLfoDepth * PmDepthScale(pm2d) * StepIndependentPmLfo(m_pmLfoPh2, pm2d);
            pmOff3 = x_pmLfoDepth * PmDepthScale(pm3d) * StepIndependentPmLfo(m_pmLfoPh3, pm3d);
        }
        else
        {
            // Legacy Daisy/v1 path (D14 flag-off) -- byte-for-byte unchanged.
            float xc = m_xcpl.Process();
            float centered = 2.0f * xc - 1.0f;
            float c12 = (centered < 0.0f) ? CouplingMagnitude(xc) : 0.0f;
            float c23 = (0.0f < centered) ? CouplingMagnitude(xc) : 0.0f;
            pmOff1 = pm1d * c12 * u2;
            pmOff2 = pm2d * (c12 * u1 + c23 * u3);
            pmOff3 = pm3d * c23 * u2;
        }

        float ph1 = WrapPhase(m_ph1 + pmOff1);
        float ph2 = WrapPhase(m_ph2 + pmOff2);
        float ph3 = WrapPhase(m_ph3 + pmOff3);
        float v1 = m_simWaveMorph ? EvalWaveMorph(ph1, morph1) : EvalWave(ph1, m_vco1Wave);
        float v2 = m_simWaveMorph ? EvalWaveMorph(ph2, morph2) : EvalWave(ph2, m_vco2Wave);
        float v3 = m_simWaveMorph ? EvalWaveMorph(ph3, morph3) : SDDSine::Evaluate(ph3);
        UpdateM5FromVco(v1, v2, v3);

        m_ph1 = WrapPhase(m_ph1 + f1);
        m_ph2 = WrapPhase(m_ph2 + f2);
        m_ph3 = WrapPhase(m_ph3 + f3);

        return {v1, v2, v3};
    }

    float MixOscVoices(float v1, float v2, float v3)
    {
        if (m_vcoAdsr && m_adsrParams)
        {
            // Task 7.5 (D15): per-VCO triplet row order (Attack, Sustain,
            // Release) x3, matching V2EngineSetup::configureAdsrPage's
            // InitParam layout below.
            v1 = m_vcoAdsr->apply(
                0, v1, m_adsrParams->GetParam(0), m_adsrParams->GetParam(1), m_adsrParams->GetParam(2));
            v2 = m_vcoAdsr->apply(
                1, v2, m_adsrParams->GetParam(3), m_adsrParams->GetParam(4), m_adsrParams->GetParam(5));
            v3 = m_vcoAdsr->apply(
                2, v3, m_adsrParams->GetParam(6), m_adsrParams->GetParam(7), m_adsrParams->GetParam(8));
        }
        if (!m_pairAr)
        {
            return (v1 + v2 + v3) * (1.0f / 3.0f);
        }

        m_pairAr->tickSmoothers();
        const float sum12 = v1 + v2;
        const float sum23 = v2 + v3;
        const float target12 = std::fabs(sum12) * 0.5f;
        const float target23 = std::fabs(sum23) * 0.5f;
        const float e12 = m_pair12.Step(
            target12,
            m_pairAr->getEffectiveSmoothed(0),
            m_pairAr->getEffectiveSmoothed(1),
            m_sampleRate);
        const float e23 = m_pair23.Step(
            target23,
            m_pairAr->getEffectiveSmoothed(2),
            m_pairAr->getEffectiveSmoothed(3),
            m_sampleRate);
        const float p12 = std::copysign(e12, sum12 == 0.0f ? 1.0f : sum12);
        const float p23 = std::copysign(e23, sum23 == 0.0f ? 1.0f : sum23);
        return (p12 + v2 + p23) * (1.0f / 3.0f);
    }

    float MixExternalAndOsc(float input, float v1, float v2, float v3, float olvl, bool hasExternal)
    {
        float oscMix = MixOscVoices(v1, v2, v3);
        if (!hasExternal)
        {
            return olvl * oscMix;
        }

        return (input * v1 + input * v2 + input * v3) * (1.0f / 3.0f);
    }

    float ApplyOutputFx(float output)
    {
        if (m_useV2FilterParallel)
        {
            const float combPath = m_comFilter.Process(m_pureDelay.Process(output));
            const float peakPath = m_resonantBump.Process(output);
            const float blend = m_filterCombPeak.Process();
            const float mixed = peakPath * (1.0f - blend) + combPath * blend;
            const float scoopMix = m_filterScoop.Process();
            const float scooped = m_scoopNotch.Process(mixed);
            output = mixed * (1.0f - scoopMix) + scooped * scoopMix;
        }
        else
        {
            output = m_pureDelay.Process(output);
            output = m_comFilter.Process(output);
            output = m_resonantBump.Process(output);
        }
        if (m_simFxInsert)
        {
            output = m_simFxInsert(output, m_simFxInsertCtx);
        }
        const float rvb = ProcessReverb(output);
        const float rvMix = m_rvMix.Process();
        m_lastRvMix = rvMix;
        return (1.0f - rvMix) * output + rvMix * rvb;
    }

    float ProcessSample(float input)
    {
        UpdateParams();
        m_marbles.Process();
        if (m_v2ModTapLayout && m_v2ModTapHooks.syncMarbles && m_modMgr)
        {
            m_v2ModTapHooks.syncMarbles(
                m_modMgr->m_mods[5], m_modMgr->m_mods[6], m_v2ModTapHooks.ctx);
        }

        float extIn = m_extInputLimiter.Process(input);
        float fuegKnob = m_audioGenParams->GetParam(7);
        auto [v1, v2, v3] = StepOscillators(fuegKnob);
        if (m_v2ModTapLayout && m_v2ModTapHooks.processOsc)
        {
            m_v2ModTapHooks.processOsc(v1, v2, v3, m_v2ModTapHooks.ctx);
        }
        float olvl = m_oscLvl.Process();
        m_envelopeLevel = m_extEnvFilter.Process(std::fabs(extIn));
        m_extGate.Process(m_envelopeLevel);
        bool hasExternal = m_extGate.m_state;
        float chainIn = MixExternalAndOsc(extIn, v1, v2, v3, olvl, hasExternal);
        float output = m_frogBlock.Process(chainIn);
        return ApplyOutputFx(output);
    }
};
