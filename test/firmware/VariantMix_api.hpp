#pragma once

// The variant is a build-time choice, so each variant's engine lives in its own
// translation unit and is reached through this API. One test binary can then
// assert both at once, which is the point: an edit to one variant's formula
// cannot move the other without a test noticing.

namespace variantmix
{

struct Probe
{
    float mix;
};

// One MixExternalAndOsc call. `mix` is its return value.
Probe SoloMix(float input, float v1, float v2, float v3, float olvl, bool hasExternal);
Probe GuitarMix(float input, float v1, float v2, float v3, float olvl, bool hasExternal);

// Solo only: drive the engine's output FX with a given reverb mix and report
// whether the reverb was bypassed for that sample, plus how many times the
// output-FX chain ran.
struct FxProbe
{
    bool bypassed;
    int chainRuns;
};
// `startEngaged` picks which side the mix is approached from, which is the
// whole content of the hysteresis: between the two thresholds the state must
// hold, so the same mix gives different answers depending on where it came from.
FxProbe SoloReverbAt(float rvMixKnob, int settleSamples, bool startEngaged);

// One UpdateParams() call, reporting how far the bump smoothers moved and how
// many biquad recomputes it cost. Both used to be doubled.
struct ParamProbe
{
    float bumpFreqAfter;
    float bumpWidthAfter;
    float oneStepReference;
    int coefficientRecomputes;
};
ParamProbe SoloUpdateParamsOnce(float target);

bool SoloHasReverb();
bool GuitarHasReverb();
int SoloPageCount();
int GuitarPageCount();

} // namespace variantmix
