#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "SmartGridInclude.hpp"
#include "ModMgr.hpp"

struct ParameterName
{
    static constexpr size_t x_nameSize = 4;
    char m_name[x_nameSize + 1];

    ParameterName()
    {
        m_name[0] = '\0';
    }

    bool IsEmpty() const
    {
        return m_name[0] == '\0';
    }

    void SetName(const char* name)
    {
        size_t length = std::min(strlen(name), x_nameSize);
        memcpy(m_name, name, length);
        m_name[length] = '\0';
    }
};

struct Parameter
{
    static constexpr size_t x_numParameters = 8;
    static constexpr float x_knobEpsilon = 0.001f;

    enum class TrackingState : uint8_t
    {
        Idle = 0,
        Tracking = 1,
        Below = 2,
        Above = 3,
    };

    ParameterName m_name;
    float m_knobValue;
    uint8_t m_position;
    uint8_t m_page;
    TrackingState m_trackingState;
    TrackingState m_modTrackingState;
    uint8_t m_modIndex;
    float m_modAmount;
    bool m_modTrackingArmed;
    Parameter* m_fuegoizationKnob;

    Parameter()
     : m_knobValue(0)
     , m_position(0)
     , m_page(0)
     , m_trackingState(TrackingState::Idle)
     , m_modTrackingState(TrackingState::Idle)
     , m_modIndex(255)
     , m_modAmount(0.0f)
     , m_modTrackingArmed(false)
     , m_fuegoizationKnob(nullptr)
    {
    }

    bool IsEmpty() const
    {
        return m_name.IsEmpty();
    }

    void Init(const char* name, uint8_t pageId, uint8_t position, float defaultValue)
    {
        m_name.SetName(name);
        m_position = position;
        m_page = pageId;
        m_trackingState = TrackingState::Idle;
        m_knobValue = defaultValue;
    }

    const char* GetName() const
    {
        return m_name.m_name;
    }

    float GetPreFuegoization(ModMgr* modMgr)
    {
        if (m_modIndex != 255)
        {
            return modMgr->Modulate(m_knobValue, m_modIndex, m_modAmount);
        }
        else
        {
            return m_knobValue;
        }
    }

    float Get(ModMgr* modMgr)
    {
        float value = GetPreFuegoization(modMgr);
        if (m_fuegoizationKnob)
        {
            float fuegoizationAmount = m_fuegoizationKnob->Get(modMgr);
            uint16_t mask = (1 << static_cast<uint16_t>(std::round(fuegoizationAmount * 8))) - 1;
            uint16_t inputInt = value * 255;
            float inputRemainder = value * 255 - inputInt;
            uint16_t lowerBits = inputInt & mask;

            lowerBits ^= (lowerBits << 3) & mask;
            lowerBits ^= (lowerBits >> 5) & mask;
            lowerBits ^= (lowerBits << 1) & mask;
            uint8_t sh = 1u + (uint8_t)(m_position % ((mask + 1) ? (mask + 1) : 1));
            lowerBits ^= (lowerBits >> sh) & mask;

            inputInt = (inputInt & ~mask) | lowerBits;
            value = (static_cast<float>(inputInt) + inputRemainder) / 255;
        }

        return value;
    }

    bool Close(float knobPosition)
    {
        if (std::abs(knobPosition - m_knobValue) < x_knobEpsilon)
        {
            return true;
        }
        else if (m_fuegoizationKnob)
        {
            float fuegoizationAmount = m_fuegoizationKnob->m_knobValue;
            uint16_t mask = ~((1 << static_cast<uint16_t>(std::round(fuegoizationAmount * 8))) - 1);
            uint16_t knobInt = knobPosition * 255;
            uint16_t myKnobInt = m_knobValue * 255;
            return (knobInt & mask) == (myKnobInt & mask);
        }
        else
        {
            return false;
        }
    }

    void PageSelect(float knobPosition)
    {
        if (Close(knobPosition))
        {
            m_trackingState = TrackingState::Tracking;
            m_knobValue = knobPosition;
        }
        else if (knobPosition < m_knobValue)
        {
            m_trackingState = TrackingState::Below;
        }
        else
        {
            m_trackingState = TrackingState::Above;
        }
    }

    void Randomize(float currentKnobPosition)
    {
        if (strcmp(m_name.m_name, "FUEG") == 0)
        {
            return;
        }
        
        RGen rgen;
        m_knobValue = rgen.UniGenRange(0, 1.0f);        
        if (m_trackingState != TrackingState::Idle)
        {
            PageSelect(currentKnobPosition);
        }
    }

