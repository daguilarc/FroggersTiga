#include "RGen.hpp"
#include "VcvHostModSource.hpp"

#include <cstdio>

int main()
{
    RGen::s_state = 0x12345678u;

    for (int trial = 0; trial < 10000; trial++)
    {
        RGen rgen;
        const uint8_t modIndex = PickVcvRandomModIndex(rgen);
        if (modIndex == 0 || modIndex == 1)
        {
            std::printf("FAIL: PickVcvRandomModIndex returned CC index %u on trial %d\n",
                        modIndex,
                        trial);
            return 1;
        }
        if (modIndex != 255 && modIndex != 4 && modIndex != 5 && modIndex != 6)
        {
            std::printf("FAIL: PickVcvRandomModIndex returned unexpected index %u on trial %d\n",
                        modIndex,
                        trial);
            return 1;
        }
    }

    std::printf("VcvRandomPool_test OK\n");
    return 0;
}
