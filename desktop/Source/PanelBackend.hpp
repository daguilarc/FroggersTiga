#pragma once

#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "ParamDisplayNames.hpp"

#include <cstdint>

struct IPanelBackend
{
    virtual ~IPanelBackend() = default;
    virtual void setKnob(uint8_t row, float value) = 0;
    virtual float getKnob(uint8_t row) const = 0;
    virtual float getEffectiveKnob(uint8_t row) const = 0;
    virtual void setModSource(uint8_t row, uint8_t modIndex) = 0;
    virtual void setModDepth(uint8_t row, float depth) = 0;
    virtual uint8_t getModSource(uint8_t row) const = 0;
    virtual float getModDepth(uint8_t row) const = 0;
    virtual const char* getRowName(uint8_t row) const = 0;
    virtual void randomizeKnobs() = 0;
    virtual void randomizeMod() = 0;
    virtual bool hasVcoWaveButtons() const = 0;
    virtual float getVcoMorph(size_t index) const = 0;
    virtual float getVcoDisplayMorph(size_t index) const = 0;
    virtual void cycleVcoMorph(size_t index) = 0;
    virtual DesktopHostIO* desktopHost()
    {
        return nullptr;
    }
    virtual bool hasPairArBand() const
    {
        return false;
    }
    virtual void setPairArKnob(uint8_t index, float value)
    {
        (void)index;
        (void)value;
    }
    virtual float getPairArKnob(uint8_t index) const
    {
        (void)index;
        return 0.5f;
    }
    virtual float getPairArEffectiveKnob(uint8_t index) const
    {
        (void)index;
        return 0.5f;
    }
    virtual void setPairArModSource(uint8_t index, uint8_t modIndex)
    {
        (void)index;
        (void)modIndex;
    }
    virtual void setPairArModDepth(uint8_t index, float depth)
    {
        (void)index;
        (void)depth;
    }
    virtual uint8_t getPairArModSource(uint8_t index) const
    {
        (void)index;
        return 255;
    }
    virtual float getPairArModDepth(uint8_t index) const
    {
        (void)index;
        return 0.5f;
    }
    virtual const char* getPairArName(uint8_t index) const
    {
        (void)index;
        return "";
    }
};

struct DesktopPanelBackend : IPanelBackend
{
    DesktopPanelBackend(uint8_t pageIndex, DesktopHostIO& host)
        : m_pageIndex(pageIndex)
        , m_host(host)
    {
    }

    void setKnob(uint8_t row, float value) override
    {
        m_host.SetPageKnob(m_pageIndex, row, value);
    }

    float getKnob(uint8_t row) const override
    {
        return m_host.GetPageParam(m_pageIndex, row);
    }

    float getEffectiveKnob(uint8_t row) const override
    {
        return m_host.GetPageParam(m_pageIndex, row);
    }

    void setModSource(uint8_t row, uint8_t modIndex) override
    {
        m_host.SetPageModSource(m_pageIndex, row, modIndex);
    }

    void setModDepth(uint8_t row, float depth) override
    {
        m_host.SetPageModDepth(m_pageIndex, row, depth);
    }

    uint8_t getModSource(uint8_t row) const override
    {
        return m_host.GetPageModSource(m_pageIndex, row);
    }

    float getModDepth(uint8_t row) const override
    {
        return m_host.GetPageModDepth(m_pageIndex, row);
    }

    const char* getRowName(uint8_t row) const override
    {
        return ParamDisplayNames::forHostPageRow(m_pageIndex, row);
    }

    void randomizeKnobs() override
    {
        m_host.EnqueueRandomizePanel(m_pageIndex);
    }

    void randomizeMod() override
    {
        m_host.EnqueueRandomizePanelMod(m_pageIndex);
    }

    bool hasVcoWaveButtons() const override
    {
        return m_pageIndex == 0;
    }

    float getVcoMorph(size_t index) const override
    {
        return m_host.GetVcoMorph(index);
    }

