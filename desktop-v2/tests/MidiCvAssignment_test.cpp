#include "control/MidiCvAssignmentTable.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
bool nearlyEqual(float a, float b)
{
    return std::fabs(a - b) < 1.0e-4f;
}

bool test_midi_clock_advances_external_sequencer()
{
    SequencerState sequencer;
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
    table.shiftButton.target = MidiCvBindingRole::HeldModifier;
    table.sceneButtons[1].enabled = true;
    table.sceneButtons[1].kind = MidiCvTriggerKind::Cc;
    table.sceneButtons[1].number = 20;
    table.sceneButtons[1].target = MidiCvBindingRole::SceneOrdinal1;

    bool modifierDown = false;
    uint8_t sceneOrdinal = 255;
    table.setUiShiftCallback([&](bool held) { modifierDown = held; });
    table.setUiSceneCallback([&](uint8_t ordinal) { sceneOrdinal = ordinal; });

    table.processIncomingMessage(1, 0x90, 36, 127, false);
    if (!modifierDown)
    {
        std::printf("FAIL: shift note on did not fire callback\n");
        return false;
    }

    table.processIncomingMessage(1, 0x80, 36, 0, false);
    if (modifierDown)
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
    table.sceneButtons[0].target = MidiCvBindingRole::SceneOrdinal0;

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
    table.shiftButton.target = MidiCvBindingRole::HeldModifier;

    table.setUiShiftCallback(nullptr);
    table.processIncomingMessage(1, 0xB0, 64, 127, false);

    bool modifierDown = false;
    table.setUiShiftCallback([&](bool held) { modifierDown = held; });
    table.drainPendingUiActions();
    if (!modifierDown)
    {
        std::printf("FAIL: drain did not replay shift callback\n");
        return false;
    }
    return true;
}

bool test_multi_target_fan_out_dispatches_to_all_targets()
{
    // desktop-v2-operator-truth-repair Packet 10 (10.1 / archived 6.4): a
    // single physical input event bound to more than one target must fan out
    // to every mapped target through the controller model, not just the
    // first match, and buildTargetMappingRows() must report the resulting
    // fan-out count for every row that shares the event.
    MidiCvAssignmentTable table;
    table.sceneButtons[0].enabled = true;
    table.sceneButtons[0].kind = MidiCvTriggerKind::Cc;
    table.sceneButtons[0].channel = 0;
    table.sceneButtons[0].number = 50;
    table.sceneButtons[0].target = MidiCvBindingRole::SceneOrdinal0;

    table.sceneButtons[1].enabled = true;
    table.sceneButtons[1].kind = MidiCvTriggerKind::Cc;
    table.sceneButtons[1].channel = 0;
    table.sceneButtons[1].number = 50;
    table.sceneButtons[1].target = MidiCvBindingRole::SceneOrdinal1;

    std::vector<uint8_t> firedOrdinals;
    table.setUiSceneCallback([&](uint8_t ordinal) { firedOrdinals.push_back(ordinal); });

    table.processIncomingMessage(1, 0xB0, 50, 127, false);

    if (firedOrdinals.size() != 2)
    {
        std::printf("FAIL: fan-out did not dispatch to both mapped targets (got %zu)\n",
                    firedOrdinals.size());
        return false;
    }
    if (firedOrdinals[0] != 0 || firedOrdinals[1] != 1)
    {
        std::printf("FAIL: fan-out dispatched to unexpected ordinals\n");
        return false;
    }

    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    const std::vector<ControllerTargetMappingRow> rows = table.buildTargetMappingRows();
    size_t matchedFanOutRows = 0;
    for (const ControllerTargetMappingRow& row : rows)
    {
        if (row.targetId == nullptr)
        {
            continue;
        }
        if (std::strcmp(row.targetId, ids.scene1) == 0 || std::strcmp(row.targetId, ids.scene2) == 0)
        {
            if (row.fanOutCount != 2)
            {
                std::printf("FAIL: fan-out row count mismatch for %s (got %zu)\n", row.targetId,
                            row.fanOutCount);
                return false;
            }
            ++matchedFanOutRows;
        }
    }
    if (matchedFanOutRows != 2)
    {
        std::printf("FAIL: expected both scene rows to report as fan-out\n");
        return false;
    }
    return true;
}

