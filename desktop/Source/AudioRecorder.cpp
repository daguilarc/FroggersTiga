#include "AudioRecorder.h"

#include "OwnedAllocationGuard.hpp"

#include <chrono>
#include <cstring>

AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder()
{
    stop();
}

void AudioRecorder::truncateAndStop()
{
    m_truncated.store(true, std::memory_order_relaxed);
    m_active.store(false, std::memory_order_relaxed);
}

void AudioRecorder::start()
{
    stop();
    m_buffer.clear();
    m_buffer.reserve(kGrowChunk * 2);
    m_sampleCount.store(0, std::memory_order_relaxed);
    m_producerAcceptedSamples = 0;
    m_testMaxSamples = kMaxSamples;
    m_truncated.store(false, std::memory_order_relaxed);
    m_writeHead.store(0, std::memory_order_relaxed);
    m_readTail.store(0, std::memory_order_relaxed);
    m_currentSlot = 0;
    m_currentFill = 0;
    m_stopConsumer.store(false, std::memory_order_relaxed);
    m_active.store(true, std::memory_order_relaxed);
    m_consumerThread = std::thread(&AudioRecorder::consumerLoop, this);
}

void AudioRecorder::stop()
{
    m_active.store(false, std::memory_order_relaxed);
    if (m_currentFill > 0)
    {
        pushFilledSlot(m_currentSlot, m_currentFill);
        m_currentFill = 0;
    }
    m_stopConsumer.store(true, std::memory_order_relaxed);
    m_wakeCv.notify_one();
    if (m_consumerThread.joinable())
    {
        m_consumerThread.join();
    }
}

bool AudioRecorder::pushFilledSlot(size_t slotIndex, size_t frameCount)
{
    const size_t head = m_writeHead.load(std::memory_order_relaxed);
    const size_t tail = m_readTail.load(std::memory_order_acquire);
    if (head - tail >= kPoolChunkCount)
    {
        return false;
    }
    m_readyFrameCounts[slotIndex] = frameCount;
    m_writeHead.store(head + 1, std::memory_order_release);
    m_wakeCv.notify_one();
    return true;
}

void AudioRecorder::appendSlotToBuffer(const ChunkSlot& slot, size_t frameCount)
{
    const size_t floatCount = frameCount * 2;
    const size_t baseOffset = m_buffer.size();
    m_buffer.resize(baseOffset + floatCount);
    std::memcpy(m_buffer.data() + baseOffset, slot.interleaved.data(), floatCount * sizeof(float));
    m_sampleCount.store(m_buffer.size() / 2, std::memory_order_relaxed);
}

void AudioRecorder::consumerLoop()
{
    while (true)
    {
#if defined(FROGGERS_RECORDER_TESTING)
        if (m_testBlockConsumer.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
#endif
        const size_t tail = m_readTail.load(std::memory_order_relaxed);
        const size_t head = m_writeHead.load(std::memory_order_acquire);
        if (tail < head)
        {
            const size_t slotIndex = tail % kPoolChunkCount;
            appendSlotToBuffer(m_pool[slotIndex], m_readyFrameCounts[slotIndex]);
            m_readTail.store(tail + 1, std::memory_order_release);
            continue;
        }
        if (m_stopConsumer.load(std::memory_order_acquire))
        {
            break;
        }
        std::unique_lock<std::mutex> lock(m_wakeMutex);
        m_wakeCv.wait_for(lock, std::chrono::milliseconds(5));
    }

    while (m_readTail.load(std::memory_order_relaxed) < m_writeHead.load(std::memory_order_acquire))
    {
        const size_t tail = m_readTail.load(std::memory_order_relaxed);
        const size_t slotIndex = tail % kPoolChunkCount;
        appendSlotToBuffer(m_pool[slotIndex], m_readyFrameCounts[slotIndex]);
        m_readTail.store(tail + 1, std::memory_order_release);
    }
}

bool AudioRecorder::appendStereo(const float* left, const float* right, size_t numSamples)
{
    FROGGERS_OWNED_ALLOCATION_GUARD();
    if (!m_active.load(std::memory_order_acquire) || !left)
    {
        return false;
    }
    const float* r = right != nullptr ? right : left;

    for (size_t i = 0; i < numSamples; ++i)
    {
        if (m_producerAcceptedSamples >= m_testMaxSamples)
        {
            truncateAndStop();
            return false;
        }
        if (m_currentFill >= kPoolChunkFrames)
        {
            if (!pushFilledSlot(m_currentSlot, m_currentFill))
            {
                truncateAndStop();
                return false;
            }
            m_currentSlot = m_writeHead.load(std::memory_order_relaxed) % kPoolChunkCount;
            m_currentFill = 0;
        }

        const size_t base = m_currentFill * 2;
        m_pool[m_currentSlot].interleaved[base] = left[i];
        m_pool[m_currentSlot].interleaved[base + 1] = r[i];
        ++m_currentFill;
        ++m_producerAcceptedSamples;
    }
    return true;
}
