#pragma once

#include "AudioPairArState.hpp"
#include "DelayState.hpp"
#include "DesktopHostIO.hpp"

#include <cstdint>
#include <cstring>

namespace SimPresetSnapshot
{
constexpr uint32_t kMagic = 0x46544947u;
constexpr uint8_t kVersion = 2;
constexpr uint8_t kVersionV1 = 1;
constexpr uint8_t kMaxPages = 8;
constexpr uint8_t kRows = 8;
constexpr uint8_t kPairArCount = AudioPairArState::kCount;

struct RowState
{
    float knobValue = 0.0f;
    float modAmount = 0.0f;
    uint8_t modIndex = 255;
    uint8_t pad[3]{0, 0, 0};
};

struct SnapshotBodyV1
{
    float knobPositions[kRows]{};
    uint8_t currentPage = 0;
    uint8_t numPages = 0;
    uint8_t pad[2]{0, 0};
    RowState pages[kMaxPages][kRows]{};
    float delayKnobs[kRows]{};
    RowState delayRows[kRows]{};
};

struct SnapshotBody
{
    float knobPositions[kRows]{};
    uint8_t currentPage = 0;
    uint8_t numPages = 0;
    uint8_t pad[2]{0, 0};
    RowState pages[kMaxPages][kRows]{};
    float delayKnobs[kRows]{};
    RowState delayRows[kRows]{};
    RowState pairArRows[kPairArCount]{};
};

inline size_t serializedSize()
{
    return sizeof(uint32_t) + sizeof(uint8_t) + sizeof(SnapshotBody);
}

inline void applyPairArDefaults(AudioPairArState& pairAr)
{
    for (uint8_t i = 0; i < kPairArCount; i++)
    {
        pairAr.setKnob(i, 0.5f);
        pairAr.setModSource(i, 255);
        pairAr.setModDepth(i, 0.5f);
    }
}

inline void writePairArRows(const AudioPairArState& pairAr, RowState* out)
{
    for (uint8_t i = 0; i < kPairArCount; i++)
    {
        out[i].knobValue = pairAr.getKnob(i);
        out[i].modIndex = pairAr.getModSource(i);
        out[i].modAmount = pairAr.getModDepth(i);
    }
}

inline void readPairArRows(DesktopHostIO& host, const RowState* rows)
{
    for (uint8_t i = 0; i < kPairArCount; i++)
    {
        host.SetAudioPairArKnob(i, rows[i].knobValue);
        host.SetAudioPairArModDepth(i, rows[i].modAmount);
        if (rows[i].modIndex != 255)
        {
            host.SetAudioPairArModSource(i, rows[i].modIndex);
        }
    }
}

inline bool write(const DesktopHostIO& host, const DelayState& delay, void* dest, size_t destBytes)
{
    if (!dest || destBytes < serializedSize())
    {
        return false;
    }

    auto* out = static_cast<uint8_t*>(dest);
    const uint32_t magic = kMagic;
    std::memcpy(out, &magic, sizeof(magic));
    out += sizeof(magic);
    *out++ = kVersion;

    SnapshotBody body{};
    const PageManager& pm = host.m_pageManager;
    body.currentPage = pm.m_currentPage;
    body.numPages = pm.m_numPages;
    std::memcpy(body.knobPositions, pm.m_knobPositions, sizeof(body.knobPositions));

    for (uint8_t page = 0; page < body.numPages && page < kMaxPages; page++)
    {
        for (uint8_t row = 0; row < kRows; row++)
        {
            const Parameter& param = pm.m_pages[page].m_parameters[row];
            body.pages[page][row].knobValue = param.m_knobValue;
            body.pages[page][row].modIndex = param.m_modIndex;
            body.pages[page][row].modAmount = param.m_modAmount;
        }
    }

    for (uint8_t row = 0; row < DelayState::kNumRows; row++)
    {
        body.delayKnobs[row] = delay.getKnob(row);
        body.delayRows[row].modIndex = delay.getModSource(row);
        body.delayRows[row].modAmount = delay.getModDepth(row);
        body.delayRows[row].knobValue = delay.getKnob(row);
    }

    writePairArRows(host.m_pairAr, body.pairArRows);

    std::memcpy(out, &body, sizeof(body));
    return true;
}

inline bool readBodyCommon(DesktopHostIO& host, DelayState& delay, const SnapshotBody& body)
{
    PageManager& pm = host.m_pageManager;
    pm.m_currentPage = body.currentPage;
    std::memcpy(pm.m_knobPositions, body.knobPositions, sizeof(body.knobPositions));

    const uint8_t pageCount = body.numPages < kMaxPages ? body.numPages : kMaxPages;
    for (uint8_t page = 0; page < pageCount; page++)
    {
        for (uint8_t row = 0; row < kRows; row++)
        {
            const RowState& rowState = body.pages[page][row];
            Parameter& param = pm.m_pages[page].m_parameters[row];
            param.m_knobValue = rowState.knobValue;
            param.m_modIndex = rowState.modIndex;
            param.m_modAmount = rowState.modAmount;
            pm.KnobUpdateOnPage(page, row, rowState.knobValue);
            if (rowState.modIndex != 255)
            {
                pm.SetPageModSource(page, row, rowState.modIndex);
            }
            pm.SetPageModDepth(page, row, rowState.modAmount);
        }
    }

    for (uint8_t row = 0; row < DelayState::kNumRows; row++)
    {
        delay.setKnob(row, body.delayKnobs[row]);
        const RowState& rowState = body.delayRows[row];
        if (rowState.modIndex == 255)
        {
            delay.setModDepth(row, rowState.modAmount);
        }
        else
        {
            delay.setModSource(row, rowState.modIndex);
            delay.setModDepth(row, rowState.modAmount);
        }
    }

    return true;
}

inline bool read(DesktopHostIO& host, DelayState& delay, const void* src, size_t srcBytes)
{
    if (!src || srcBytes < sizeof(uint32_t) + sizeof(uint8_t))
    {
        return false;
    }

    auto* in = static_cast<const uint8_t*>(src);
    uint32_t magic = 0;
    std::memcpy(&magic, in, sizeof(magic));
    in += sizeof(magic);
    if (magic != kMagic)
    {
        return false;
    }

    const uint8_t version = *in++;
    if (version == kVersionV1)
    {
        if (srcBytes < sizeof(uint32_t) + sizeof(uint8_t) + sizeof(SnapshotBodyV1))
        {
            return false;
        }
        SnapshotBody body{};
        SnapshotBodyV1 bodyV1{};
        std::memcpy(&bodyV1, in, sizeof(bodyV1));
        std::memcpy(body.knobPositions, bodyV1.knobPositions, sizeof(body.knobPositions));
        body.currentPage = bodyV1.currentPage;
        body.numPages = bodyV1.numPages;
        std::memcpy(body.pages, bodyV1.pages, sizeof(body.pages));
        std::memcpy(body.delayKnobs, bodyV1.delayKnobs, sizeof(body.delayKnobs));
        std::memcpy(body.delayRows, bodyV1.delayRows, sizeof(body.delayRows));
        if (!readBodyCommon(host, delay, body))
        {
            return false;
        }
        applyPairArDefaults(host.m_pairAr);
        return true;
    }

    if (version != kVersion)
    {
        return false;
    }

    if (srcBytes < serializedSize())
    {
        return false;
    }

    SnapshotBody body{};
    std::memcpy(&body, in, sizeof(body));
    if (!readBodyCommon(host, delay, body))
    {
        return false;
    }
    readPairArRows(host, body.pairArRows);
    return true;
}
} // namespace SimPresetSnapshot