bool test_mapping_identity_persists_via_stable_target_ids()
{
    // desktop-v2-operator-truth-repair Packet 10 (10.3 / archived 6.5):
    // controller mappings must resolve through stable manifest target IDs,
    // not through any transient/display state, and persistence state
    // tracking must reflect dirty/saved transitions independent of that
    // identity.
    MidiCvAssignmentTable table;
    table.externalModA.enabled = true;
    table.externalModA.channel = 2;
    table.externalModA.cc = 10;

    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    const char* expectedId = froggers_v2::manifest::kControllerTargetDeclarations[2].stableId;
    if (std::strcmp(ids.externalModA, expectedId) != 0)
    {
        std::printf("FAIL: external mod A target id is not the manifest stable id\n");
        return false;
    }

    if (table.mappingPersistenceState() != ControllerPersistenceState::Saved)
    {
        std::printf("FAIL: fresh table should start in Saved persistence state\n");
        return false;
    }

    table.markMappingsDirty();
    if (table.mappingPersistenceState() != ControllerPersistenceState::Dirty)
    {
        std::printf("FAIL: markMappingsDirty did not set Dirty state\n");
        return false;
    }

    table.markMappingsSaved("Saved to disk");
    if (table.mappingPersistenceState() != ControllerPersistenceState::Saved
        || table.mappingPersistenceMessage() != "Saved to disk")
    {
        std::printf("FAIL: markMappingsSaved did not restore Saved state/message\n");
        return false;
    }

    // The mapping's identity (target id) must be unchanged and independent
    // of the dirty/saved churn above -- persistence survives through the
    // stable id, not through any mutable label/UI state.
    const std::vector<ControllerTargetMappingRow> firstRows = table.buildTargetMappingRows();
    const std::vector<ControllerTargetMappingRow> secondRows = table.buildTargetMappingRows();
    bool found = false;
    for (size_t i = 0; i < firstRows.size(); ++i)
    {
        if (firstRows[i].targetId == nullptr || secondRows[i].targetId == nullptr)
        {
            continue;
        }
        if (std::strcmp(firstRows[i].targetId, ids.externalModA) == 0)
        {
            found = true;
            if (std::strcmp(secondRows[i].targetId, ids.externalModA) != 0)
            {
                std::printf("FAIL: target id was not stable across repeated projections\n");
                return false;
            }
            if (firstRows[i].event.channel != 2 || firstRows[i].event.number != 10)
            {
                std::printf("FAIL: mapped event data did not persist on the stable id row\n");
                return false;
            }
        }
    }
    if (!found)
    {
        std::printf("FAIL: external mod A row not found by stable target id\n");
        return false;
    }
    return true;
}

bool test_controller_target_ids_match_manifest()
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    const char* actual[] = {ids.pitch,
                            ids.gate,
                            ids.externalModA,
                            ids.externalModB,
                            ids.shiftButton,
                            ids.scene1,
                            ids.scene2,
                            ids.scene3,
                            ids.qwertyVirtual,
                            ids.externalClock};
    static_assert(std::size(actual) == froggers_v2::manifest::kControllerTargetDeclarations.size());
    for (size_t i = 0; i < froggers_v2::manifest::kControllerTargetDeclarations.size(); ++i)
    {
        const char* expected = froggers_v2::manifest::kControllerTargetDeclarations[i].stableId;
        if (std::strcmp(actual[i], expected) != 0)
        {
            std::printf("FAIL: controller target ID %zu mismatch\n", i);
            return false;
        }
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
    if (!test_controller_target_ids_match_manifest())
    {
        return 1;
    }
    if (!test_multi_target_fan_out_dispatches_to_all_targets())
    {
        return 1;
    }
    if (!test_mapping_identity_persists_via_stable_target_ids())
    {
        return 1;
    }

    std::printf("PASS: MidiCvAssignment tests\n");
    return 0;
}
