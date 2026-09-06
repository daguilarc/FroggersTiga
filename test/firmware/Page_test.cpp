// PageManager mediates every control surface interaction: turning a knob,
// assigning a mod source and depth, randomizing a page, and stepping between
// pages. Each of these asserts the round trip through the getter PageManager
// itself exposes, not just that a field got written.

#include "Page.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>

#include "Check.hpp"

int main()
{
    // KnobUpdate moves the current page's parameter value, read back through
    // GetParamCurrentPage. SetAllParamsTracking puts every parameter into the
    // tracking state KnobUpdate requires to move the value at all -- without
    // it, a fresh Idle parameter only follows a knob that already sits within
    // its snap epsilon.
    {
        PageManager pageManager;
        pageManager.AddPage();
        pageManager.InitParam("ROW0", 0, 0, 0.2f);
        pageManager.SetAllParamsTracking();
        pageManager.KnobUpdate(0, 0.8f);
        float value = pageManager.GetParamCurrentPage(0);
        Check(std::fabs(value - 0.8f) < 1e-6f, "KnobUpdate moves the current page's parameter value");
        std::printf("KnobUpdate(0, 0.8) -> GetParamCurrentPage(0) = %f\n", value);
    }

    // SetPageModSource/SetPageModDepth/GetPageModDepth round-trip a source
    // index and a depth. Index 4 is a valid sim-assignable mod index
    // (IsSimAssignableModIndex in SimModSource.hpp).
    {
        PageManager pageManager;
        pageManager.AddPage();
        pageManager.InitParam("ROW0", 0, 0, 0.0f);
        pageManager.SetPageModSource(0, 0, 4);
        uint8_t source = pageManager.GetPageModSource(0, 0);
        Check(source == 4, "SetPageModSource round-trips through GetPageModSource");
        pageManager.SetPageModDepth(0, 0, 0.6f);
        float depth = pageManager.GetPageModDepth(0, 0);
        Check(std::fabs(depth - 0.6f) < 1e-6f, "SetPageModDepth round-trips through GetPageModDepth");
        std::printf("mod source=%d depth=%f\n", source, depth);
    }

    // RandomizeCurrentPage changes at least one row's value, and never the
    // FUEG row (Parameter::Randomize returns early when the name is "FUEG").
    {
        PageManager pageManager;
        Page* page = pageManager.AddPage();
        const uint8_t crispyRow = 7;
        for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
        {
            if (row != crispyRow)
            {
                page->InitParam("ROWX", row, 0.3f);
            }
        }
        page->SetFuegoization(crispyRow);

        float before[Parameter::x_numParameters];
        for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
        {
            before[row] = page->m_parameters[row].m_knobValue;
        }

        pageManager.RandomizeCurrentPage();

        int changed = 0;
        for (uint8_t row = 0; row < Parameter::x_numParameters; row++)
        {
            float after = page->m_parameters[row].m_knobValue;
            if (row == crispyRow)
            {
                Check(after == before[row], "RandomizeCurrentPage leaves the FUEG row untouched");
            }
            else if (after != before[row])
            {
                changed++;
            }
        }
        Check(changed > 0, "RandomizeCurrentPage changes at least one non-FUEG row");
        std::printf("RandomizeCurrentPage changed %d of %d non-FUEG rows\n",
                     changed, static_cast<int>(Parameter::x_numParameters) - 1);
    }

    // PageNext/PagePrevious wrap around the page count.
    {
        PageManager pageManager;
        for (int i = 0; i < 3; i++)
        {
            pageManager.AddPage();
        }
        for (uint8_t position = 0; position < Parameter::x_numParameters; position++)
        {
            pageManager.KnobUpdate(position, 0.0f);
        }
        pageManager.SelectPage(0);
        Check(pageManager.m_currentPage == 0, "starts on page 0");

        pageManager.PagePrevious();
        Check(pageManager.m_currentPage == 2, "PagePrevious wraps from page 0 to the last page");

        pageManager.PageNext();
        Check(pageManager.m_currentPage == 0, "PageNext wraps from the last page back to page 0");

        pageManager.PageNext();
        pageManager.PageNext();
        pageManager.PageNext();
        Check(pageManager.m_currentPage == 0, "PageNext cycles through all pages back to page 0");
        std::printf("final page after wraps: %d\n", pageManager.m_currentPage);
    }

    if (g_failures == 0)
    {
        std::printf("PASS: Page_test\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
