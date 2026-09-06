#pragma once

// Every firmware test counts its failed checks through this helper: Check
// prints a FAIL line and increments g_failures when the condition is false.

#include <cstdio>

inline int g_failures = 0;

inline void Check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAIL: %s\n", what);
        g_failures++;
    }
}
