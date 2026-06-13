#include "FroggersEngine.hpp"
#include "Page.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

static void runFreshEngine(const std::vector<float>& input, float* out)
{
    PageManager pageManager;
    FroggersEngine engine;
    engine.Config(&pageManager);
    pageManager.SetAllParamsTracking();
    engine.SetSampleRate(44100.0f);
    engine.SetSimFxInsert(nullptr, nullptr);
    engine.ProcessBlock(input.data(), out, input.size());
}

int main()
{
    std::vector<float> input(4096);
    std::vector<float> outA(4096);
    std::vector<float> outB(4096);
    for (size_t i = 0; i < input.size(); i++)
    {
        input[i] = std::sin(static_cast<float>(i) * 0.013f) * 0.7f;
    }

    runFreshEngine(input, outA.data());
    runFreshEngine(input, outB.data());

    float maxDiff = 0.0f;
    for (size_t i = 0; i < input.size(); i++)
    {
        const float diff = std::fabs(outA[i] - outB[i]);
        if (diff > maxDiff)
        {
            maxDiff = diff;
        }
    }

    if (maxDiff > 0.0f)
    {
        std::printf("HookIdentity_test FAIL maxDiff=%g\n", maxDiff);
        return 1;
    }

    std::printf("HookIdentity_test PASS\n");
    return 0;
}
