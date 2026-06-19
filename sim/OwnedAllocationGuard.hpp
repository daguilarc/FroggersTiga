#pragma once

// Lightweight owned-allocation instrumentation for Froggers realtime paths.
//
// Scope: repository-controlled heap activity in guarded native/WASM callbacks
// (JUCE render, parameter apply, recorder producer append, WasmSimHost/VCV process).
//
// Out of scope: browser-internal structured-clone allocation and garbage collection
// in Web Audio worklet telemetry — those are not repository-controlled and are
// never counted by this instrumentation (see realtime-audio-safety spec).

#include <cassert>
#include <cstddef>

namespace froggers
{
#if defined(FROGGERS_OWNED_ALLOCATION_INSTRUMENT) || !defined(NDEBUG)
constexpr bool kOwnedAllocationInstrumentActive = true;
#else
constexpr bool kOwnedAllocationInstrumentActive = false;
#endif

struct OwnedAllocationCounter
{
    static thread_local int depth;
    static thread_local std::size_t allocations;
    static thread_local std::size_t deallocations;
    static thread_local std::size_t capacityGrowth;

    static void enter();
    static void leave();
    static void recordAllocation();
    static void recordDeallocation();
    static void recordCapacityGrowth();
    static void assertZeroOwnedActivity();
};

inline thread_local int OwnedAllocationCounter::depth = 0;
inline thread_local std::size_t OwnedAllocationCounter::allocations = 0;
inline thread_local std::size_t OwnedAllocationCounter::deallocations = 0;
inline thread_local std::size_t OwnedAllocationCounter::capacityGrowth = 0;

#if defined(FROGGERS_OWNED_ALLOCATION_INSTRUMENT) || !defined(NDEBUG)

inline void OwnedAllocationCounter::enter()
{
    if (depth++ == 0)
    {
        allocations = 0;
        deallocations = 0;
        capacityGrowth = 0;
    }
}

inline void OwnedAllocationCounter::leave()
{
    if (depth > 0)
    {
        --depth;
    }
}

inline void OwnedAllocationCounter::recordAllocation()
{
    if (depth > 0)
    {
        ++allocations;
    }
}

inline void OwnedAllocationCounter::recordDeallocation()
{
    if (depth > 0)
    {
        ++deallocations;
    }
}

inline void OwnedAllocationCounter::recordCapacityGrowth()
{
    if (depth > 0)
    {
        ++capacityGrowth;
    }
}

inline void OwnedAllocationCounter::assertZeroOwnedActivity()
{
    if (depth != 1)
    {
        return;
    }
    assert(allocations == 0 && deallocations == 0 && capacityGrowth == 0);
}

#else

inline void OwnedAllocationCounter::enter() {}
inline void OwnedAllocationCounter::leave() {}
inline void OwnedAllocationCounter::recordAllocation() {}
inline void OwnedAllocationCounter::recordDeallocation() {}
inline void OwnedAllocationCounter::recordCapacityGrowth() {}
inline void OwnedAllocationCounter::assertZeroOwnedActivity() {}

#endif

class OwnedAllocationGuard
{
public:
    OwnedAllocationGuard() { OwnedAllocationCounter::enter(); }

    ~OwnedAllocationGuard()
    {
        OwnedAllocationCounter::assertZeroOwnedActivity();
        OwnedAllocationCounter::leave();
    }

    OwnedAllocationGuard(const OwnedAllocationGuard&) = delete;
    OwnedAllocationGuard& operator=(const OwnedAllocationGuard&) = delete;
};
} // namespace froggers

#if defined(FROGGERS_OWNED_ALLOCATION_INSTRUMENT) || !defined(NDEBUG)
#define FROGGERS_OWNED_ALLOCATION_ASSERT() froggers::OwnedAllocationCounter::assertZeroOwnedActivity()
#define FROGGERS_OWNED_ALLOCATION_GUARD() froggers::OwnedAllocationGuard froggersOwnedAllocationGuard_
#else
#define FROGGERS_OWNED_ALLOCATION_ASSERT() ((void)0)
#define FROGGERS_OWNED_ALLOCATION_GUARD() ((void)0)
#endif
