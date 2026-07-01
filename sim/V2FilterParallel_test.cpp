#include "PagedHostIO.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

static void seedFilterKnobs(PagedHostIO& io)
{
    io.SetPageKnob(3, 0, 0.45f);
    io.SetPageKnob(3, 1, 0.72f);
    io.SetPageKnob(3, 2, 0.64f);
    io.SetPageKnob(3, 3, 0.51f);
    io.SetPageKnob(3, 4, 0.33f);
    io.SetPageKnob(3, 5, 0.58f);
    io.SetPageKnob(3, 6, 0.83f);
    io.SetPageKnob(3, 7, 1.0f);
    io.SetPageKnob(3, 8, 1.0f);
    io.SetPageKnob(3, 9, 0.35f);
}

static float rmsDifference(const std::vector<float>& a, const std::vector<float>& b)
{
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
    {
        const float d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum / static_cast<float>(a.size()));
}

int main()
{
    static constexpr size_t kBlock = 2048;
    std::vector<float> input(kBlock);
    std::vector<float> outV1(kBlock);
    std::vector<float> outWeb(kBlock);
    for (size_t i = 0; i < kBlock; ++i)
    {
        input[i] = std::sin(static_cast<float>(i) * 0.013f) * 0.5f;
    }

    PagedHostIO v1Io;
    v1Io.m_hostKind = SimHostKind::Desktop;
    v1Io.Init();
    seedFilterKnobs(v1Io);
    v1Io.ProcessBlock(input.data(), outV1.data(), kBlock);

    PagedHostIO webIo;
    webIo.m_hostKind = SimHostKind::Web;
    webIo.Init();
    seedFilterKnobs(webIo);
    webIo.ProcessBlock(input.data(), outWeb.data(), kBlock);

    const float diff = rmsDifference(outV1, outWeb);
    if (diff < 1.0e-4f)
    {
        std::printf("FAIL: v2/web parallel filter should diverge from v1 serial topology (diff=%g)\n", diff);
        return 1;
    }

    std::printf("V2FilterParallel_test OK (rms diff=%g)\n", diff);
    return 0;
}
