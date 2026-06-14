#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>

struct MidiCcEvent
{
    uint8_t pair;
    uint8_t value;
};

struct CvMidiBridge
{
    static constexpr int kMidiCcQueueSize = 64;
    static constexpr uint8_t kCcModIndices[2] = {0, 1};

    uint8_t m_outChannel = 0;
    uint8_t m_outCc = 74;
    uint8_t m_inChannel1 = 0;
    uint8_t m_inCc1 = 1;
    uint8_t m_inChannel2 = 0;
    uint8_t m_inCc2 = 2;
    float m_inCcLevel1 = 0.0f;
    float m_inCcLevel2 = 0.0f;
    bool m_inCcEnabled[2] = {true, false};
    std::array<MidiCcEvent, kMidiCcQueueSize> m_ccQueue{};
    std::atomic<int> m_ccWrite{0};
    std::atomic<int> m_ccRead{0};

    void enqueueMidiCc(uint8_t pair, uint8_t value)
    {
        int w = m_ccWrite.load(std::memory_order_relaxed);
        int r = m_ccRead.load(std::memory_order_acquire);
        if (w - r >= kMidiCcQueueSize)
        {
            assert(false && "CvMidiBridge CC queue overflow");
            m_ccRead.store(r + 1, std::memory_order_release);
        }
        m_ccQueue[static_cast<size_t>(w % kMidiCcQueueSize)] = {pair, value};
        m_ccWrite.store(w + 1, std::memory_order_release);
    }

    void drainCcQueue()
    {
        while (true)
        {
            const int r = m_ccRead.load(std::memory_order_relaxed);
            const int w = m_ccWrite.load(std::memory_order_acquire);
            if (r >= w)
            {
                break;
            }
            const MidiCcEvent& event = m_ccQueue[static_cast<size_t>(r % kMidiCcQueueSize)];
            const float level = static_cast<float>(event.value) / 127.0f;
            if (event.pair == 1)
            {
                m_inCcLevel1 = level;
            }
            else if (event.pair == 2)
            {
                m_inCcLevel2 = level;
            }
            m_ccRead.store(r + 1, std::memory_order_release);
        }
    }

    bool isCcPairEnabled(uint8_t pairIndex) const
    {
        return pairIndex < 2 && m_inCcEnabled[pairIndex];
    }

    bool isCcModIndexEnabled(uint8_t modIndex) const
    {
        for (uint8_t i = 0; i < 2; ++i)
        {
            if (kCcModIndices[i] == modIndex)
            {
                return m_inCcEnabled[i];
            }
        }
        return false;
    }

    void setCcPairEnabled(uint8_t pairIndex, bool enabled)
    {
        if (pairIndex >= 2)
        {
            return;
        }
        m_inCcEnabled[pairIndex] = enabled;
        if (!enabled)
        {
            if (pairIndex == 0)
            {
                m_inCcLevel1 = 0.0f;
            }
            else
            {
                m_inCcLevel2 = 0.0f;
            }
        }
    }

    void PushMidiCc(uint8_t channel, uint8_t cc, uint8_t value)
    {
        if (channel == m_inChannel1 && cc == m_inCc1)
        {
            if (isCcPairEnabled(0))
            {
                enqueueMidiCc(1, value);
            }
            return;
        }
        if (channel == m_inChannel2 && cc == m_inCc2)
        {
            if (isCcPairEnabled(1))
            {
                enqueueMidiCc(2, value);
            }
        }
    }

    void drainMidiIn(float* mods, size_t modCount)
    {
        drainCcQueue();
        if (modCount > 0)
        {
            mods[0] = isCcPairEnabled(0) ? m_inCcLevel1 : 0.0f;
        }
        if (modCount > 1)
        {
            mods[1] = isCcPairEnabled(1) ? m_inCcLevel2 : 0.0f;
        }
    }

    void tickMidiOut(float envelope,
                     const std::function<void(uint8_t channel, uint8_t cc, uint8_t value)>& send)
    {
        if (!send)
        {
            return;
        }
        float clamped = envelope < 0.0f ? 0.0f : (envelope > 1.0f ? 1.0f : envelope);
        send(m_outChannel, m_outCc, static_cast<uint8_t>(clamped * 127.0f));
    }
};
