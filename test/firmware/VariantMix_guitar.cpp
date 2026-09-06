#define FROGGERS_VARIANT_GUITAR 1

#include "VariantMix_body.inl"

namespace variantmix
{
Probe GuitarMix(float input, float v1, float v2, float v3, float olvl, bool hasExternal)
{
    return OneMix(input, v1, v2, v3, olvl, hasExternal);
}

bool GuitarHasReverb()
{
    return FROGGERS_HAS_REVERB != 0;
}

int GuitarPageCount()
{
    Engine();
    return Pages().m_numPages;
}
} // namespace variantmix
