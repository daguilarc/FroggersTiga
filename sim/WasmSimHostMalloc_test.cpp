#include "HostPanelLayout.hpp"
#include "WasmSimHost.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

static bool checkLayoutConstants()
{
    if (WasmSimHost::kScopeSize != static_cast<size_t>(HostPanelLayout::kScopeSampleCapacity))
    {
        std::printf("WasmSimHostMalloc_test FAIL kScopeSize=%zu kScopeSampleCapacity=%d\n",
                    WasmSimHost::kScopeSize,
                    HostPanelLayout::kScopeSampleCapacity);
        return false;
    }
    if (WasmSimHost::maxProcessChunk() != WasmSimHost::kProcessChunkSize)
    {
        std::printf("WasmSimHostMalloc_test FAIL maxProcessChunk mismatch\n");
        return false;
    }
    if (WasmSimHost::kScopeModIndices.size() != 4)
    {
        std::printf("WasmSimHostMalloc_test FAIL expected 4 web scope mod indices\n");
        return false;
    }
    return true;
}

static void configureHost(WasmSimHost& host)
{
    host.setSampleRate(44100.0f);
    for (int i = 0; i < 8; i++)
    {
        host.io.SetKnob(static_cast<size_t>(i), 0.5f);
    }
}

static bool repeatedProcessAndScopeCalls()
{
    WasmSimHost host;
    configureHost(host);

    static constexpr size_t kBlockSizes[] = {128, 4096, 9000};
    std::vector<float> scopeScratch(WasmSimHost::kScopeSize);

    for (size_t blockSize : kBlockSizes)
    {
        std::vector<float> input(blockSize);
        std::vector<float> outL(blockSize);
        std::vector<float> outR(blockSize);
        for (size_t i = 0; i < blockSize; i++)
        {
            input[i] = std::sin(static_cast<float>(i) * 0.013f) * 0.7f;
        }

        size_t expectedScopeCount = 0;
        for (int iteration = 0; iteration < 500; iteration++)
        {
            host.processBlock(input.data(), outL.data(), outR.data(), blockSize, 2);
            for (uint8_t modIndex : WasmSimHost::kScopeModIndices)
            {
                const size_t count =
                    host.copyScopeSamples(modIndex, scopeScratch.data(), WasmSimHost::kScopeSize);
                if (count > WasmSimHost::kScopeSize)
                {
                    std::printf("WasmSimHostMalloc_test FAIL scope count overflow mod=%u count=%zu\n",
                                modIndex,
                                count);
                    return false;
                }
                if (iteration == 0)
                {
                    expectedScopeCount = count;
                }
                else if (count != expectedScopeCount)
                {
                    std::printf(
                        "WasmSimHostMalloc_test FAIL scope count changed mod=%u iter=%d count=%zu expected=%zu\n",
                        modIndex,
                        iteration,
                        count,
                        expectedScopeCount);
                    return false;
                }
            }
        }
    }
    return true;
}

int main()
{
    if (!checkLayoutConstants())
    {
        return 1;
    }
    if (!repeatedProcessAndScopeCalls())
    {
        return 1;
    }

    std::printf("WasmSimHostMalloc_test PASS (fixed scratch/scope storage; 1500 process iterations)\n");
    return 0;
}
