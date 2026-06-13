#pragma once

#include <cstddef>
#include <vector>

class AudioRecorder
{
public:
    static constexpr size_t kMaxSamples = 44100 * 60 * 30;
    static constexpr size_t kGrowChunk = 44100 * 10;

    void start();
    void stop();
    bool isActive() const { return m_active; }
    bool appendStereo(const float* left, const float* right, size_t numSamples);
    const std::vector<float>& getInterleaved() const { return m_buffer; }
    size_t getSampleCount() const { return m_sampleCount; }
    bool wasTruncated() const { return m_truncated; }

private:
    std::vector<float> m_buffer;
    size_t m_sampleCount = 0;
    bool m_active = false;
    bool m_truncated = false;
};
