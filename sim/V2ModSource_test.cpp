#include "CvMidiBridge.hpp"
#include "Page.hpp"
#include "SimModSource.hpp"
#include "V2ModSourceCatalog.hpp"
#include "V2ParamDisplayNames.hpp"

#include <cstdio>
#include <cstring>

int main()
{
    CvMidiBridge bridge;
    bridge.setCcPairEnabled(0, true);

    if (!IsSimModSourceAvailable(3, bridge, SimHostKind::DesktopV2))
    {
        std::printf("FAIL: DesktopV2 mod index 3 (VCO 1 EF) should be available\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(0, bridge, SimHostKind::DesktopV2))
    {
        std::printf("FAIL: DesktopV2 mod index 0 (VCO 1+2) should be available\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(11, bridge, SimHostKind::VstV2))
    {
        std::printf("FAIL: VstV2 mod index 11 (Random S&H 1) should be available\n");
        return 1;
    }

    if (!IsSimModSourceAvailable(12, bridge, SimHostKind::VstV2))
    {
        std::printf("FAIL: VstV2 mod index 12 (Random S&H 2) should be available\n");
        return 1;
    }

    if (IsSimModSourceAvailable(3, bridge, SimHostKind::Desktop))
    {
        std::printf("FAIL: v1 Desktop mod index 3 should be unavailable\n");
        return 1;
    }

    if (V2ParamDisplayNames::CrispyRowForPage(0) != 7)
    {
        std::printf("FAIL: CrispyRowForPage(Audio) expected 7 got %u\n",
                    V2ParamDisplayNames::CrispyRowForPage(0));
        return 1;
    }

    for (uint8_t page = 1; page <= 5; ++page)
    {
        if (V2ParamDisplayNames::CrispyRowForPage(page) != 9)
        {
            std::printf("FAIL: CrispyRowForPage(%u) expected 9 got %u\n",
                        page,
                        V2ParamDisplayNames::CrispyRowForPage(page));
            return 1;
        }
    }

    if (V2ParamDisplayNames::CrispyRowForPage(6) != 6)
    {
        std::printf("FAIL: CrispyRowForPage(Pair-AR) expected 6 got %u\n",
                    V2ParamDisplayNames::CrispyRowForPage(6));
        return 1;
    }

    if (std::strcmp(permanentModSourceDisplayName(3), "VCO 1 EF") != 0)
    {
        std::printf("FAIL: permanentModSourceDisplayName(3) mismatch\n");
        return 1;
    }

    if (std::strcmp(V2ParamDisplayNames::forHostPageRow(3, 7), "Comb/Peak") != 0)
    {
        std::printf("FAIL: Filter row 7 label expected Comb/Peak\n");
        return 1;
    }

    if (std::strcmp(V2ParamDisplayNames::forHostPageRow(1, 7), "Crispy") == 0)
    {
        std::printf("FAIL: Random row 7 must not be v1 Crispy\n");
        return 1;
    }

    if (std::strcmp(permanentModSourceDisplayName(11), "Random S&H 1") != 0
        || std::strcmp(permanentModSourceDisplayName(12), "Random S&H 2") != 0)
    {
        std::printf("FAIL: permanentModSourceDisplayName Random S&H mismatch\n");
        return 1;
    }

    PageManager pageManager;
    pageManager.SetPageModSource(1, 0, 11);
    if (pageManager.GetPageModSource(1, 0) != 11)
    {
        std::printf("FAIL: SetPageModSource should persist mod index 11\n");
        return 1;
    }

    std::printf("V2ModSource_test OK\n");
    return 0;
}
