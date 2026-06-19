#pragma once

#include "HostAudioConfig.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

class AudioRecorder
{
public:
    static constexpr size_t kMaxSamples = HostAudioConfig::maxRecordingSamples();
    static constexpr size_t kGrowChunk = static_cast<size_t>(HostAudioConfig::kMaxSampleRate) * 10;
    static constexpr size_t kPoolChunkFrames = 4096;
    static constexpr size_t kPoolChunkCount = 16;

    AudioRecorder();
    ~AudioRecorder();

    void start();
    void stop();
    bool isActive() const { return m_active.load(std::memory_order_relaxed); }
    bool appendStereo(const float* left, const float* right, size_t numSamples);
    const std::vector<float>& getInterleaved() const { return m_buffer; }
    size_t getSampleCount() const { return m_sampleCount.load(std::memory_order_relaxed); }
    bool wasTruncated() const { return m_truncated.load(std::memory_order_relaxed); }

#if defined(FROGGERS_RECORDER_TESTING)
    void setConsumerBlockedForTest(bool blocked) { m_testBlockConsumer.store(blocked); }
    void setMaxSamplesForTest(size_t maxSamples) { m_testMaxSamples = maxSamples; }
#endif

private:
    struct ChunkSlot
    {
        std::array<float, kPoolChunkFrames * 2> interleaved{};
    };

    void consumerLoop();
    bool pushFilledSlot(size_t slotIndex, size_t frameCount);
    void appendSlotToBuffer(const ChunkSlot& slot, size_t frameCount);
    void truncateAndStop();

    std::array<ChunkSlot, kPoolChunkCount> m_pool{};
    std::array<size_t, kPoolChunkCount> m_readyFrameCounts{};
    std::atomic<size_t> m_writeHead{0};
    std::atomic<size_t> m_readTail{0};
    size_t m_currentSlot = 0;
    size_t m_currentFill = 0;
    size_t m_producerAcceptedSamples = 0;

    std::vector<float> m_buffer;
    std::atomic<size_t> m_sampleCount{0};
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_truncated{false};
    std::atomic<bool> m_stopConsumer{false};
    std::atomic<bool> m_testBlockConsumer{false};
    size_t m_testMaxSamples = kMaxSamples;

    std::thread m_consumerThread;
    std::mutex m_wakeMutex;
    std::condition_variable m_wakeCv;
};