    void RandomizeMod(float currentKnobPosition)
    {
        RGen rgen;
        m_modAmount = rgen.UniGenRange(0, 1.0f);
        if (rgen.UniGen() < 0.5f)
        {
            m_modIndex = 255;
        }
        else
        {
            m_modIndex = rgen.RangeGen(ModMgr::x_numMods - 1);
        }
    }

    void PageDeSelect(float knobValue)
    {
        StopModTracking(knobValue);
        m_trackingState = TrackingState::Idle;
    }

    void KnobUpdate(float knobPosition)
    {
        if (m_trackingState == TrackingState::Tracking)
        {
            m_knobValue = knobPosition;
        }
        else if (m_trackingState == TrackingState::Below && m_knobValue < knobPosition)
        {
            m_knobValue = knobPosition;
            m_trackingState = TrackingState::Tracking;
        }
        else if (m_trackingState == TrackingState::Above && knobPosition < m_knobValue)
        {
            m_knobValue = knobPosition;
            m_trackingState = TrackingState::Tracking;
        }
        else if (Close(knobPosition))
        {
            m_trackingState = TrackingState::Tracking;
            m_knobValue = knobPosition;
        }
    }

    void StartModTracking(int modIndex, float modAmount)
    {
        PageDeSelect(modAmount);
        StopModTracking(modAmount);
        if (modIndex == m_modIndex)
        {
            if (std::abs(modAmount - m_modAmount) < x_knobEpsilon)
            {
                m_modTrackingState = TrackingState::Tracking;
                m_modAmount = modAmount;
            }
            else if (modAmount < m_modAmount)
            {
                m_modTrackingState = TrackingState::Below;
            }
            else
            {
                m_modTrackingState = TrackingState::Above;
            }
        }
        else if (modAmount < x_knobEpsilon)
        {
            m_modTrackingArmed = true;
        }   
        else
        {
            m_modTrackingState = TrackingState::Idle;
        }
    }

    void StopModTracking(float knobValue)
    {
        m_modTrackingState = TrackingState::Idle;
        if (m_modAmount < x_knobEpsilon)
        {
            m_modAmount = 0.0f;
            m_modIndex = 255;
        }

        m_modTrackingArmed = false;

        PageSelect(knobValue);
    }

    void ModUpdate(int modIndex, float modAmount)
    {
        if (m_modTrackingState == TrackingState::Tracking)
        {
            m_modAmount = modAmount;
        }
        else if (m_modTrackingArmed && x_knobEpsilon < modAmount)
        {
            m_modTrackingState = TrackingState::Tracking;
            m_modIndex = modIndex;
            m_modAmount = modAmount;
            m_modTrackingArmed = false;
        }
        else if (modIndex == m_modIndex && m_modTrackingState == TrackingState::Below && m_modAmount < modAmount)
        {
            m_modAmount = modAmount;
            m_modTrackingState = TrackingState::Tracking;
        }
        else if (modIndex == m_modIndex && m_modTrackingState == TrackingState::Above && modAmount < m_modAmount)
        {
            m_modAmount = modAmount;
            m_modTrackingState = TrackingState::Tracking;
        }
        else if (modIndex == m_modIndex && std::abs(modAmount - m_modAmount) < x_knobEpsilon)
        {
            m_modTrackingState = TrackingState::Tracking;
            m_modIndex = modIndex;
        }
        else if (modIndex != m_modIndex && modAmount < x_knobEpsilon)
        {
            m_modTrackingArmed = true;
        }
        else
        {
            m_modTrackingState = TrackingState::Idle;
        }
    }

    float GetModAmount(int modIndex)
    {
        if (modIndex == m_modIndex)
        {
            return m_modAmount;
        }
        else
        {
            return 0.0f;
        }
    }

    bool IsTracking()
    {
        return m_trackingState == TrackingState::Tracking;
    }

    bool IsModTracking()
    {
        return m_modTrackingState == TrackingState::Tracking;
    }

    char TrackingBadge()
    {
        switch (m_trackingState)
        {
            case TrackingState::Above:
                return '<';
            case TrackingState::Below:
                return '>';
            case TrackingState::Tracking:
                return ' ';
            case TrackingState::Idle:
                return '-';
            default:
                return '?';
        }
    }

    char ModTrackingBadge()
    {
        switch (m_modTrackingState)
        {
            case TrackingState::Above:
                return '<';
            case TrackingState::Below:
                return '>';
            case TrackingState::Tracking:
                return ' ';
            case TrackingState::Idle:
                return '-';
            default:
                return '?';
        }
    }
};