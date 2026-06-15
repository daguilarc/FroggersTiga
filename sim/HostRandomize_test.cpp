#include "AudioPairArLayout.hpp"
#include "HostRandomize.hpp"
#include "PagedHostIO.hpp"

#include <cmath>
#include <cstdio>

static bool knobsAllEqual(const AudioPairArState& pairAr, float value)
{
    for (uint8_t i = 0; i < AudioPairArState::kCount; i++)
    {
        if (std::fabs(pairAr.getKnob(i) - value) > 0.001f)
        {
            return false;
        }
    }
    return true;
}

static bool anyKnobDiffers(const AudioPairArState& before, const AudioPairArState& after)
{
    for (uint8_t i = 0; i < AudioPairArState::kCount; i++)
    {
        if (std::fabs(before.getKnob(i) - after.getKnob(i)) > 0.001f)
        {
            return true;
        }
    }
    return false;
}

int main()
{
    PagedHostIO io;
    io.Init();

    if (!knobsAllEqual(io.m_pairAr, 0.5f))
    {
        std::printf("FAIL: pair-AR knobs expected init 0.5\n");
        return 1;
    }

    const AudioPairArState beforePage = io.m_pairAr;
    io.RandomizePage(AudioPairArLayout::kAudioHostPage);
    if (!anyKnobDiffers(beforePage, io.m_pairAr))
    {
        std::printf("FAIL: Audio page Randomize did not change pair-AR knobs\n");
        return 1;
    }

    const uint8_t modBefore = io.m_pairAr.getModSource(0);
    const float depthBefore = io.m_pairAr.getModDepth(0);
    io.RandomizePageMod(AudioPairArLayout::kAudioHostPage);
    if (io.m_pairAr.getModSource(0) == modBefore && io.m_pairAr.getModDepth(0) == depthBefore)
    {
        std::printf("FAIL: Audio page Randmod did not change pair-AR mod route 0\n");
        return 1;
    }

    const AudioPairArState beforeAll = io.m_pairAr;
    io.RandomizeAllPages();
    if (!anyKnobDiffers(beforeAll, io.m_pairAr))
    {
        std::printf("FAIL: global RandomizeAllPages did not change pair-AR knobs\n");
        return 1;
    }

    const AudioPairArState beforeNonAudio = io.m_pairAr;
    io.RandomizePage(1);
    if (anyKnobDiffers(beforeNonAudio, io.m_pairAr))
    {
        std::printf("FAIL: non-audio page Randomize should not touch pair-AR\n");
        return 1;
    }

    std::printf("HostRandomize_test OK\n");
    return 0;
}
