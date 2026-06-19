#pragma once

#include "AudioPairArLayout.hpp"
#include "AudioPairArState.hpp"
#include "CvMidiBridge.hpp"
#include "Page.hpp"

#include <cstdint>

inline void RandomizePageWithExtras(PageManager& pm,
                                    uint8_t page,
                                    AudioPairArState& pairAr)
{
    pm.RandomizePage(page);
    if (page == AudioPairArLayout::kAudioHostPage)
    {
        pairAr.randomizeKnobs();
    }
}

inline void RandomizePageModWithExtras(PageManager& pm,
                                       uint8_t page,
                                       AudioPairArState& pairAr,
                                       const CvMidiBridge& bridge,
                                       SimHostKind hostKind)
{
    pm.RandomizePageModSim(page, bridge, hostKind);
    if (page == AudioPairArLayout::kAudioHostPage)
    {
        pairAr.randomizeMod(bridge, hostKind);
    }
}

inline void RandomizeAllPagesWithPairAr(PageManager& pm, AudioPairArState& pairAr)
{
    pm.RandomizeAllPages();
    pairAr.randomizeKnobs();
}

inline void RandomizeAllPagesIndependentWithPairAr(PageManager& pm, AudioPairArState& pairAr)
{
    pm.RandomizeAllPagesIndependent();
    pairAr.randomizeKnobs();
}
