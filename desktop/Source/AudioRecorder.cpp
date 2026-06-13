#include "AudioRecorder.h"

void AudioRecorder::start()
{
    m_buffer.clear();
    m_buffer.reserve(kGrowChunk * 2);
    m_sampleCount = 0;
    m_truncated = false;
    m_active = true;
}

void AudioRecorder::stop()
{
    m_active = false;
}

bool AudioRecorder::appendStereo(const float* left, const float* right, size_t numSamples)
{
    if (!m_active || !left)
    {
        return false;
    }
    const float* r = right != nullptr ? right : left;
    for (size_t i = 0; i < numSamples; ++i)
    {
        if (m_sampleCount >= kMaxSamples)
        {
            m_truncated = true;
            m_active = false;
            return false;
        }
        if (m_buffer.size() < (m_sampleCount + 1) * 2)
        {
            m_buffer.resize(m_buffer.size() + kGrowChunk * 2, 0.0f);
        }
        const size_t base = m_sampleCount * 2;
        m_buffer[base] = left[i];
        m_buffer[base + 1] = r[i];
        ++m_sampleCount;
    }
    return true;
}
