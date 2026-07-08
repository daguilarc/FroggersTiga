#include "Fuegoize.hpp"
#include "PagedHostIO.hpp"
#include "VcvSectionAdapter.hpp"
#include "VcvVcoArExtension.hpp"

#include <cmath>
#include <cstdio>
#include <string>

static bool expectNear(float actual, float expected, const char* label)
{
    if (std::fabs(actual - expected) > 0.001f)
    {
        std::printf("FAIL: %s expected %f got %f\n", label, expected, actual);
        return false;
    }
    return true;
}

static bool expectTrue(bool value, const char* label)
{
    if (!value)
    {
        std::printf("FAIL: %s\n", label);
        return false;
    }
    return true;
}

int main()
{
    PagedHostIO host;
    host.m_hostKind = SimHostKind::Vcv;
    host.Init();
    DelayState delay;
    delay.init(44100.0f);
    VcvSectionAdapter adapter(host, delay);

    const uint8_t originalPage = host.m_pageManager.m_currentPage;
    host.m_pageManager.m_knobPositions[1] = 0.91f;
    adapter.setSectionBaseValue(VcvSection::Filter, 1, 0.23f);
    if (!expectTrue(host.m_pageManager.m_currentPage == originalPage,
                    "section write changed current page"))
    {
        return 1;
    }
    if (!expectNear(host.m_pageManager.m_knobPositions[1], 0.91f, "section write mutated knob latch"))
    {
        return 1;
    }
    if (!expectNear(host.m_pageManager.m_pages[3].m_parameters[1].m_knobValue,
                    0.23f,
                    "section base write"))
    {
        return 1;
    }

    host.m_pageManager.m_modMgr.m_mods[5] = 0.4f;
    adapter.setSectionBaseValue(VcvSection::Filter, 2, 0.25f);
    adapter.setSectionInternalRoute(VcvSection::Filter, 2, 5, 1.0f);
    adapter.applyRowEffective(VcvSection::Filter, 2, 0.25f, false, 0.0f, 0.0f);
    const Parameter& filtered = host.m_pageManager.m_pages[3].m_parameters[2];
    if (!expectNear(host.GetPageParam(3, 2), 0.4f, "disconnected internal effective"))
    {
        return 1;
    }
    if (!expectNear(filtered.m_knobValue, 0.25f, "stored base preserved"))
    {
        return 1;
    }
    if (!expectTrue(filtered.m_modIndex == 5, "stored route preserved"))
    {
        return 1;
    }
    if (!expectNear(filtered.m_modAmount, 1.0f, "stored depth preserved"))
    {
        return 1;
    }

    adapter.clearEffectiveOverrides();
    adapter.applyRowEffective(VcvSection::Filter, 2, 0.25f, true, 5.0f, 0.0f);
    if (!expectNear(host.GetPageParam(3, 2), 0.9f, "connected CV combines once"))
    {
        return 1;
    }
    adapter.clearEffectiveOverrides();

    const float crunchyOff = effectiveVcvGlobalCrunchy(0.25f, false, 10.0f);
    const float crunchyRaised = effectiveVcvGlobalCrunchy(0.25f, true, 5.0f);
    const float crunchyHigh = effectiveVcvGlobalCrunchy(0.8f, true, 5.0f);
    const float crunchyLow = effectiveVcvGlobalCrunchy(0.1f, true, -5.0f);
    if (!expectNear(crunchyOff, 0.25f, "global crunchy disconnected"))
    {
        return 1;
    }
    if (!expectNear(crunchyRaised, 0.75f, "global crunchy positive CV"))
    {
        return 1;
    }
    if (!expectNear(crunchyHigh, 1.0f, "global crunchy clamp high"))
    {
        return 1;
    }
    if (!expectNear(crunchyLow, 0.0f, "global crunchy clamp low"))
    {
        return 1;
    }

    adapter.setSectionBaseValue(VcvSection::Drive, 0, 0.42f);
    adapter.setSectionBaseValue(VcvSection::Drive, 7, 0.75f);
    adapter.applyRowEffective(VcvSection::Drive, 0, 0.42f, false, 0.0f, 0.5f);
    const float expectedOrder = Fuegoize(Fuegoize(0.42f, 0.5f, 0), 0.75f, 0);
    if (!expectNear(host.GetPageParam(4, 0), expectedOrder, "global crunchy before section crispy"))
    {
        return 1;
    }
    adapter.clearEffectiveOverrides();

    host.m_pageManager.m_knobPositions[0] = 0.0f;
    host.m_pageManager.m_knobPositions[1] = 0.0f;
    adapter.setSectionBaseValue(VcvSection::Drive, 0, 0.11f);
    adapter.setSectionBaseValue(VcvSection::Drive, 1, 0.22f);
    adapter.randomizeAllSections();
    const float randomizedA = host.m_pageManager.m_pages[4].m_parameters[0].m_knobValue;
    const float randomizedB = host.m_pageManager.m_pages[4].m_parameters[1].m_knobValue;
    if (!expectTrue(std::fabs(randomizedA - 0.11f) > 0.001f
                        || std::fabs(randomizedB - 0.22f) > 0.001f,
                    "random all did not update section state"))
    {
        return 1;
    }
    if (!expectNear(host.m_pageManager.m_knobPositions[0], 0.0f, "random all mutated latch 0"))
    {
        return 1;
    }
    if (!expectNear(host.m_pageManager.m_knobPositions[1], 0.0f, "random all mutated latch 1"))
    {
        return 1;
    }

    VcvVcoArExtension::Snapshot vcoAr = VcvVcoArExtension::defaultSnapshot();
    if (!expectNear(vcoAr.rows[0].base, 0.5f, "VCO AR default attack"))
    {
        return 1;
    }
    if (!expectNear(vcoAr.rows[VcvVcoArExtension::kCrispyRow].base, 0.0f, "VCO AR default crispy"))
    {
        return 1;
    }
    if (!expectTrue(std::string(VcvVcoArExtension::rowLabel(4)) == "Atk VCO3",
                    "VCO AR row label"))
    {
        return 1;
    }

    std::printf("VcvSectionAdapter_test OK\n");
    return 0;
}
