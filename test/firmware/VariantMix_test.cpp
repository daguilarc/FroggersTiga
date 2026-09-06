#include "VariantMix_api.hpp"

#include <cmath>
#include <cstdio>

#include "Check.hpp"

namespace
{
bool Exact(float a, float b)
{
    return a == b;
}

bool Near(float a, float b, float eps = 1e-6f)
{
    return std::fabs(a - b) <= eps;
}

// The weights as the spec states them, not as a rounded decimal.
constexpr float kDry = 7.0f / 12.0f;
constexpr float kRing = 5.0f / 12.0f;
} // namespace

int main()
{
    const float in = 0.37f;
    const float v1 = 0.21f;
    const float v2 = -0.44f;
    const float v3 = 0.62f;
    const float olvl = 0.4f;
    const float ringMod = (in * v1 + in * v2 + in * v3) * (1.0f / 3.0f);

    // Each variant is identified by what it does, not by a string it carries.
    Check(variantmix::SoloHasReverb(), "Solo has the reverb page");
    Check(!variantmix::GuitarHasReverb(), "Guitar has no reverb page");

    // Page navigation wraps against the count, so removing the reverb page
    // leaves no hole -- Guitar simply has one page fewer.
    Check(variantmix::SoloPageCount() == 5, "Solo has five pages");
    Check(variantmix::GuitarPageCount() == 4, "Guitar has four pages");
    Check(variantmix::SoloPageCount() - variantmix::GuitarPageCount() == 1,
          "Guitar is exactly one page shorter than Solo");

    // Guitar's gate-open mix, against the exact weights.
    const float guitarOpen = variantmix::GuitarMix(in, v1, v2, v3, olvl, true).mix;
    Check(Near(guitarOpen, kDry * in + kRing * ringMod),
          "Guitar gate-open is (7/12)*extIn + (5/12)*ringMod");

    // The weights sum to 1, so Guitar is not louder than Solo at the same knobs.
    Check(Exact(kDry + kRing, 1.0f), "the two weights sum to exactly 1");

    // Solo's gate-open formula is untouched by the Guitar work.
    const float soloOpen = variantmix::SoloMix(in, v1, v2, v3, olvl, true).mix;
    Check(Near(soloOpen, ringMod), "Solo gate-open is the ring mod alone, no dry term");

    // Guitar carries a dry term and Solo does not: the two must differ here, or
    // the variant selector is not reaching the mix at all.
    Check(!Near(guitarOpen, soloOpen, 1e-4f),
          "Guitar and Solo differ with the gate open");

    // Gate closed, the two variants are identical.
    const float soloClosed = variantmix::SoloMix(in, v1, v2, v3, olvl, false).mix;
    const float guitarClosed = variantmix::GuitarMix(in, v1, v2, v3, olvl, false).mix;
    Check(Exact(soloClosed, guitarClosed), "gate closed, both variants are bit-identical");
    Check(Near(soloClosed, olvl * ((v1 + v2 + v3) * (1.0f / 3.0f))),
          "gate closed is OLVL * average(VCO1, VCO2, VCO3)");

    // FUEG/Crispy reaches the engine as a knob value, never as a term in the
    // external mix: with the gate open the OLVL argument must not matter.
    const float guitarOpenOtherOlvl = variantmix::GuitarMix(in, v1, v2, v3, 0.9f, true).mix;
    const float soloOpenOtherOlvl = variantmix::SoloMix(in, v1, v2, v3, 0.9f, true).mix;
    Check(Exact(guitarOpen, guitarOpenOtherOlvl), "Guitar external mix ignores OLVL");
    Check(Exact(soloOpen, soloOpenOtherOlvl), "Solo external mix ignores OLVL");

    // With pair-AR enabled the ring-mod term still uses raw per-VCO samples, so
    // the gate-open output is unchanged by pair-AR even though pair-AR state
    // advances. Both facts are asserted: the value, and the state.
    const variantmix::Probe soloAr = variantmix::SoloPairArRun(in, v1, v2, v3, 64);
    const variantmix::Probe guitarAr = variantmix::GuitarPairArRun(in, v1, v2, v3, 64);
    Check(Near(soloAr.mix, ringMod),
          "pair-AR on, Solo gate-open still uses raw per-VCO samples");
    Check(Near(guitarAr.mix, kDry * in + kRing * ringMod),
          "pair-AR on, Guitar gate-open still uses raw per-VCO samples");

    // The positive control for the line above: pair-AR state must actually have
    // moved, or "unchanged output" proves nothing about routing.
    Check(soloAr.pair12Level > 0.0f && soloAr.pair23Level > 0.0f,
          "pair-AR state advanced on the gate-open path (positive control)");

    // MixOscVoices runs on the gate-open path in both variants and advances
    // that state identically. A future early-out that skips it would break this.
    Check(Exact(soloAr.pair12Level, guitarAr.pair12Level),
          "pair-AR 1-2 envelope identical across variants");
    Check(Exact(soloAr.pair23Level, guitarAr.pair23Level),
          "pair-AR 2-3 envelope identical across variants");

    // Reverb bypass, Solo only: at a zero mix the reverb does not run, and at a
    // full mix it does. The chain itself runs once per sample either way, which
    // is the "one chain, not two" scenario measured rather than read.
    const variantmix::FxProbe atZero = variantmix::SoloReverbAt(0.0f, 256, true);
    const variantmix::FxProbe atFull = variantmix::SoloReverbAt(1.0f, 256, false);
    Check(atZero.bypassed, "zero reverb mix leaves the reverb bypassed");
    Check(!atFull.bypassed, "a full reverb mix takes the reverb out of bypass");
    Check(atZero.chainRuns == 256, "the output-FX chain runs once per sample, bypassed");
    Check(atFull.chainRuns == 256, "the output-FX chain runs once per sample, engaged");

    // The hysteresis: a mix parked between the two thresholds holds whichever
    // state it arrived in. Both directions are asserted, because a single
    // direction is also what a plain threshold would produce.
    const variantmix::FxProbe boundaryFromBelow = variantmix::SoloReverbAt(3e-4f, 256, false);
    const variantmix::FxProbe boundaryFromAbove = variantmix::SoloReverbAt(3e-4f, 256, true);
    Check(boundaryFromBelow.bypassed, "a mix between the thresholds does not leave bypass");
    Check(!boundaryFromAbove.bypassed, "the same mix approached from above stays engaged");
    Check(boundaryFromBelow.bypassed != boundaryFromAbove.bypassed,
          "one mix, two states: the bypass has real hysteresis, not a bare threshold");

    // One UpdateParams call is one biquad recompute, not six, and each bump
    // smoother advances exactly one step rather than two.
    const variantmix::ParamProbe pp = variantmix::SoloUpdateParamsOnce(0.25f);
    Check(pp.coefficientRecomputes == 1,
          "UpdateParams costs one biquad recompute per sample");
    Check(Exact(pp.bumpFreqAfter, pp.oneStepReference),
          "m_bumpFreq advances exactly one smoother step per sample");
    Check(Exact(pp.bumpWidthAfter, pp.oneStepReference),
          "m_bumpWidth advances exactly one smoother step per sample");
    Check(Exact(pp.bumpFreqAfter, pp.bumpWidthAfter),
          "both bump smoothers sit at the same point of the sweep");

    if (g_failures == 0)
    {
        std::printf("PASS: VariantMix_test, 28 checks\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
