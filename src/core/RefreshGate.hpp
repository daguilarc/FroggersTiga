#pragma once

#include <cstdint>

// The OLED and the LED bus refresh on the same terms: immediately when
// something changed, and otherwise no faster than ~30 Hz. Both are outputs
// whose cost the poll loop pays, so both are rationed the same way.
struct RefreshGate
{
    static constexpr uint32_t kIntervalMs = 33;

    bool m_dirty = true;
    uint32_t m_lastMs = 0;

    bool Due(uint32_t nowMs) const
    {
        return m_dirty || kIntervalMs <= (nowMs - m_lastMs);
    }

    void MarkSent(uint32_t nowMs)
    {
        m_lastMs = nowMs;
        m_dirty = false;
    }
};
