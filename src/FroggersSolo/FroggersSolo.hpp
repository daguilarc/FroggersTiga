#pragma once

#include "../core/FroggersEngine.hpp"
#include "daisy.h"

using namespace daisy;

struct FroggersTiga : FroggersEngine
{
    void Process(AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out, size_t size)
    {
        ProcessBlock(in[0], out[0], size);
        for (size_t i = 0; i < size; i++)
        {
            out[1][i] = 0.0f;
        }
    }
};
