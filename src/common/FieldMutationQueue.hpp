#pragma once

#include "Page.hpp"

enum class FieldMutationType : uint8_t
{
    RandAll,
    RandAllMod,
};

struct FieldMutation
{
    FieldMutationType type = FieldMutationType::RandAll;
};

struct FieldMutationQueue
{
    static constexpr int kDepth = 8;

    FieldMutation m_queue[kDepth]{};
    int m_write = 0;
    int m_read = 0;
    uint8_t m_page = 0;

    void Enqueue(FieldMutationType type)
    {
        if (m_write > m_read)
        {
            const FieldMutation& last = m_queue[(m_write - 1) % kDepth];
            if (last.type == type)
            {
                return;
            }
        }
        if (m_write - m_read >= kDepth)
        {
            return;
        }
        m_queue[m_write % kDepth].type = type;
        m_write++;
    }

    // A whole Rand All is one page of work per call, not all of them: running
    // every page inside a single drain is what stalls the poll loop under
    // repeated presses. The entry is retired only once its last page is done,
    // so coalescing in Enqueue still collapses a held button.
    bool DrainOne(PageManager& pageManager)
    {
        if (m_read >= m_write)
        {
            return false;
        }
        const FieldMutation mutation = m_queue[m_read % kDepth];
        if (mutation.type == FieldMutationType::RandAll)
        {
            pageManager.RandomizePageFromKnobs(m_page);
        }
        else
        {
            pageManager.RandomizePageModFromKnobs(m_page);
        }
        m_page++;
        if (m_page >= pageManager.m_numPages)
        {
            m_page = 0;
            m_read++;
        }
        return true;
    }
};
