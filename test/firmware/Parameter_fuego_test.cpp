// Fuegoization is the bit-scramble Parameter::Get applies to a row's knob
// value when a FUEG row is wired in as its m_fuegoizationKnob. At knob 0 the
// scramble mask is empty and every row passes its value through unchanged;
// away from 0 the mask widens and rows generally stop matching their input.
// This asserts both ends against a copy of the scramble formula itself, so an
// edit to the bit-twiddling in Parameter::Get shows up here even if the
// on-device behavior still "looks" plausible.

#include "Page.hpp"

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

// The same bit-scramble as Parameter::Get, kept as an independent
// re-implementation rather than a call into production code.
float Scramble(float value, float fuegKnob, uint8_t position)
{
    uint16_t mask = (1 << static_cast<uint16_t>(std::round(fuegKnob * 8))) - 1;
    uint16_t inputInt = static_cast<uint16_t>(value * 255);
    float inputRemainder = value * 255 - inputInt;
    uint16_t lowerBits = inputInt & mask;

    lowerBits ^= (lowerBits << 3) & mask;
    lowerBits ^= (lowerBits >> 5) & mask;
    lowerBits ^= (lowerBits << 1) & mask;
    uint8_t sh = 1u + (uint8_t)(position % ((mask + 1) ? (mask + 1) : 1));
    lowerBits ^= (lowerBits >> sh) & mask;

    inputInt = (inputInt & ~mask) | lowerBits;
    return (static_cast<float>(inputInt) + inputRemainder) / 255;
}
} // namespace

int main()
{
    PageManager pageManager;
    Page* page = pageManager.AddPage();
    const uint8_t crispyRow = 7;
    for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
    {
        if (row != crispyRow)
        {
            page->InitParam("ROWX", row, 0.0f);
        }
    }
    page->SetFuegoization(crispyRow);
    ModMgr modMgr;

    const float inputs[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    const int numInputs = 5;
    const int rowsChecked = static_cast<int>(Parameter::x_numParameters) - 1;

    // At FUEG knob 0.0 the mask is empty, so every row's output equals its
    // input.
    page->m_parameters[crispyRow].m_knobValue = 0.0f;
    bool allIdentity = true;
    for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
    {
        if (row == crispyRow)
        {
            continue;
        }
        for (int i = 0; i < numInputs; i++)
        {
            page->m_parameters[row].m_knobValue = inputs[i];
            float out = page->m_parameters[row].Get(&modMgr);
            if (std::fabs(out - inputs[i]) > 1.0f / 255.0f)
            {
                allIdentity = false;
            }
        }
    }
    Check(allIdentity, "FUEG knob 0.0 leaves every row's output equal to its input");
    std::printf("rows checked: %d\n", rowsChecked);

    // Away from 0.0 the mask is nonempty: at least one (row, input) pair
    // differs from identity, and every pair matches the re-implemented
    // scramble formula.
    const float fuegKnobs[] = {0.5f, 1.0f};
    for (float fk : fuegKnobs)
    {
        page->m_parameters[crispyRow].m_knobValue = fk;
        int differing = 0;
        bool formulaMatches = true;
        for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
        {
            if (row == crispyRow)
            {
                continue;
            }
            for (int i = 0; i < numInputs; i++)
            {
                page->m_parameters[row].m_knobValue = inputs[i];
                float out = page->m_parameters[row].Get(&modMgr);
                float expected = Scramble(inputs[i], fk, row);
                if (std::fabs(out - expected) > 1e-6f)
                {
                    formulaMatches = false;
                }
                if (std::fabs(out - inputs[i]) > 1.0f / 255.0f)
                {
                    differing++;
                }
            }
        }
        Check(differing > 0, "FUEG knob away from 0.0 makes at least one pair differ from identity");
        Check(formulaMatches, "every pair matches the re-implemented scramble formula");
        std::printf("FUEG knob %.2f: %d of %d pairs differ from identity\n",
                     fk, differing, rowsChecked * numInputs);
    }

    if (g_failures == 0)
    {
        std::printf("PASS: Parameter_fuego_test\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
