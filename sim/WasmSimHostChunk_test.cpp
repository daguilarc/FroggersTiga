#include "WasmSimHost.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

static void configureHost(WasmSimHost& host)
{
    host.setSampleRate(44100.0f);
    for (int i = 0; i < 8; i++)
    {
        host.io.SetKnob(static_cast<size_t>(i), 0.5f);
    }
}

static void processBlockWholeBuffer(WasmSimHost& host,
                                    const float* in,
                                    float* outL,
                                    float* outR,
                                    size_t n,
                                    int numOutputChannels)
{
    host.delay.beginBlock(&host.io.m_pageManager.m_modMgr);
    std::vector<float> mono(n);
    host.io.ProcessBlock(in, mono.data(), n);
    const StereoFxSpread spread = makeStereoFxSpread(
        host.delay,
        host.io.m_engine.getReverbStereoDeltaL(),
        host.io.m_engine.getReverbStereoDeltaR(),
        host.io.m_engine.getLastRvMix());
    applyStereoBus(mono.data(), outL, outR, n, spread, numOutputChannels);
}

static float maxAbsDiff(const float* a, const float* b, size_t n)
{
    float maxDiff = 0.0f;
    for (size_t i = 0; i < n; i++)
    {
        maxDiff = std::max(maxDiff, std::fabs(a[i] - b[i]));
    }
    return maxDiff;
}

static bool compareBlockSizes(const std::vector<float>& input, size_t blockSize)
{
    const size_t n = input.size();
    std::vector<float> refL(n);
    std::vector<float> refR(n);
    std::vector<float> chunkL(n);
    std::vector<float> chunkR(n);

    WasmSimHost referenceHost;
    WasmSimHost chunkedHost;
    configureHost(referenceHost);
    configureHost(chunkedHost);

    processBlockWholeBuffer(
        referenceHost, input.data(), refL.data(), refR.data(), n, 2);
    chunkedHost.processBlock(input.data(), chunkL.data(), chunkR.data(), n, 2);

    const float leftDiff = maxAbsDiff(refL.data(), chunkL.data(), n);
    const float rightDiff = maxAbsDiff(refR.data(), chunkR.data(), n);
    if (leftDiff > 1.0e-5f || rightDiff > 1.0e-5f)
    {
        std::printf("WasmSimHostChunk_test FAIL blockSize=%zu leftDiff=%g rightDiff=%g\n",
                    blockSize,
                    leftDiff,
                    rightDiff);
        return false;
    }
    return true;
}

int main()
{
    if (WasmSimHost::maxProcessChunk() != WasmSimHost::kProcessChunkSize)
    {
        std::printf("WasmSimHostChunk_test FAIL maxProcessChunk mismatch\n");
        return 1;
    }

    static constexpr size_t kBlockSizes[] = {128, 4096, 9000};
    for (size_t blockSize : kBlockSizes)
    {
        std::vector<float> input(blockSize);
        for (size_t i = 0; i < blockSize; i++)
        {
            input[i] = std::sin(static_cast<float>(i) * 0.013f) * 0.7f;
        }
        if (!compareBlockSizes(input, blockSize))
        {
            return 1;
        }
    }

    std::printf("WasmSimHostChunk_test PASS\n");
    return 0;
}
