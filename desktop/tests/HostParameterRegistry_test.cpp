#include "HostParameterInventory.hpp"

#include <cstdio>
#include <cstring>

int main()
{
    if (HostParameterInventory::kCount != 107)
    {
        std::printf("FAIL: expected 107 host parameters, got %zu\n", HostParameterInventory::kCount);
        return 1;
    }

    if (!HostParameterInventory::validateInventory())
    {
        std::printf("FAIL: inventory descriptor table failed validation\n");
        return 1;
    }

    for (size_t i = 0; i < HostParameterInventory::kCount; ++i)
    {
        for (size_t j = i + 1; j < HostParameterInventory::kCount; ++j)
        {
            if (std::strcmp(HostParameterInventory::kDescriptors[i].stableKey,
                            HostParameterInventory::kDescriptors[j].stableKey)
                == 0)
            {
                std::printf("FAIL: duplicate stable key %s\n",
                            HostParameterInventory::kDescriptors[i].stableKey);
                return 1;
            }
        }
    }

    std::printf("PASS: HostParameterInventory (%zu parameters)\n", HostParameterInventory::kCount);
    return 0;
}
