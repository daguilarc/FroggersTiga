#include "control/FroggersV2ControlCore.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool nearlyEqual(float a, float b, float eps = 1.0e-4f)
{
    return std::fabs(a - b) < eps;
}

void pushAndProcess(froggers_v2::FroggersV2ControlCore& core, const froggers_v2::MessageIn& message)
{
    core.bus().push(message);
    core.processBus();
    core.compute();
}

bool test_manifest_lane_assignment_persists()
{
    froggers_v2::FroggersV2ControlCore core;

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 8;
    pushAndProcess(core, assign);

    if (core.assignedModSource(0, 0) != 8)
    {
        std::printf("FAIL: LFO 1 lane assignment did not persist\n");
        return false;
    }
    return true;
}

bool test_manifest_lane_affects_effective_value()
{
    froggers_v2::FroggersV2ControlCore core;

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 3;
    pushAndProcess(core, assign);

    froggers_v2::MessageIn press;
    press.type = froggers_v2::MessageIn::Type::ParamPress;
    press.page = 0;
    press.slot = 0;
    pushAndProcess(core, press);
    pushAndProcess(core, froggers_v2::MessageIn::ParamTurn(0, 0, 40.0f));

    froggers_v2::MessageIn sourceClock;
    sourceClock.type = froggers_v2::MessageIn::Type::Clock;
    sourceClock.index = static_cast<uint8_t>(3 + 6);
    sourceClock.value = 1.0f;
    pushAndProcess(core, sourceClock);
    const float withMax = core.effectiveRow(0, 0).effective;

    sourceClock.value = 0.0f;
    pushAndProcess(core, sourceClock);
    const float withMin = core.effectiveRow(0, 0).effective;

    if (nearlyEqual(withMax, withMin))
    {
        std::printf("FAIL: VCO 1 EF lane did not change effective value\n");
        return false;
    }
    return true;
}

bool test_external_midi_mod_slot_still_readable()
{
    froggers_v2::FroggersV2ControlCore core;
    core.setExternalMidiMod(1, 0.25f);

    if (!nearlyEqual(core.externalMidiMod(1), 0.25f))
    {
        std::printf("FAIL: external MIDI mod B slot value mismatch\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    if (!test_manifest_lane_assignment_persists())
    {
        return 1;
    }
    if (!test_manifest_lane_affects_effective_value())
    {
        return 1;
    }
    if (!test_external_midi_mod_slot_still_readable())
    {
        return 1;
    }

    std::printf("PASS: ExternalMidiMod tests\n");
    return 0;
}
