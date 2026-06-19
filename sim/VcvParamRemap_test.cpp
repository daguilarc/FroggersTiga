#include "VcvLegacyParamRemap.hpp"

#include <cstdio>

static bool expectRemap(int oldId, int expected, int schemaVersion)
{
    const int actual = VcvLegacyParamRemap::resolveLoadParamId(oldId, schemaVersion);
    if (actual != expected)
    {
        std::printf("FAIL: param %d schema %d expected %d got %d\n",
                    oldId,
                    schemaVersion,
                    expected,
                    actual);
        return false;
    }
    return true;
}

int main()
{
    using namespace VcvLegacyParamRemap;

    if (!expectRemap(0, 0, kSchemaVersionV1))
    {
        return 1;
    }
    if (!expectRemap(1, kDroppedParamId, kSchemaVersionV1))
    {
        return 1;
    }
    if (!expectRemap(2, kDroppedParamId, kSchemaVersionV1))
    {
        return 1;
    }
    if (!expectRemap(3, 1, kSchemaVersionV1))
    {
        return 1;
    }
    if (!expectRemap(4, 2, kSchemaVersionV1))
    {
        return 1;
    }
    if (!expectRemap(10, 8, kSchemaVersionV1))
    {
        return 1;
    }

    if (!expectRemap(0, 0, kSchemaVersionV2))
    {
        return 1;
    }
    if (!expectRemap(1, 1, kSchemaVersionV2))
    {
        return 1;
    }
    if (!expectRemap(3, 3, kSchemaVersionV2))
    {
        return 1;
    }
    if (!expectRemap(10, 10, kSchemaVersionV2))
    {
        return 1;
    }

    const int v2Random = resolveLoadParamId(0, kSchemaVersionV2);
    const int v2RandomAgain = resolveLoadParamId(v2Random, kSchemaVersionV2);
    if (v2RandomAgain != v2Random)
    {
        std::printf("FAIL: v2 param 0 double-load remapped %d -> %d\n", v2Random, v2RandomAgain);
        return 1;
    }

    const int v1Knob = resolveLoadParamId(3, kSchemaVersionV1);
    const int v2Knob = resolveLoadParamId(v1Knob, kSchemaVersionV2);
    if (v2Knob != v1Knob)
    {
        std::printf("FAIL: v1-mapped id %d should stay %d on v2 reload, got %d\n",
                    v1Knob,
                    v1Knob,
                    v2Knob);
        return 1;
    }

    std::printf("VcvParamRemap_test OK\n");
    return 0;
}
