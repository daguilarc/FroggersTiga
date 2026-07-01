#pragma once

#include "Fuegoize.hpp"

#include <cstdint>

namespace V2FuegoStack
{
inline float ApplyGlobal(float value, float globalCrunchy, uint8_t row)
{
    return Fuegoize(value, globalCrunchy, row);
}

inline float ApplyMusicalRow(float value,
                             float globalCrunchy,
                             float crispyKnobPreFuego,
                             uint8_t row,
                             uint8_t crispyRow)
{
    const float afterCrunchy = ApplyGlobal(value, globalCrunchy, row);
    const float crispyAfterCrunchy = ApplyGlobal(crispyKnobPreFuego, globalCrunchy, crispyRow);
    return Fuegoize(afterCrunchy, crispyAfterCrunchy, row);
}
} // namespace V2FuegoStack
