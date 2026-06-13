#include "Fuegoize.hpp"
#include "Parameter.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

struct TestParam : Parameter
{
    TestParam(uint8_t position)
    {
        m_position = position;
        m_knobValue = 0.5f;
    }

    float fuegoizeViaCore(float value, float fuegKnob)
    {
        m_knobValue = value;
        TestParam fuegParam(7);
        fuegParam.m_knobValue = fuegKnob;
        m_fuegoizationKnob = &fuegParam;
        ModMgr modMgr;
        return Get(&modMgr);
    }
};

int main()
{
    struct Tuple
    {
        float value;
        float fueg;
        uint8_t row;
    };

    const std::vector<Tuple> tuples = {
        {0.0f, 0.0f, 0},   {1.0f, 0.0f, 3},   {0.5f, 0.0f, 6},
        {0.25f, 1.0f, 0},  {0.75f, 1.0f, 2},  {0.33f, 1.0f, 5},
        {0.1f, 0.5f, 1},   {0.9f, 0.5f, 4},   {0.42f, 0.25f, 6},
        {0.05f, 0.75f, 0}, {0.95f, 0.75f, 3}, {0.66f, 0.33f, 1},
        {0.123f, 0.88f, 2}, {0.888f, 0.12f, 4}, {0.314f, 0.62f, 5},
        {0.777f, 1.0f, 6},
    };

    float maxDiff = 0.0f;
    for (const Tuple& t : tuples)
    {
        TestParam param(t.row);
        const float core = param.fuegoizeViaCore(t.value, t.fueg);
        const float sim = Fuegoize(t.value, t.fueg, t.row);
        const float diff = std::fabs(core - sim);
        if (diff > maxDiff)
        {
            maxDiff = diff;
        }
    }

    if (maxDiff > 0.0f)
    {
        std::printf("Fuegoize_test FAIL maxDiff=%g\n", maxDiff);
        return 1;
    }

    std::printf("Fuegoize_test PASS (%zu tuples)\n", tuples.size());
    return 0;
}
