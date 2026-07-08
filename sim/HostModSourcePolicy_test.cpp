#include "CvMidiBridge.hpp"
#include "RGen.hpp"
#include "SimModSource.hpp"

#include <cstdio>
#include <set>

int main()
{
    CvMidiBridge bridge;
    bridge.setCcPairEnabled(0, true);
    bridge.setCcPairEnabled(1, true);

    if (IsSimModSourceAvailable(0, bridge, SimHostKind::Vcv))
    {
        std::printf("FAIL: VCV mod index 0 should be unavailable\n");
        return 1;
    }

    if (IsSimModSourceAvailable(1, bridge, SimHostKind::Vcv))
    {
        std::printf("FAIL: VCV mod index 1 should be unavailable\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(4, bridge, SimHostKind::Vcv))
    {
        std::printf("FAIL: VCV mod index 4 should be available\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(0, bridge, SimHostKind::Desktop))
    {
        std::printf("FAIL: Desktop mod index 0 should be available when CC enabled\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(0, bridge, SimHostKind::Web))
    {
        std::printf("FAIL: Web mod index 0 should be available when CC enabled\n");
        return 1;
    }

    RGen::s_state = 0xa341316cu;
    std::set<uint8_t> seen;
    for (int i = 0; i < 500; i++)
    {
        RGen rgen;
        const uint8_t idx = DrawAssignableModLane(rgen, bridge, SimHostKind::Vcv);
        if (idx == 0 || idx == 1)
        {
            std::printf("FAIL: VCV random pool must exclude mod indices 0 and 1, got %u\n", idx);
            return 1;
        }
        if (idx != 255)
        {
            seen.insert(idx);
        }
    }

    if (seen.find(4) == seen.end() && seen.find(5) == seen.end() && seen.find(6) == seen.end())
    {
        std::printf("FAIL: VCV random pool should include internal indices 4/5/6\n");
        return 1;
    }

    std::printf("HostModSourcePolicy_test OK\n");
    return 0;
}
