// Drift-guard for the D10 page-label authority fork.
//
// desktop-v2/Source/V2DesktopPageDisplayNames.hpp (6 pages, no Random module page)
// is a deliberate fork of the shared sim/V2ParamDisplayNames.hpp (7 pages incl.
// Random), pending the web-v2 reconciliation. Originally the fork was intentional
// ONLY on the Random page (D10); task 7.4 (D11/D12/D14, operator 2026-07-23) widened
// it to Audio too -- desktop-v2 removes the Cross-coupler row from its Audio page
// (flag-gated to V2 hosts only; the shared engine XCPL param slot is retained, not
// deleted, for Daisy/v1 index stability), while the shared table + web V2 host keep
// Cross-coupler unchanged. Task 7.5 (operator 2026-07-24) widens it again: desktop-v2
// page 5 is renamed "Pair-AR" -> "Envelope" with full-word per-VCO row labels
// (Attack/Release only -- see V2DesktopPageDisplayNames.hpp's file-header note on
// why Sustain rows are not added), while the shared table's page 6 keeps "Pair-AR"
// unchanged for the web/wasm V2 host + v1. Every OTHER page (Reverb/Filter/Drive/
// Delay) must still stay byte-identical across the two tables. This test asserts
// that agreement (Audio and Envelope excluded) so a future edit to one table that
// forgets the other is caught before web-v2 migration.
//
// Page-index mapping (desktop UI page -> shared 7-page index; the shared table has
// Random at index 1, so everything after Audio is shifted by one):
//   Audio    desktop 0 <-> shared 0   (EXCLUDED -- Cross-coupler fork, D11/D14)
//   Reverb   desktop 1 <-> shared 2
//   Filter   desktop 2 <-> shared 3
//   Drive    desktop 3 <-> shared 4
//   Delay    desktop 4 <-> shared 5
//   Envelope desktop 5 <-> shared 6   (EXCLUDED -- Pair-AR->Envelope fork, task 7.5)

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

// Audio (desktop 0 <-> shared 0) and Envelope (desktop 5 <-> shared 6) are
// deliberately excluded (see file header): task 7.4 forked Audio (removing the
// Cross-coupler row from desktop-v2 only), task 7.5 forked Envelope (renaming
// "Pair-AR" -> "Envelope" with full-word row labels, desktop-v2 only).
constexpr PagePair kSharedPages[] = {
    {1, 2, "Reverb"},
    {2, 3, "Filter"},
    {3, 4, "Drive"},
    {4, 5, "Delay"},
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
