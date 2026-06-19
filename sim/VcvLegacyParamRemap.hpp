#pragma once

#include <cstdint>

namespace VcvLegacyParamRemap
{
inline constexpr int kSchemaVersionV1 = 1;
inline constexpr int kSchemaVersionV2 = 2;
inline constexpr int kDroppedParamId = -1;

inline bool shouldApplyLegacyRemap(int schemaVersion)
{
    return schemaVersion < kSchemaVersionV2;
}

inline int remapV1ParamId(int oldId)
{
    if (oldId == 1 || oldId == 2)
    {
        return kDroppedParamId;
    }
    if (oldId >= 3)
    {
        return oldId - 2;
    }
    return oldId;
}

inline int resolveLoadParamId(int paramId, int schemaVersion)
{
    if (!shouldApplyLegacyRemap(schemaVersion))
    {
        return paramId;
    }
    return remapV1ParamId(paramId);
}

} // namespace VcvLegacyParamRemap
