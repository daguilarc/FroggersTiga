#pragma once

#include "daisy_field.h"

struct FieldSwitchGuard
{
    enum class AuditConfig : uint8_t
    {
        Default,
        Swapped,
        NormalPolarity,
        SwappedNormal,
    };

    bool m_stuckAtBoot[2]{false, false};
    bool m_suppressEdges[2]{false, false};
    AuditConfig m_auditConfig{AuditConfig::Default};

    const char* AuditLabel() const
    {
        switch (m_auditConfig)
        {
        case AuditConfig::Default: return "def";
        case AuditConfig::Swapped: return "swap";
        case AuditConfig::NormalPolarity: return "norm";
        case AuditConfig::SwappedNormal: return "swpi";
        }
        return "def";
    }

    bool SuppressionActive(size_t i) const
    {
        return i < 2 && m_suppressEdges[i];
    }

    bool AllowPageSwitch(size_t i) const
    {
        return !SuppressionActive(i);
    }

    void UpdateSuppression(size_t i, daisy::Switch& sw)
    {
        if (2 <= i || !m_suppressEdges[i])
        {
            return;
        }
        if (sw.FallingEdge())
        {
            m_suppressEdges[i] = false;
        }
    }

    void RunBootAudit(daisy::DaisyField& field)
    {
        int bestScore = -1;
        m_auditConfig = AuditConfig::Default;

        for (size_t t = 0; t < kNumTries; t++)
        {
            ApplySwitchConfig(field, kTries[t]);
            SettleDebouncers(field);

            const int score = RestScore(field);
            if (bestScore < score)
            {
                bestScore = score;
                m_auditConfig = kTries[t].id;
            }
            if (score == 2)
            {
                break;
            }
        }

        ApplySwitchConfig(field, ConfigFor(m_auditConfig));
        SettleDebouncers(field);

        for (size_t i = 0; i < 2; i++)
        {
            if (field.sw[i].RawState())
            {
                m_stuckAtBoot[i] = true;
                m_suppressEdges[i] = true;
            }
        }
    }

  private:
    struct ConfigTry
    {
        AuditConfig id;
        daisy::Pin pin0;
        daisy::Pin pin1;
        daisy::Switch::Polarity pol;
    };

    static constexpr daisy::Pin kPinSw1 = daisy::seed::D30;
    static constexpr daisy::Pin kPinSw2 = daisy::seed::D29;

    static constexpr ConfigTry kTries[] = {
        {AuditConfig::Default, kPinSw1, kPinSw2, daisy::Switch::POLARITY_INVERTED},
        {AuditConfig::Swapped, kPinSw2, kPinSw1, daisy::Switch::POLARITY_INVERTED},
        {AuditConfig::NormalPolarity, kPinSw1, kPinSw2, daisy::Switch::POLARITY_NORMAL},
        {AuditConfig::SwappedNormal, kPinSw2, kPinSw1, daisy::Switch::POLARITY_NORMAL},
    };

    static constexpr size_t kNumTries = sizeof(kTries) / sizeof(kTries[0]);

    static ConfigTry ConfigFor(AuditConfig id)
    {
        for (size_t i = 0; i < kNumTries; i++)
        {
            if (kTries[i].id == id)
            {
                return kTries[i];
            }
        }
        return kTries[0];
    }

    static void ApplySwitchConfig(daisy::DaisyField& field, const ConfigTry& cfg)
    {
        field.sw[0].Init(cfg.pin0,
                         0.f,
                         daisy::Switch::TYPE_MOMENTARY,
                         cfg.pol,
                         daisy::GPIO::Pull::PULLUP);
        field.sw[1].Init(cfg.pin1,
                         0.f,
                         daisy::Switch::TYPE_MOMENTARY,
                         cfg.pol,
                         daisy::GPIO::Pull::PULLUP);
    }

    static void SettleDebouncers(daisy::DaisyField& field)
    {
        for (int n = 0; n < 50; n++)
        {
            field.sw[0].Debounce();
            field.sw[1].Debounce();
            daisy::System::Delay(1);
        }
    }

    static int RestScore(daisy::DaisyField& field)
    {
        int score = 0;
        for (size_t i = 0; i < 2; i++)
        {
            if (!field.sw[i].RawState())
            {
                score++;
            }
        }
        return score;
    }
};
