#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterPendingStore.hpp"
#include "HostParameterRouting.hpp"
#include "OwnedAllocationGuard.hpp"
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

static bool test_wasm_process_block_steady_state()
{
    WasmSimHost host;
    configureHost(host);

    static constexpr size_t kBlockSize = 9000;
    std::vector<float> input(kBlockSize);
    std::vector<float> outL(kBlockSize);
    std::vector<float> outR(kBlockSize);
    for (size_t i = 0; i < kBlockSize; i++)
    {
        input[i] = std::sin(static_cast<float>(i) * 0.011f) * 0.5f;
    }

    for (int pass = 0; pass < 8; ++pass)
    {
        FROGGERS_OWNED_ALLOCATION_GUARD();
        host.processBlock(input.data(), outL.data(), outR.data(), kBlockSize, 2);
    }
    return true;
}

static bool test_apply_pending_steady_state()
{
    DesktopHostIO host;
    DelayState delay;
    HostParameterPendingStore pending;

    host.Init();
    delay.init(44100.0f);
    pending.queue(0, 0.25f);
    pending.queue(42, 0.75f);

    for (int pass = 0; pass < 8; ++pass)
    {
        pending.queue(static_cast<size_t>(pass % HostParameterInventory::kCount),
                      static_cast<float>(pass) * 0.01f);
        FROGGERS_OWNED_ALLOCATION_GUARD();
        HostParameterRouting::applyPending(host, delay, pending);
    }
    return true;
}

int main()
{
    if (!test_wasm_process_block_steady_state())
    {
        std::printf("FAIL: WasmSimHost processBlock owned-allocation\n");
        return 1;
    }
    if (!test_apply_pending_steady_state())
    {
        std::printf("FAIL: applyPending owned-allocation\n");
        return 1;
    }

    std::printf("PASS: OwnedAllocation steady-state tests (WasmSimHost/VCV stand-in, applyPending)\n");
    return 0;
}
