#include "control/MidiCvAssignmentTable.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool nearlyEqual(float a, float b)
{
    return std::fabs(a - b) < 1.0e-4f;
}

bool test_midi_clock_advances_external_sequencer()
{
    SequencerState sequencer;
    sequencer.setPatternLength(4);
    sequencer.m_playing = true;
    sequencer.m_externalClock = true;

    bool hookFired = false;
    MidiCvAssignmentTable table;
    table.setSequencerAdvanceHook([&hookFired]() { hookFired = true; });

    table.onMidiClockTick(sequencer);
    if (sequencer.m_playhead != 1)
    {
        std::printf("FAIL: external clock did not advance playhead\n");
        return false;
    }
    if (!hookFired)
    {
        std::printf("FAIL: sequencer advance hook did not fire\n");
        return false;
    }

    sequencer.m_playing = false;
    hookFired = false;
    table.onMidiClockTick(sequencer);
    if (hookFired)
    {
        std::printf("FAIL: hook fired while transport stopped\n");
        return false;
    }
    return true;
}

bool test_midi_clock_ignored_without_external_mode()
{
    SequencerState sequencer;
    sequencer.m_playing = true;
    sequencer.m_externalClock = false;

    bool hookFired = false;
    MidiCvAssignmentTable table;
    table.setSequencerAdvanceHook([&hookFired]() { hookFired = true; });
    table.onMidiClockTick(sequencer);

    if (sequencer.m_playhead != 0 || hookFired)
    {
        std::printf("FAIL: internal clock mode should ignore MIDI clock tick\n");
        return false;
    }
    return true;
}

bool test_note_on_sets_pitch_gate_and_note_off_clears()
{
    MidiCvAssignmentTable table;
    table.pitchEnabled = true;
    table.gateEnabled = true;
    table.pitchNoteMin = 60;
    table.pitchNoteMax = 72;

    float pitchValue = -1.0f;
    bool gateValue = false;
    table.setHostPitchCallback([&](uint8_t, uint8_t, float value) { pitchValue = value; });
    table.setHostGateCallback([&](bool high) { gateValue = high; });

    table.processIncomingMessage(1, 0x90, 60, 127, false);
    if (!gateValue)
    {
        std::printf("FAIL: note on did not raise gate\n");
        return false;
    }
    if (pitchValue < 0.0f)
    {
        std::printf("FAIL: note on did not emit pitch\n");
        return false;
    }

    table.processIncomingMessage(1, 0x80, 60, 0, false);
    if (gateValue)
    {
        std::printf("FAIL: note off did not clear gate\n");
        return false;
    }
    if (!nearlyEqual(table.pitchValue(), 0.0f))
    {
        std::printf("FAIL: note off did not clear pitch\n");
        return false;
    }
    return true;
}

bool test_cc_routes_external_mod_slots()
{
    MidiCvAssignmentTable table;
    table.externalModA.enabled = true;
    table.externalModA.cc = 74;
    table.externalModB.enabled = true;
    table.externalModB.cc = 1;

    table.processIncomingMessage(1, 0xB0, 74, 127, false);
    table.processIncomingMessage(1, 0xB0, 1, 64, false);

    if (!nearlyEqual(table.externalModLevel(0), 1.0f))
    {
        std::printf("FAIL: external mod A level mismatch\n");
        return false;
    }
    if (!nearlyEqual(table.externalModLevel(1), 64.0f / 127.0f))
    {
        std::printf("FAIL: external mod B level mismatch\n");
        return false;
    }
    return true;
}

bool test_shift_and_scene_button_bindings()
{
    MidiCvAssignmentTable table;
    table.shiftButton.enabled = true;
    table.shiftButton.kind = MidiCvTriggerKind::Note;
    table.shiftButton.number = 36;
    table.shiftButton.target = MidiCvButtonTarget::Shift;
    table.sceneButtons[1].enabled = true;
    table.sceneButtons[1].kind = MidiCvTriggerKind::Cc;
    table.sceneButtons[1].number = 20;
    table.sceneButtons[1].target = MidiCvButtonTarget::Scene2;

    bool shiftHeld = false;
    uint8_t sceneOrdinal = 255;
    table.setUiShiftCallback([&](bool held) { shiftHeld = held; });
    table.setUiSceneCallback([&](uint8_t ordinal) { sceneOrdinal = ordinal; });

    table.processIncomingMessage(1, 0x90, 36, 127, false);
    if (!shiftHeld)
    {
        std::printf("FAIL: shift note on did not fire callback\n");
        return false;
    }

    table.processIncomingMessage(1, 0x80, 36, 0, false);
    if (shiftHeld)
    {
        std::printf("FAIL: shift note off did not clear callback\n");
        return false;
    }

    table.processIncomingMessage(1, 0xB0, 20, 127, false);
    if (sceneOrdinal != 1)
    {
        std::printf("FAIL: scene CC did not select ordinal 1\n");
        return false;
    }
    return true;
}

