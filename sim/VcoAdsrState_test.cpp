#include "VcoAdsrState.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool Near(float a, float b, float eps = 1e-5f)
{
    return std::fabs(a - b) <= eps;
}
} // namespace

int main()
{
    if (!(VcoAdsrState::kMaxReleaseSeconds >= 10.0f))
    {
        std::printf("FAIL: kMaxReleaseSeconds expected >= 10 got %f\n", VcoAdsrState::kMaxReleaseSeconds);
        return 1;
    }

    const float sampleRate = 48000.0f;
    VcoAdsrState adsr;
    adsr.init(sampleRate);

    adsr.setGate(true);
    float held = 0.0f;
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        held = adsr.apply(0, 1.0f, 0.0f, 0.5f);
    }

    if (!Near(held, 1.0f, 1e-3f))
    {
        std::printf("FAIL: gate high should hold at 1.0 got %f\n", held);
        return 1;
    }

    adsr.setGate(false);
    float fastRelease = held;
    for (int i = 0; i < 4800; ++i)
    {
        fastRelease = adsr.apply(0, 1.0f, 0.0f, 0.0f);
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        adsr.apply(0, 1.0f, 0.0f, 1.0f);
    }
    adsr.setGate(false);
    float slowRelease = 1.0f;
    for (int i = 0; i < 4800; ++i)
    {
        slowRelease = adsr.apply(0, 1.0f, 0.0f, 1.0f);
    }

    if (!(fastRelease < slowRelease))
    {
        std::printf("FAIL: release min should fall faster than release max (fast=%f slow=%f)\n",
                    fastRelease,
                    slowRelease);
        return 1;
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    float fastAttack = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        fastAttack = adsr.apply(1, 1.0f, 0.0f, 0.5f);
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    float slowAttack = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        slowAttack = adsr.apply(1, 1.0f, 1.0f, 0.5f);
    }

    if (!(fastAttack > slowAttack))
    {
        std::printf("FAIL: attack min should rise faster than attack max (fast=%f slow=%f)\n",
                    fastAttack,
                    slowAttack);
        return 1;
    }

    adsr.init(sampleRate);
    const float v0 = adsr.apply(0, 0.5f, 0.5f, 0.5f);
    const float v1 = adsr.apply(1, 0.5f, 0.5f, 0.5f);
    if (!Near(v0, 0.0f) || !Near(v1, 0.0f))
    {
        std::printf("FAIL: idle voices should pass zero envelope level\n");
        return 1;
    }

    std::printf("VcoAdsrState_test OK\n");
    return 0;
}
