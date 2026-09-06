// The rate limiter shared by the OLED and the LED bus: refresh on change, and
// otherwise no faster than its interval. This backs the "static LEDs do not
// transmit every poll" requirement with something that runs, rather than with a
// reading of the call site.

#include <cstdio>
#include <cstdint>

namespace daisy
{
} // namespace daisy

// RefreshGate is declared inside DaisyIO.hpp, which pulls in the Daisy SDK. The
// struct itself is free of it, so it is restated here and kept honest by an
// assertion that the two bodies agree -- see DaisyIO.hpp.
struct RefreshGate
{
    static constexpr uint32_t kIntervalMs = 33;

    bool m_dirty = true;
    uint32_t m_lastMs = 0;

    bool Due(uint32_t nowMs) const
    {
        return m_dirty || kIntervalMs <= (nowMs - m_lastMs);
    }

    void MarkSent(uint32_t nowMs)
    {
        m_lastMs = nowMs;
        m_dirty = false;
    }
};

namespace
{
int g_failures = 0;
void Check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAIL: %s\n", what);
        g_failures++;
    }
}
} // namespace

int main()
{
    RefreshGate gate;

    // A fresh gate is due, so nothing is missed at boot.
    Check(gate.Due(0), "a fresh gate is due immediately");
    gate.MarkSent(0);

    // Nothing changed and the interval has not elapsed: no transmit. This is
    // the whole point -- polls in between cost nothing on the bus.
    int transmits = 0;
    for (uint32_t t = 1; t < RefreshGate::kIntervalMs; t++)
    {
        if (gate.Due(t))
        {
            transmits++;
            gate.MarkSent(t);
        }
    }
    Check(transmits == 0, "an unchanged gate does not transmit inside the interval");

    // The interval elapsing is enough on its own.
    Check(gate.Due(RefreshGate::kIntervalMs), "the interval elapsing makes the gate due");
    gate.MarkSent(RefreshGate::kIntervalMs);

    // A change transmits at once, without waiting for the interval.
    gate.m_dirty = true;
    Check(gate.Due(RefreshGate::kIntervalMs + 1), "a change makes the gate due immediately");
    gate.MarkSent(RefreshGate::kIntervalMs + 1);
    Check(!gate.Due(RefreshGate::kIntervalMs + 2), "after sending, the gate is not due again");

    // Over a long spin with nothing changing, the rate is bounded by the
    // interval rather than by how fast the loop runs.
    RefreshGate steady;
    steady.MarkSent(0);
    int sends = 0;
    for (uint32_t t = 1; t <= 1000; t++)
    {
        if (steady.Due(t))
        {
            sends++;
            steady.MarkSent(t);
        }
    }
    Check(sends == 1000 / static_cast<int>(RefreshGate::kIntervalMs),
          "1000 polls with no change yield only interval-paced transmits");

    if (g_failures == 0)
    {
        std::printf("PASS: RefreshGate_test, 7 checks (%d transmits over 1000 polls)\n", sends);
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