bool test_scene_button_respects_channel_filter()
{
    MidiCvAssignmentTable table;
    table.sceneButtons[0].enabled = true;
    table.sceneButtons[0].kind = MidiCvTriggerKind::Note;
    table.sceneButtons[0].number = 60;
    table.sceneButtons[0].channel = 3;
    table.sceneButtons[0].target = MidiCvButtonTarget::Scene1;

    uint8_t sceneOrdinal = 255;
    table.setUiSceneCallback([&](uint8_t ordinal) { sceneOrdinal = ordinal; });

    table.processIncomingMessage(1, 0x90, 60, 127, false);
    if (sceneOrdinal != 255)
    {
        std::printf("FAIL: scene note on wrong channel should be ignored\n");
        return false;
    }

    table.processIncomingMessage(3, 0x90, 60, 127, false);
    if (sceneOrdinal != 0)
    {
        std::printf("FAIL: scene note on configured channel should fire\n");
        return false;
    }
    return true;
}

bool test_external_cc_channel_any_matches_all()
{
    MidiCvAssignmentTable table;
    table.externalModA.enabled = true;
    table.externalModA.channel = 0;
    table.externalModA.cc = 74;

    table.processIncomingMessage(5, 0xB0, 74, 100, false);
    if (!nearlyEqual(table.externalModLevel(0), 100.0f / 127.0f))
    {
        std::printf("FAIL: channel Any should accept CC on any channel\n");
        return false;
    }
    return true;
}

bool test_qwerty_channel_filter()
{
    MidiCvAssignmentTable table;
    table.qwertyVirtualChannelEnabled = true;
    table.qwertyMidiChannel = 2;
    table.gateEnabled = true;

    bool gateValue = false;
    table.setHostGateCallback([&](bool high) { gateValue = high; });

    table.processIncomingMessage(1, 0x90, 60, 127, true);
    if (gateValue)
    {
        std::printf("FAIL: qwerty message on wrong channel should be ignored\n");
        return false;
    }

    table.processIncomingMessage(2, 0x90, 60, 127, true);
    if (!gateValue)
    {
        std::printf("FAIL: qwerty message on configured channel should pass\n");
        return false;
    }
    return true;
}

bool test_pending_ui_action_drain()
{
    MidiCvAssignmentTable table;
    table.shiftButton.enabled = true;
    table.shiftButton.kind = MidiCvTriggerKind::Cc;
    table.shiftButton.number = 64;
    table.shiftButton.target = MidiCvButtonTarget::Shift;

    table.setUiShiftCallback(nullptr);
    table.processIncomingMessage(1, 0xB0, 64, 127, false);

    bool shiftHeld = false;
    table.setUiShiftCallback([&](bool held) { shiftHeld = held; });
    table.drainPendingUiActions();
    if (!shiftHeld)
    {
        std::printf("FAIL: drain did not replay shift callback\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    if (!test_midi_clock_advances_external_sequencer())
    {
        return 1;
    }
    if (!test_midi_clock_ignored_without_external_mode())
    {
        return 1;
    }
    if (!test_note_on_sets_pitch_gate_and_note_off_clears())
    {
        return 1;
    }
    if (!test_cc_routes_external_mod_slots())
    {
        return 1;
    }
    if (!test_shift_and_scene_button_bindings())
    {
        return 1;
    }
    if (!test_scene_button_respects_channel_filter())
    {
        return 1;
    }
    if (!test_external_cc_channel_any_matches_all())
    {
        return 1;
    }
    if (!test_qwerty_channel_filter())
    {
        return 1;
    }
    if (!test_pending_ui_action_drain())
    {
        return 1;
    }

    std::printf("PASS: MidiCvAssignment tests\n");
    return 0;
}
