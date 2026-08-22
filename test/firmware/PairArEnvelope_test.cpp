#include "PairArEnvelope.hpp"
#include "PhaseUtils.hpp"

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
    if (!Near(PairArEnvelope::kMinTimeSec, 1e-3f))
    {
        std::printf("FAIL: kMinTimeSec expected 1e-3 got %f\n", PairArEnvelope::kMinTimeSec);
        return 1;
    }

    if (!Near(PairArEnvelope::kMaxTimeSec, 10.f))
    {
        std::printf("FAIL: kMaxTimeSec expected 10 got %f\n", PairArEnvelope::kMaxTimeSec);
        return 1;
    }

    if (!Near(PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec,
                                            PairArEnvelope::kMaxTimeSec,
                                            0.0f),
              1e-3f))
    {
        std::printf("FAIL: knob 0 time constant\n");
        return 1;
    }

    if (!Near(PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec,
                                            PairArEnvelope::kMaxTimeSec,
                                            0.5f),
              0.1f))
    {
        std::printf("FAIL: knob 0.5 time constant expected 0.1\n");
        return 1;
    }

    if (!Near(PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec,
                                            PairArEnvelope::kMaxTimeSec,
                                            1.0f),
              10.f))
    {
        std::printf("FAIL: knob 1 time constant\n");
        return 1;
    }

    std::printf("PairArEnvelope_test OK\n");
    return 0;
}
