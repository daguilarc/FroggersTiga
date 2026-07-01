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

bool test_midi_cc_a_assignment_persists()
{
    froggers_v2::FroggersV2ControlCore core;

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = froggers_v2::kModSourceMidiCcA;
    pushAndProcess(core, assign);

    if (core.assignedModSource(0, 0) != froggers_v2::kModSourceMidiCcA)
    {
        std::printf("FAIL: MIDI CC A assignment did not persist\n");
        return false;
    }
    return true;
}

bool test_external_midi_mod_affects_effective_value()
{
    froggers_v2::FroggersV2ControlCore core;

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = froggers_v2::kModSourceMidiCcA;
    pushAndProcess(core, assign);

    froggers_v2::MessageIn press;
    press.type = froggers_v2::MessageIn::Type::ParamPress;
    press.page = 0;
    press.slot = 0;
    pushAndProcess(core, press);
    pushAndProcess(core, froggers_v2::MessageIn::ParamTurn(0, 0, 40.0f));

    froggers_v2::MessageIn extMod;
    extMod.type = froggers_v2::MessageIn::Type::Clock;
    extMod.index = 0;
    extMod.value = 1.0f;
    pushAndProcess(core, extMod);
    const float withMax = core.effectiveRow(0, 0).effective;

    extMod.value = 0.0f;
    pushAndProcess(core, extMod);
    const float withMin = core.effectiveRow(0, 0).effective;

    if (nearlyEqual(withMax, withMin))
    {
        std::printf("FAIL: external MIDI CC A did not change effective value\n");
        return false;
    }
    return true;
}

bool test_external_midi_mod_b_assignment()
{
    froggers_v2::FroggersV2ControlCore core;
    core.setExternalMidiMod(1, 0.25f);

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 1;
    assign.index = froggers_v2::kModSourceMidiCcB;
    pushAndProcess(core, assign);

    if (core.assignedModSource(0, 1) != froggers_v2::kModSourceMidiCcB)
    {
        std::printf("FAIL: MIDI CC B assignment did not persist\n");
        return false;
    }
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
    if (!test_midi_cc_a_assignment_persists())
    {
        return 1;
    }
    if (!test_external_midi_mod_affects_effective_value())
    {
        return 1;
    }
    if (!test_external_midi_mod_b_assignment())
    {
        return 1;
    }

    std::printf("PASS: ExternalMidiMod tests\n");
    return 0;
}
