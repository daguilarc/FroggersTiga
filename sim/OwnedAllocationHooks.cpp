#include "OwnedAllocationGuard.hpp"

#if defined(FROGGERS_OWNED_ALLOCATION_INSTRUMENT)

#include <cstdlib>
#include <new>

void* operator new(std::size_t count)
{
    froggers::OwnedAllocationCounter::recordAllocation();
    if (void* ptr = std::malloc(count))
    {
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t count)
{
    froggers::OwnedAllocationCounter::recordAllocation();
    if (void* ptr = std::malloc(count))
    {
        return ptr;
    }
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept
{
    if (ptr != nullptr)
    {
        froggers::OwnedAllocationCounter::recordDeallocation();
    }
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    if (ptr != nullptr)
    {
        froggers::OwnedAllocationCounter::recordDeallocation();
    }
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    operator delete[](ptr);
}

#endif
