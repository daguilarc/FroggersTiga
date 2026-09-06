// Included once per variant translation unit, after that unit has defined its
// FROGGERS_VARIANT_*. The two units differ only in that define.
//
// The engine's layout differs between variants -- Solo carries the reverb delay
// lines and Guitar does not -- so two variant units in one binary would be an
// ODR violation, and the linker would pick one definition for both. Pulling the
// engine in through an anonymous namespace gives each unit its own internal
// type. The standard headers are included first, at global scope, so that they
// stay there.

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>

namespace
{
#include "AudioPairArState.hpp"
#include "FroggersEngine.hpp"
} // namespace

#include "VariantMix_api.hpp"

namespace
{
// The engine carries the reverb delay lines by value on Solo, so it is too
// large for the stack. Config() runs once: it allocates the pages, and running
// it twice would keep allocating them.
PageManager& Pages()
{
    static PageManager pageManager;
    return pageManager;
}

FroggersEngine& Engine()
{
    PageManager& pageManager = Pages();
    static FroggersEngine engine;
    static bool configured = false;
    if (!configured)
    {
        configured = true;
        engine.Config(&pageManager);
        engine.SetSampleRate(48000.0f);
    }
    return engine;
}


variantmix::Probe OneMix(float input, float v1, float v2, float v3, float olvl, bool hasExternal)
{
    FroggersEngine& e = Engine();
    e.m_pairAr = nullptr;
    e.m_vcoAdsr = nullptr;
    e.m_adsrParams = nullptr;
    e.m_sampleRate = 48000.0f;
    const float mix = e.MixExternalAndOsc(input, v1, v2, v3, olvl, hasExternal);
    return {mix, e.m_pair12.level, e.m_pair23.level};
}

variantmix::Probe PairArRun(float input, float v1, float v2, float v3, int n)
{
    static AudioPairArState pairAr;
    pairAr.init(48000.0f);
    FroggersEngine& e = Engine();
    e.m_pairAr = &pairAr;
    e.m_vcoAdsr = nullptr;
    e.m_adsrParams = nullptr;
    e.m_sampleRate = 48000.0f;
    e.m_pair12.level = 0.0f;
    e.m_pair23.level = 0.0f;
    float mix = 0.0f;
    for (int i = 0; i < n; i++)
    {
        mix = e.MixExternalAndOsc(input, v1, v2, v3, 0.4f, true);
    }
    e.m_pairAr = nullptr;
    return {mix, e.m_pair12.level, e.m_pair23.level};
}
} // namespace
