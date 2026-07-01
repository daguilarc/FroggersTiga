#include "Page.hpp"
#include "FroggersEngine.hpp"
#include <cstdio>
#include <cstring>

int main() {
    int fails = 0;
    PageManager pm;
    FroggersEngine eng;
    eng.Config(&pm);
    for (int i = 0; i < (int)Parameter::x_numParameters; i++) pm.m_knobPositions[i] = 0.5f;
    pm.Finalize();

    printf("H1 numPages=%d (expect 5)\n", (int)pm.m_numPages);
    printf("H1 currentPage=%d (expect 0)\n", (int)pm.m_currentPage);
    printf("H1 row0 name='%s' (expect V1VO)\n", pm.GetNameCurrentPage(0));
    if (pm.m_numPages != 5) fails++;
    if (pm.m_currentPage != 0) fails++;
    if (strcmp(pm.GetNameCurrentPage(0), "V1VO") != 0) fails++;

    for (int p = 0; p < (int)pm.m_numPages; p++)
        printf("  page %d row0='%s'\n", p, pm.m_pages[p].m_parameters[0].GetName());

    pm.SelectPage(0);
    printf("H2 SW2/PageNext from 0: ");
    for (int k = 0; k < 8; k++) { pm.PageNext(); printf("%d ", (int)pm.m_currentPage); }
    printf("\n");

    pm.SelectPage(0);
    printf("H3 SW1/PagePrevious from 0: ");
    int prev = (int)pm.m_currentPage; int stuck = 0;
    for (int k = 0; k < 8; k++) { pm.PagePrevious(); printf("%d ", (int)pm.m_currentPage); if ((int)pm.m_currentPage==prev) stuck++; prev=(int)pm.m_currentPage; }
    printf("\n");
    printf("H3 SW1 stuck count=%d (expect 0)\n", stuck);
    if (stuck != 0) fails++;

    pm.SelectPage(2);
    pm.StartModTracking(0);
    printf("H4 before: page=%d modIndex=%d\n", (int)pm.m_currentPage, (int)pm.m_modIndex);
    pm.PagePrevious();
    printf("H4 after SW1: page=%d modIndex=%d (expect page=1 modIndex=255)\n", (int)pm.m_currentPage, (int)pm.m_modIndex);
    if (pm.m_currentPage != 1 || (int)pm.m_modIndex != 255) fails++;

    printf("RESULT fails=%d %s\n", fails, fails ? "FAIL" : "ALL PASS");
    return fails ? 1 : 0;
}
