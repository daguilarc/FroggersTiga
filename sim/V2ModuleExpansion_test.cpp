#include "PagedHostIO.hpp"
#include "V2ParamDisplayNames.hpp"
#include "WasmSimHost.hpp"

#include <cstdio>
#include <cstring>

int main()
{
    PagedHostIO webIo;
    webIo.m_hostKind = SimHostKind::Web;
    webIo.Init();

    if (webIo.GetNumPages() != 5)
    {
        std::printf("FAIL: expected 5 core pages on web, got %u\n", webIo.GetNumPages());
        return 1;
    }

    if (std::strcmp(webIo.GetPageRowName(1, 7), "Spread") != 0
        || std::strcmp(webIo.GetPageRowName(1, 8), "Bias") != 0
        || std::strcmp(webIo.GetPageRowName(1, 9), "Crispy") != 0)
    {
        std::printf("FAIL: random page expansion labels mismatch\n");
        return 1;
    }

    if (std::strcmp(webIo.GetPageRowName(3, 7), "Comb/Peak") != 0
        || std::strcmp(webIo.GetPageRowName(3, 8), "Scoop") != 0)
    {
        std::printf("FAIL: filter expansion labels mismatch\n");
        return 1;
    }

    webIo.SetPageKnob(2, 0, 0.73f);
    webIo.SetPageKnob(2, V2ParamDisplayNames::CrispyRowForPage(2), 0.62f);
    webIo.SetGlobalCrunchy(0.41f);
    const float valueWithCrunchy = webIo.GetPageParam(2, 0);
    webIo.SetGlobalCrunchy(0.0f);
    const float valueWithoutCrunchy = webIo.GetPageParam(2, 0);
    if (valueWithCrunchy == valueWithoutCrunchy)
    {
        std::printf("FAIL: global crunchy should affect page params\n");
        return 1;
    }

    WasmSimHost host;
    if (std::strcmp(DelayState::rowName(7), "Color") != 0
        || std::strcmp(DelayState::rowName(8), "Halo") != 0
        || std::strcmp(DelayState::rowName(9), "Crispy") != 0)
    {
        std::printf("FAIL: delay expansion labels mismatch\n");
        return 1;
    }
    host.io.SetGlobalCrunchy(0.3f);
    if (host.io.GetGlobalCrunchy() <= 0.0f)
    {
        std::printf("FAIL: web global crunchy setter/getter mismatch\n");
        return 1;
    }

    std::printf("V2ModuleExpansion_test OK\n");
    return 0;
}
