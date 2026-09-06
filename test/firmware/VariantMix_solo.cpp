#define FROGGERS_VARIANT_SOLO 1

#include "VariantMix_body.inl"

namespace variantmix
{
Probe SoloMix(float input, float v1, float v2, float v3, float olvl, bool hasExternal)
{
    return OneMix(input, v1, v2, v3, olvl, hasExternal);
}

Probe SoloPairArRun(float input, float v1, float v2, float v3, int n)
{
    return PairArRun(input, v1, v2, v3, n);
}

FxProbe SoloReverbAt(float rvMixKnob, int settleSamples, bool startEngaged)
{
    FroggersEngine& e = Engine();
    e.m_pairAr = nullptr;
    e.m_vcoAdsr = nullptr;
    e.m_adsrParams = nullptr;
    e.SetSampleRate(48000.0f);
    // The engine is a static, so the smoother carries whatever the previous
    // probe left in it. Place it explicitly on the side we mean to approach
    // from, or this measures the last call instead of this one.
    e.m_rvMix.m_filter.m_output = startEngaged ? 1.0f : 0.0f;
    e.m_rvBypassed = !startEngaged;
    static int s_chainRuns;
    s_chainRuns = 0;
    e.m_simFxInsertCtx = &s_chainRuns;
    e.m_simFxInsert = [](float x, void* ctx) -> float {
        (*static_cast<int*>(ctx))++;
        return x;
    };
    e.m_rvMix.SetTarget(rvMixKnob);
    for (int i = 0; i < settleSamples; i++)
    {
        e.ApplyOutputFx(0.0f);
    }
    e.m_simFxInsert = nullptr;
    e.m_simFxInsertCtx = nullptr;
    return {e.m_rvBypassed, s_chainRuns};
}

ParamProbe SoloUpdateParamsOnce(float target)
{
    FroggersEngine& e = Engine();
    e.SetSampleRate(48000.0f);
    e.m_bumpFreq.m_filter.m_output = 0.0f;
    e.m_bumpWidth.m_filter.m_output = 0.0f;
    e.m_bumpResonance.m_filter.m_output = 0.0f;
    e.m_bumpFreq.SetTarget(target);
    e.m_bumpWidth.SetTarget(target);
    e.m_bumpResonance.SetTarget(target);

    ResonantBump::s_updateCount = 0;
    e.UpdateParams();
    const int recomputes = ResonantBump::s_updateCount;

    // What exactly one advance of the same smoother looks like.
    RuntimeParam reference;
    reference.SetSmoothingRate(48000.0f);
    reference.SetTarget(target);
    const float oneStep = reference.Process();

    return {e.m_bumpFreq.m_filter.m_output,
            e.m_bumpWidth.m_filter.m_output,
            oneStep,
            recomputes};
}

bool SoloHasReverb()
{
    return FROGGERS_HAS_REVERB != 0;
}

int SoloPageCount()
{
    Engine();
    return Pages().m_numPages;
}
} // namespace variantmix
