// One behavioral property per DSP building block, each with a positive
// control that flips the same property false. The control is what
// distinguishes an assertion that tests real behavior from one that would
// pass no matter what the code does.

#include "BiquadSection.hpp"
#include "Comb.hpp"
#include "Marbles.hpp"
#include "OPLowPassFilter.hpp"
#include "PolynomialDrive.hpp"
#include "RGen.hpp"
#include "SDDSine.hpp"
#include "SampleRateReducer.hpp"
#include "TanhSaturator.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>

namespace
{
int g_failures = 0;
void Check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAIL: %s\n", what);
        g_failures++;
    }
}

// Unity coefficients pass the input through unchanged; a non-unity gain does
// not.
void TestBiquadSection()
{
    BiquadSection unity;
    float out = unity.Process(0.37f);
    Check(out == 0.37f, "BiquadSection: default (unity) coefficients pass the input through");
    std::printf("BiquadSection unity: Process(0.37) = %f\n", out);

    BiquadSection gained;
    gained.m_b0 = 2.0f;
    float gainedOut = gained.Process(0.37f);
    Check(gainedOut != 0.37f, "BiquadSection: a non-unity b0 does not pass the input through (control)");
    std::printf("BiquadSection b0=2.0: Process(0.37) = %f\n", gainedOut);
}

// Zero feedback is an identity pass-through, regardless of delay settings;
// nonzero feedback makes the output diverge from the input once the delay
// line recirculates.
void TestComb()
{
    Comb identity;
    identity.SetFeedback(0.0f);
    identity.m_delaySamples = 50;
    bool allIdentity = true;
    for (int i = 0; i < 20; i++)
    {
        float input = 0.1f * i;
        float out = identity.Process(input);
        if (out != input)
        {
            allIdentity = false;
        }
    }
    Check(allIdentity, "Comb: zero feedback passes every input through unchanged");

    Comb fed;
    fed.SetFeedback(0.5f);
    fed.SetCutoffAlpha(1.0f);
    fed.m_delaySamples = 1;
    float first = fed.Process(1.0f);
    float second = fed.Process(1.0f);
    Check(second != 1.0f, "Comb: nonzero feedback makes the output diverge from the input (control)");
    std::printf("Comb feedback=0.5: Process(1.0) twice = %f, %f\n", first, second);
}

// A lowpass driven with DC settles to the DC value; its first sample lands
// below that target.
void TestOPLowPassFilter()
{
    OPLowPassFilter filter;
    filter.SetAlphaFromNatFreq(0.001f);
    const float dc = 0.8f;
    float first = filter.Process(dc);
    Check(first < dc, "OPLowPassFilter: the first sample of a slow lowpass is below the DC target (control)");
    float last = 0.0f;
    for (int i = 0; i < 10000; i++)
    {
        last = filter.Process(dc);
    }
    Check(std::fabs(last - dc) < 1e-4f, "OPLowPassFilter: driven with DC, the filter settles to the DC value");
    std::printf("OPLowPassFilter: first=%f last=%f target=%f\n", first, last, dc);
}

// At zero drive (unity gain, flat coefficients, no fuzz, no hash/flip) a
// small input is close to unchanged; at full drive the same small input
// comes out somewhere else entirely.
void TestFrogBlock()
{
    FrogBlock zeroDrive;
    zeroDrive.m_fuzz = 0.0f;
    zeroDrive.m_polynomialDrive.SetGain(0.0f);
    zeroDrive.m_polynomialDrive.SetCoefs(0.0f);
    zeroDrive.m_digitalReorganizer.SetHash(0.0f);
    zeroDrive.m_digitalReorganizer.SetFlip(0.0f);
    zeroDrive.m_sampleRateReducer1.SetFreq(1.0f);
    zeroDrive.m_sampleRateReducer2.SetFreq(1.0f);
    for (int i = 0; i < 200; i++)
    {
        zeroDrive.Process(0.0f);
    }
    const float input = 0.02f;
    float zeroOut = zeroDrive.Process(input);
    Check(std::fabs(zeroOut - input) < 0.02f, "FrogBlock: at zero drive a small input is near-identity");

    FrogBlock fullDrive;
    fullDrive.m_fuzz = 1.0f;
    fullDrive.m_polynomialDrive.SetGain(1.0f);
    fullDrive.m_polynomialDrive.SetCoefs(0.7f);
    fullDrive.m_digitalReorganizer.SetHash(1.0f);
    fullDrive.m_digitalReorganizer.SetFlip(0.6f);
    fullDrive.m_sampleRateReducer1.SetFreq(1.0f);
    fullDrive.m_sampleRateReducer2.SetFreq(1.0f);
    for (int i = 0; i < 200; i++)
    {
        fullDrive.Process(0.0f);
    }
    float fullOut = fullDrive.Process(input);
    Check(std::fabs(fullOut - input) > 0.02f, "FrogBlock: at full drive the same small input is not near-identity (control)");
    std::printf("FrogBlock: input=%f zeroDrive out=%f fullDrive out=%f\n", input, zeroOut, fullOut);
}

// Two seeds give different sequences; the same seed reproduces its sequence.
// RGen's state is a single static generator, so the "seed" is set directly.
void TestRGen()
{
    RGen::s_state = 12345u;
    uint32_t a1 = RGen::NextUInt();
    uint32_t a2 = RGen::NextUInt();

    RGen::s_state = 12345u;
    uint32_t b1 = RGen::NextUInt();
    uint32_t b2 = RGen::NextUInt();
    Check(a1 == b1 && a2 == b2, "RGen: the same seed reproduces the same sequence");

    RGen::s_state = 99999u;
    uint32_t c1 = RGen::NextUInt();
    Check(c1 != a1, "RGen: a different seed gives a different sequence (control)");
    std::printf("RGen: seed 12345 -> %u, %u; seed 99999 -> %u\n", a1, a2, c1);
}

