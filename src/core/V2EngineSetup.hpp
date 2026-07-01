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
    Page* adsrPage = pageManager.AddPage();
    adsrPage->InitParam("A1", 0, 0.05f);
    adsrPage->InitParam("A2", 1, 0.05f);
    adsrPage->InitParam("A3", 2, 0.05f);
    adsrPage->InitParam("S1", 3, 0.8f);
    adsrPage->InitParam("S2", 4, 0.8f);
    adsrPage->InitParam("S3", 5, 0.8f);
    adsrPage->InitParam("R1", 6, 0.2f);
    adsrPage->InitParam("R2", 7, 0.2f);
    adsrPage->InitParam("R3", 8, 0.2f);
    adsrPage->SetFuegoization(V2ParamDisplayNames::CrispyRowForPage(6));
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
