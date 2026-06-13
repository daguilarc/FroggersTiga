#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>

struct MidiCcEvent
{
    uint8_t channel;
    uint8_t cc;
    uint8_t value;
};

struct MidiNoteEvent
{
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    bool isNoteOn;
};

struct CvMidiBridge
{
    static constexpr int kMidiNoteQueueSize = 64;
    static constexpr int kMidiCcQueueSize = 64;
    static constexpr int kQwertyPitchLow = 60;
    static constexpr float kQwertyPitchSpan = 16.0f;

    uint8_t m_outChannel = 0;
    uint8_t m_outCc = 74;
    uint8_t m_inChannel = 0;
    uint8_t m_inCc = 1;
    std::array<uint8_t, 128> m_heldVelocity{};
    std::array<MidiNoteEvent, kMidiNoteQueueSize> m_noteQueue{};
    std::atomic<int> m_noteWrite{0};
    std::atomic<int> m_noteRead{0};
    std::array<MidiCcEvent, kMidiCcQueueSize> m_ccQueue{};
    std::atomic<int> m_ccWrite{0};
    std::atomic<int> m_ccRead{0};
    float m_modLevelFromNotes = 0.0f;

    void recomputeModLevelFromHeldNotes()
    {
        uint8_t maxVel = 0;
        int highestNote = -1;
        for (size_t i = 0; i < m_heldVelocity.size(); i++)
        {
            const uint8_t vel = m_heldVelocity[i];
            if (vel == 0)
            {
                continue;
            }
            if (vel > maxVel)
            {
                maxVel = vel;
            }
            if (highestNote < static_cast<int>(i))
            {
                highestNote = static_cast<int>(i);
            }
        }
        if (highestNote < 0 || maxVel == 0)
        {
            m_modLevelFromNotes = 0.0f;
            return;
        }

        float pitchStep =
            static_cast<float>(highestNote - kQwertyPitchLow + 1) / kQwertyPitchSpan;
        if (pitchStep < 0.0f)
        {
            pitchStep = 0.0f;
        }
        if (pitchStep > 1.0f)
        {
            pitchStep = 1.0f;
        }
        m_modLevelFromNotes = pitchStep * (static_cast<float>(maxVel) / 127.0f);
    }

    void enqueueMidiNote(const MidiNoteEvent& event)
    {
        int w = m_noteWrite.load(std::memory_order_relaxed);
        int r = m_noteRead.load(std::memory_order_acquire);
        if (w - r >= kMidiNoteQueueSize)
        {
            assert(false && "CvMidiBridge note queue overflow");
            m_noteRead.store(r + 1, std::memory_order_release);
        }
        m_noteQueue[static_cast<size_t>(w % kMidiNoteQueueSize)] = event;
        m_noteWrite.store(w + 1, std::memory_order_release);
    }

    void enqueueMidiCc(const MidiCcEvent& event)
    {
        int w = m_ccWrite.load(std::memory_order_relaxed);
        int r = m_ccRead.load(std::memory_order_acquire);
        if (w - r >= kMidiCcQueueSize)
        {
            assert(false && "CvMidiBridge CC queue overflow");
            m_ccRead.store(r + 1, std::memory_order_release);
        }
        m_ccQueue[static_cast<size_t>(w % kMidiCcQueueSize)] = event;
        m_ccWrite.store(w + 1, std::memory_order_release);
    }

    void drainNoteQueue()
    {
        while (true)
        {
            const int r = m_noteRead.load(std::memory_order_relaxed);
            const int w = m_noteWrite.load(std::memory_order_acquire);
            if (r >= w)
            {
                break;
            }
            const MidiNoteEvent& event = m_noteQueue[static_cast<size_t>(r % kMidiNoteQueueSize)];
            if (event.note < m_heldVelocity.size())
            {
                if (event.isNoteOn)
                {
                    m_heldVelocity[static_cast<size_t>(event.note)] =
                        event.velocity > 0 ? event.velocity : 127;
                }
                else
                {
                    m_heldVelocity[static_cast<size_t>(event.note)] = 0;
                }
            }
            m_noteRead.store(r + 1, std::memory_order_release);
        }
    }

    void drainCcQueue(float* mods, size_t modCount)
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
            if (modCount > 0 && event.channel == m_inChannel && event.cc == m_inCc)
            {
                mods[0] = static_cast<float>(event.value) / 127.0f;
            }
            m_ccRead.store(r + 1, std::memory_order_release);
        }
    }

    void PushMidiNote(uint8_t channel, uint8_t note, uint8_t velocity, bool isNoteOn)
    {
        if (channel != m_inChannel || note >= m_heldVelocity.size())
        {
            return;
        }
        enqueueMidiNote({channel, note, velocity, isNoteOn});
    }

    void PushMidiCc(uint8_t channel, uint8_t cc, uint8_t value)
    {
        enqueueMidiCc({channel, cc, value});
    }

    void drainMidiIn(float* mods, size_t modCount)
    {
        drainNoteQueue();
        recomputeModLevelFromHeldNotes();
        if (modCount > 0)
        {
            mods[0] = m_modLevelFromNotes;
        }
        drainCcQueue(mods, modCount);
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
