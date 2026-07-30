## MODIFIED Requirements

**Audit 2026-06-30:** Device dropdown and `pushExternalMidiMods` ingest CC levels (`MainComponent.cpp` L137–157, `setExternalMidiMod` in control core). Broken: `sourceValue` reads `m_sourceValues` only; `kNumModSources=8` has no MIDI CC A/B slots; `ModSourceCell` menu lists scope taps only.

### Requirement: v2-single-midi-cv-input

Desktop v2 SHALL expose one primary MIDI input device selector for user-assignable CV routing (pitch, gate, and mod CV targets).

The settings UI SHALL document the two-step operator model: (1) select input device, (2) configure CV assignments.

MIDI CC A and MIDI CC B assignments SHALL be reachable as named mod sources in the module mod assignment UI, not only as silent backend state.

#### Scenario: CC to mod source assignment

- **WHEN** the user maps MIDI CC 74 on channel 1 to MIDI CC A and assigns **MIDI CC A** as a mod source on a knob row
- **THEN** that CC level modulates the row through the control core at control rate
