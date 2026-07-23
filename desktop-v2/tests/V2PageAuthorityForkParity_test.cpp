// Drift-guard for the D10 page-label authority fork.
//
// desktop-v2/Source/V2DesktopPageDisplayNames.hpp (6 pages, no Random module page)
// is a deliberate fork of the shared sim/V2ParamDisplayNames.hpp (7 pages incl.
// Random), pending the web-v2 reconciliation. The fork is intentional ONLY on the
// Random page; every OTHER page (Audio + Reverb/Filter/Drive/Delay + Pair-AR) must
// stay byte-identical across the two tables. This test asserts that agreement so a
// future edit to one table that forgets the other is caught before web-v2 migration.
//
// Page-index mapping (desktop UI page -> shared 7-page index; the shared table has
// Random at index 1, so everything after Audio is shifted by one):
//   Audio  desktop 0 <-> shared 0
//   Reverb desktop 1 <-> shared 2
//   Filter desktop 2 <-> shared 3
//   Drive  desktop 3 <-> shared 4
//   Delay  desktop 4 <-> shared 5
//   PairAR desktop 5 <-> shared 6

#include "V2DesktopPageDisplayNames.hpp"
#include "V2ParamDisplayNames.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{
struct PagePair
{
    uint8_t desktop;
    uint8_t shared;
    const char* name;
};

constexpr PagePair kSharedPages[] = {
    {0, 0, "Audio"},
    {1, 2, "Reverb"},
    {2, 3, "Filter"},
    {3, 4, "Drive"},
    {4, 5, "Delay"},
    {5, 6, "Pair-AR"},
};

const char* safe(const char* s)
{
    return s != nullptr ? s : "";
}
} // namespace

int main()
{
    int fails = 0;

    // The fork is exactly one page wide: shared has 7 pages, desktop has 6.
    if (V2ParamDisplayNames::kV2NumHostPages != 7 || V2DesktopPageDisplayNames::kV2NumHostPages != 6)
    {
        std::printf("FAIL: unexpected page counts shared=%u desktop=%u (fork should be shared 7 / desktop 6)\n",
                    static_cast<unsigned>(V2ParamDisplayNames::kV2NumHostPages),
                    static_cast<unsigned>(V2DesktopPageDisplayNames::kV2NumHostPages));
        ++fails;
    }

    for (const PagePair& pair : kSharedPages)
    {
        // Page display name agreement.
        if (std::strcmp(safe(V2DesktopPageDisplayNames::forHostPage(pair.desktop)),
                        safe(V2ParamDisplayNames::forHostPage(pair.shared)))
            != 0)
        {
            std::printf("FAIL: %s page name drift desktop[%u]=\"%s\" shared[%u]=\"%s\"\n",
                        pair.name,
                        static_cast<unsigned>(pair.desktop),
                        safe(V2DesktopPageDisplayNames::forHostPage(pair.desktop)),
                        static_cast<unsigned>(pair.shared),
                        safe(V2ParamDisplayNames::forHostPage(pair.shared)));
            ++fails;
        }

        // Crispy-row agreement.
        if (V2DesktopPageDisplayNames::CrispyRowForPage(pair.desktop)
            != V2ParamDisplayNames::CrispyRowForPage(pair.shared))
        {
            std::printf("FAIL: %s crispy row drift desktop=%u shared=%u\n",
                        pair.name,
                        static_cast<unsigned>(V2DesktopPageDisplayNames::CrispyRowForPage(pair.desktop)),
                        static_cast<unsigned>(V2ParamDisplayNames::CrispyRowForPage(pair.shared)));
            ++fails;
        }

        // Row-label agreement across all rows (including out-of-range -> "" on both).
        for (uint8_t row = 0; row < V2DesktopPageDisplayNames::kV2ExpandedNumRows; ++row)
        {
            if (std::strcmp(safe(V2DesktopPageDisplayNames::forHostPageRow(pair.desktop, row)),
                            safe(V2ParamDisplayNames::forHostPageRow(pair.shared, row)))
                != 0)
            {
                std::printf("FAIL: %s row %u label drift desktop=\"%s\" shared=\"%s\"\n",
                            pair.name,
                            static_cast<unsigned>(row),
                            safe(V2DesktopPageDisplayNames::forHostPageRow(pair.desktop, row)),
                            safe(V2ParamDisplayNames::forHostPageRow(pair.shared, row)));
                ++fails;
            }
        }
    }

    if (fails != 0)
    {
        std::printf("V2PageAuthorityForkParity_test FAIL (%d)\n", fails);
        return 1;
    }
    std::printf("V2PageAuthorityForkParity_test OK\n");
    return 0;
}