    float getVcoDisplayMorph(size_t index) const override
    {
        return m_host.GetVcoDisplayMorph(index);
    }

    void cycleVcoMorph(size_t index) override
    {
        m_host.CycleVcoMorph(index);
    }

    DesktopHostIO* desktopHost() override
    {
        return &m_host;
    }

    bool hasPairArBand() const override
    {
        return m_pageIndex == 0;
    }

    void setPairArKnob(uint8_t index, float value) override
    {
        if (m_pageIndex == 0)
        {
            m_host.SetAudioPairArKnob(index, value);
        }
    }

    float getPairArKnob(uint8_t index) const override
    {
        return m_pageIndex == 0 ? m_host.GetAudioPairArKnob(index) : 0.5f;
    }

    float getPairArEffectiveKnob(uint8_t index) const override
    {
        return m_pageIndex == 0 ? m_host.GetAudioPairArEffective(index) : 0.5f;
    }

    void setPairArModSource(uint8_t index, uint8_t modIndex) override
    {
        if (m_pageIndex == 0)
        {
            m_host.SetAudioPairArModSource(index, modIndex);
        }
    }

    void setPairArModDepth(uint8_t index, float depth) override
    {
        if (m_pageIndex == 0)
        {
            m_host.SetAudioPairArModDepth(index, depth);
        }
    }

    uint8_t getPairArModSource(uint8_t index) const override
    {
        return m_pageIndex == 0 ? m_host.GetAudioPairArModSource(index) : 255;
    }

    float getPairArModDepth(uint8_t index) const override
    {
        return m_pageIndex == 0 ? m_host.GetAudioPairArModDepth(index) : 0.5f;
    }

    const char* getPairArName(uint8_t index) const override
    {
        return m_pageIndex == 0 ? ParamDisplayNames::forAudioPairAr(index) : "";
    }

    uint8_t pageIndex() const
    {
        return m_pageIndex;
    }

    DesktopHostIO& host()
    {
        return m_host;
    }

private:
    uint8_t m_pageIndex;
    DesktopHostIO& m_host;
};

struct DelayHostBackend : IPanelBackend
{
    DelayHostBackend(DelayState& state, DesktopHostIO& host)
        : m_state(state)
        , m_host(host)
    {
    }

    void setKnob(uint8_t row, float value) override
    {
        m_state.setKnob(row, value);
    }

    float getKnob(uint8_t row) const override
    {
        return m_state.getKnob(row);
    }

    float getEffectiveKnob(uint8_t row) const override
    {
        return m_state.getEffectiveKnob(row);
    }

    void setModSource(uint8_t row, uint8_t modIndex) override
    {
        m_host.EnqueueDelaySetModSource(row, modIndex);
    }

    void setModDepth(uint8_t row, float depth) override
    {
        m_state.setModDepth(row, depth);
    }

    uint8_t getModSource(uint8_t row) const override
    {
        return m_state.getModSource(row);
    }

    float getModDepth(uint8_t row) const override
    {
        return m_state.getModDepth(row);
    }

    const char* getRowName(uint8_t row) const override
    {
        return ParamDisplayNames::forHostPageRow(ParamDisplayNames::kDelayHostPage, row);
    }

    void randomizeKnobs() override
    {
        m_host.EnqueueDelayRandomizeKnobs();
    }

    void randomizeMod() override
    {
        m_host.EnqueueDelayRandomizeMod();
    }

    bool hasVcoWaveButtons() const override
    {
        return false;
    }

    float getVcoMorph(size_t) const override
    {
        return 0.0f;
    }

    float getVcoDisplayMorph(size_t) const override
    {
        return 0.0f;
    }

    void cycleVcoMorph(size_t) override
    {
    }

    DelayState& state()
    {
        return m_state;
    }

private:
    DelayState& m_state;
    DesktopHostIO& m_host;
};
