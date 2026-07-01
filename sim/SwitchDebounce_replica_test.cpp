#include <cstdint>
#include <cstdio>

struct SwitchDebounceReplica {
    uint8_t  state_       = 0x00;
    uint32_t last_update_ = 0;
    bool     flip_        = true;

    void reset() {
        state_       = 0x00;
        last_update_ = 0;
    }

    void debounce(uint32_t now_ms, bool pressed) {
        if (now_ms - last_update_ < 1) {
            return;
        }
        last_update_ = now_ms;
        const bool raw = !pressed;
        state_         = static_cast<uint8_t>((state_ << 1) | (flip_ ? !raw : raw));
    }

    bool risingEdge() const { return state_ == 0x7f; }
    bool fallingEdge() const { return state_ == 0x80; }
    bool pressedP() const { return state_ == 0xff; }
};

struct RunResult {
    int  rising_edges = 0;
    int  first_rising_ms = -1;
    uint8_t final_state = 0;
};

static RunResult runSequence(SwitchDebounceReplica& sw,
                             uint32_t start_ms,
                             uint32_t end_ms,
                             bool pressed) {
    RunResult result;
    for (uint32_t now_ms = start_ms; now_ms <= end_ms; ++now_ms) {
        sw.debounce(now_ms, pressed);
        if (sw.risingEdge()) {
            ++result.rising_edges;
            if (result.first_rising_ms < 0) {
                result.first_rising_ms = static_cast<int>(now_ms);
            }
        }
    }
    result.final_state = sw.state_;
    return result;
}

static void printScenarioA() {
    SwitchDebounceReplica sw;
    const RunResult result = runSequence(sw, 0, 200, false);
    std::printf("SCENARIO A total_rising_edges=%d\n", result.rising_edges);
}

static void printScenarioB() {
    SwitchDebounceReplica sw;
    const RunResult result = runSequence(sw, 0, 200, true);
    std::printf("SCENARIO B first_rising_ms=%d total_rising_edges=%d final_state=0x%02x\n",
                result.first_rising_ms,
                result.rising_edges,
                result.final_state);
}

static void printScenarioC() {
    for (int hold = 1; hold <= 8; ++hold) {
        SwitchDebounceReplica sw;
        int rising_edges = 0;
        for (int i = 0; i < hold; ++i) {
            sw.debounce(static_cast<uint32_t>(i + 1), true);
            if (sw.risingEdge()) {
                ++rising_edges;
            }
        }
        for (int i = 0; i < 10; ++i) {
            sw.debounce(static_cast<uint32_t>(hold + 1 + i), false);
            if (sw.risingEdge()) {
                ++rising_edges;
            }
        }
        std::printf("SCENARIO C HOLD=%d rising_edges=%d\n", hold, rising_edges);
    }
}

static void printScenarioD() {
    const int steps[] = {1, 2, 5, 8, 12, 20};
    for (const int step : steps) {
        SwitchDebounceReplica sw;
        int rising_edges = 0;
        for (int iteration = 0; iteration < 40; ++iteration) {
            const uint32_t now_ms = static_cast<uint32_t>(iteration * step);
            const bool pressed =
                now_ms >= 5 && now_ms <= 15;
            sw.debounce(now_ms, pressed);
            if (sw.risingEdge()) {
                ++rising_edges;
            }
        }
        std::printf("SCENARIO D STEP=%d rising_edges=%d\n", step, rising_edges);
    }
}

int main() {
    std::printf("SwitchDebounce replica test\n");
    printScenarioA();
    printScenarioB();
    printScenarioC();
    printScenarioD();
    std::printf("SUMMARY\n");
    std::printf("END\n");
    return 0;
}
