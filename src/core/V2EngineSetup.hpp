#pragma once

#include "Page.hpp"
#include "V2ParamDisplayNames.hpp"

#include <array>
#include <cstdint>

namespace V2EngineSetup
{
inline void initExpandedRows(Page& page, uint8_t pageIndex)
{
    page.InitParam("X7", 7, 0.5f);
    page.InitParam("X8", 8, 0.5f);
    page.SetFuegoization(V2ParamDisplayNames::CrispyRowForPage(pageIndex));
}

inline void configureExpandedModulePages(PageManager& pageManager)
{
    static constexpr std::array<uint8_t, 4> kExpandedPages = {1, 2, 3, 4};
    for (uint8_t pageIndex : kExpandedPages)
    {
        initExpandedRows(pageManager.m_pages[pageIndex], pageIndex);
    }
}

inline void configureAdsrPage(PageManager& pageManager)
{
    // Task 7.5 (D15): per-VCO triplet row order -- Attack, Sustain, Release
    // for VCO1, then VCO2, then VCO3 -- matching FroggersEngine::MixOscVoices'
    // GetParam(0..8) reads. Crispy lives at row 9 (x_numParameters == 10).
    Page* adsrPage = pageManager.AddPage();
    adsrPage->InitParam("A1", 0, 0.05f);
    adsrPage->InitParam("S1", 1, 0.8f);
    adsrPage->InitParam("R1", 2, 0.2f);
    adsrPage->InitParam("A2", 3, 0.05f);
    adsrPage->InitParam("S2", 4, 0.8f);
    adsrPage->InitParam("R2", 5, 0.2f);
    adsrPage->InitParam("A3", 6, 0.05f);
    adsrPage->InitParam("S3", 7, 0.8f);
    adsrPage->InitParam("R3", 8, 0.2f);
    adsrPage->SetFuegoization(9);
}

inline void configure(PageManager& pageManager, bool includeAdsrPage)
{
    configureExpandedModulePages(pageManager);
    if (includeAdsrPage)
    {
        configureAdsrPage(pageManager);
    }
}
} // namespace V2EngineSetup