// Evaluate(0.25) and Evaluate(0.75) sit at the +1/-1 peaks; Evaluate(0) does
// not.
void TestSDDSine()
{
    float atQuarter = SDDSine::Evaluate(0.25f);
    float atThreeQuarter = SDDSine::Evaluate(0.75f);
    float atZero = SDDSine::Evaluate(0.0f);
    Check(std::fabs(atQuarter - 1.0f) < 0.001f, "SDDSine: Evaluate(0.25) is at the +1 peak");
    Check(std::fabs(atThreeQuarter - (-1.0f)) < 0.001f, "SDDSine: Evaluate(0.75) is at the -1 peak");
    Check(std::fabs(atZero) < 0.001f, "SDDSine: Evaluate(0) is near zero, not a peak (control)");
    std::printf("SDDSine: Evaluate(0.25)=%f Evaluate(0.75)=%f Evaluate(0)=%f\n",
                 atQuarter, atThreeQuarter, atZero);
}

// A far-reduced sample rate holds a value for multiple samples; reduction
// off passes every input straight through.
void TestSampleRateReducer()
{
    SampleRateReducer held;
    held.SetFreq(0.25f);
    float outputs[8];
    for (int i = 0; i < 8; i++)
    {
        outputs[i] = held.Process(static_cast<float>(i));
    }
    // The phase wraps every 4th call, so calls 4, 5, 6 hold call 3's value
    // (3.0) rather than tracking the instantaneous input (4, 5, 6).
    bool holds = (outputs[4] == 3.0f) && (outputs[5] == 3.0f) && (outputs[6] == 3.0f) && (outputs[7] == 7.0f);
    Check(holds, "SampleRateReducer: a reduced rate holds a value across samples instead of tracking the input");
    std::printf("SampleRateReducer freq=0.25: outputs[3..7] = %f %f %f %f %f\n",
                 outputs[3], outputs[4], outputs[5], outputs[6], outputs[7]);

    SampleRateReducer passthrough;
    passthrough.SetFreq(1.0f);
    bool allPassthrough = true;
    for (int i = 0; i < 8; i++)
    {
        float out = passthrough.Process(static_cast<float>(i));
        if (out != static_cast<float>(i))
        {
            allPassthrough = false;
        }
    }
    Check(allPassthrough, "SampleRateReducer: reduction off passes every input straight through (control)");
}

// Output magnitude is bounded for a large input; for a small input it is
// near-linear.
void TestTanhSaturator()
{
    TanhSaturator<false> sat;
    sat.SetInputGain(1.0f);
    float big = sat.Process(100.0f);
    Check(std::fabs(big) <= 1.0f + 1e-4f, "TanhSaturator: output magnitude is bounded for a large input");

    float small = sat.Process(0.01f);
    Check(std::fabs(small - 0.01f) < 0.001f, "TanhSaturator: a small input is near-linear (control)");
    std::printf("TanhSaturator: Process(100)=%f Process(0.01)=%f\n", big, small);
}

// Increment() changes the sampled output over steps when the bag always
// advances; a zero-probability bag never advances and never changes.
void TestMarbles()
{
    Marbles moving;
    moving.m_probability = 1.0f;
    moving.m_dejaVuKnob[0] = 0.0f;
    moving.m_dejaVuKnob[1] = 0.0f;
    moving.m_size[0] = Marbles::x_numMarbles;
    moving.m_size[1] = Marbles::x_numMarbles;
    moving.m_filter[0].m_alpha = 1.0f;
    moving.m_filter[1].m_alpha = 1.0f;
    moving.Process();
    float prev = *moving.m_output[0];
    int changes = 0;
    for (int i = 0; i < 20; i++)
    {
        moving.Increment();
        moving.Process();
        float cur = *moving.m_output[0];
        if (cur != prev)
        {
            changes++;
        }
        prev = cur;
    }
    Check(changes > 0, "Marbles: Increment changes the output over steps when the bag always advances");

    Marbles frozen;
    frozen.m_probability = 0.0f;
    frozen.m_dejaVuKnob[0] = 0.0f;
    frozen.m_dejaVuKnob[1] = 0.0f;
    frozen.m_size[0] = Marbles::x_numMarbles;
    frozen.m_size[1] = Marbles::x_numMarbles;
    frozen.m_filter[0].m_alpha = 1.0f;
    frozen.m_filter[1].m_alpha = 1.0f;
    frozen.Process();
    float frozenPrev = *frozen.m_output[0];
    int frozenChanges = 0;
    for (int i = 0; i < 20; i++)
    {
        frozen.Increment();
        frozen.Process();
        float cur = *frozen.m_output[0];
        if (cur != frozenPrev)
        {
            frozenChanges++;
        }
        frozenPrev = cur;
    }
    Check(frozenChanges == 0, "Marbles: a zero-probability bag never changes its output (control)");
    std::printf("Marbles: moving changes=%d/20 frozen changes=%d/20\n", changes, frozenChanges);
}
} // namespace

int main()
{
    TestBiquadSection();
    TestComb();
    TestOPLowPassFilter();
    TestFrogBlock();
    TestRGen();
    TestSDDSine();
    TestSampleRateReducer();
    TestTanhSaturator();
    TestMarbles();

    if (g_failures == 0)
    {
        std::printf("PASS: DspModules_test\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
