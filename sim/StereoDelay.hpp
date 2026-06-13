#pragma once

#include "PhaseUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

struct DelayParams
{
    float dtim = 0.0f;
    float dsnd = 0.0f;
    float dfbk = 0.0f;
    float dwid = 0.0f;
    float ddet = 0.0f;
    float dmod = 0.0f;
    float dmix = 0.0f;
};

struct WetPair
{
    float l = 0.0f;
    float r = 0.0f;
};

struct StereoDelay
{
    static constexpr float kMaxDelaySeconds = 2.0f;
    static constexpr size_t kMaxDelaySamples = 96000;
    static constexpr float kMaxDetuneCents = 50.0f;

    void clearBuffers()
    {
        std::fill(m_lineL.begin(), m_lineL.end(), 0.0f);
        std::fill(m_lineR.begin(), m_lineR.end(), 0.0f);
        m_writePos = 0;
        m_lfoPhase = 0.0f;
        m_lastWet = {};
    }

    void setSampleRate(float hz)
    {
        m_sampleRate = hz > 0.0f ? hz : 44100.0f;
        m_capacity = std::min(kMaxDelaySamples,
                              static_cast<size_t>(std::ceil(kMaxDelaySeconds * m_sampleRate)));
        if (m_capacity < 4)
        {
            m_capacity = 4;
        }
        m_lineL.assign(m_capacity, 0.0f);
        m_lineR.assign(m_capacity, 0.0f);
        m_writePos = 0;
        m_lfoPhase = 0.0f;
        m_lfoInc = 2.0f * 3.14159265f * 0.25f / m_sampleRate;
    }

    WetPair process(float bumpIn, const DelayParams& p)
    {
        if (p.dsnd <= 0.0001f || m_capacity == 0)
        {
            m_lastWet = {};
            return m_lastWet;
        }

        const float baseSeconds = PhaseUtils::ExpParam::Compute(0.001f, kMaxDelaySeconds, p.dtim);
        m_lfoPhase += m_lfoInc;
        if (m_lfoPhase > 6.2831853f)
        {
            m_lfoPhase -= 6.2831853f;
        }
        const float modSeconds = std::sin(m_lfoPhase) * p.dmod * baseSeconds * 0.08f;
        const float widthSpread = p.dwid * baseSeconds * 0.35f;
        float timeL = std::max(0.001f, baseSeconds + modSeconds);
        float timeR = std::max(0.001f, baseSeconds + modSeconds + widthSpread);

        const float detune = std::min(std::max(p.ddet, 0.0f), 1.0f);
        const float cents = detune * kMaxDetuneCents;
        const float ratioL = std::pow(2.0f, cents / 1200.0f);
        const float ratioR = std::pow(2.0f, -cents / 1200.0f);
        timeL /= ratioL;
        timeR /= ratioR;

        float dL = readAt(timeL, m_lineL);
        float dR = readAt(timeR, m_lineR);

        const float cross = p.dwid * 0.5f;
        const float fbL = dL * (1.0f - cross) + dR * cross;
        const float fbR = dR * (1.0f - cross) + dL * cross;
        const float fbk = std::min(std::max(p.dfbk, 0.0f), 0.98f);
        const float send = std::min(std::max(p.dsnd, 0.0f), 1.0f);
        const float inSignal = bumpIn * send;

        writeSample(inSignal + fbL * fbk, m_lineL);
        writeSample(inSignal + fbR * fbk, m_lineR);
        advanceWrite();

        m_lastWet.l = dL;
        m_lastWet.r = dR;
        return m_lastWet;
    }

    WetPair getLastWet() const
    {
        return m_lastWet;
    }

    float toReverbMono(float bumpIn, WetPair wet, float dmix) const
    {
        const float mix = std::min(std::max(dmix, 0.0f), 1.0f);
        const float monoWet = (wet.l + wet.r) * 0.5f;
        return (1.0f - mix) * bumpIn + mix * monoWet;
    }

private:
    float readAt(float seconds, const std::vector<float>& line) const
    {
        const float delaySamples = seconds * m_sampleRate;
        const float readPos = static_cast<float>(m_writePos) - delaySamples;
        const size_t idx0 = wrapIndex(static_cast<size_t>(readPos));
        const size_t idx1 = wrapIndex(idx0 + 1);
        const float frac = readPos - std::floor(readPos);
        return line[idx0] * (1.0f - frac) + line[idx1] * frac;
    }

    void writeSample(float sample, std::vector<float>& line)
    {
        line[m_writePos] = sample;
    }

    size_t wrapIndex(size_t idx) const
    {
        while (idx >= m_capacity)
        {
            idx -= m_capacity;
        }
        return idx;
    }

    void advanceWrite()
    {
        m_writePos++;
        if (m_writePos >= m_capacity)
        {
            m_writePos = 0;
        }
    }

    float m_sampleRate = 44100.0f;
    size_t m_capacity = 0;
    size_t m_writePos = 0;
    std::vector<float> m_lineL;
    std::vector<float> m_lineR;
    WetPair m_lastWet{};
    float m_lfoPhase = 0.0f;
    float m_lfoInc = 0.0f;
};
