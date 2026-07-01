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

    bool DrainOne(PageManager& pageManager)
    {
        if (m_read >= m_write)
        {
            return false;
        }
        const FieldMutation mutation = m_queue[m_read % kDepth];
        m_read++;
        if (mutation.type == FieldMutationType::RandAll)
        {
            pageManager.RandomizeAllPages();
        }
        else
        {
            pageManager.RandomizeAllPagesMod();
        }
        return true;
    }
};
