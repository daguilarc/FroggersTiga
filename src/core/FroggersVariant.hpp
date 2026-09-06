#pragma once

// Build-time variant identity. Each app directory's Makefile defines exactly
// one FROGGERS_VARIANT_* before including ../mk/daisy.mk, and the two programs
// are built and flashed separately -- the differences below are resolved by the
// preprocessor, so neither binary carries the other's code.
//
// Solo has the reverb page. Guitar does not, and sums the dry external signal
// with the ring mod instead of being replaced by it.

#if !defined(FROGGERS_VARIANT_GUITAR) && !defined(FROGGERS_VARIANT_SOLO)
#define FROGGERS_VARIANT_SOLO 1
#endif

#if defined(FROGGERS_VARIANT_GUITAR)
#define FROGGERS_HAS_REVERB 0
#define FROGGERS_EXTERNAL_DRY_PARALLEL 1
#else
#define FROGGERS_HAS_REVERB 1
#define FROGGERS_EXTERNAL_DRY_PARALLEL 0
#endif
