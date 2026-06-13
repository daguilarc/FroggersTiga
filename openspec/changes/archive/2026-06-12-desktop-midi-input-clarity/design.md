## Context

**Two separate flows (must not be conflated)**

```text
┌─ MIDI IN (settings) ─────────────────────────────────────────┐
│  Computer keyboard  OR  hardware juce::MidiInput             │
│       │                                                      │
│       ▼  Note On/Off (+ optional CC for knobs)               │
│  CvMidiBridge → m_mods[0]                                    │
│       │                                                      │
│       ▼                                                      │
│  Mod rack "MIDI" jack ──patch──▶ knob CV inputs              │
└──────────────────────────────────────────────────────────────┘

┌─ MIDI OUT (VCO Env) (settings) ──────────────────────────────┐
│  VCO Envelope level only                                     │
│       │                                                      │
│       ▼  tickMidiOut (after audio block)                     │
│  m_midiOut → physical MIDI port (if device open)             │
└──────────────────────────────────────────────────────────────┘
```

**Why it is wrong today**

| Expected | Actual |
|----------|--------|
| QWERTY notes → MIDI mod jack | `feedComputerKeyboardMod(127)` — one fake CC, not note semantics |
| Hardware notes → MIDI mod jack | `handleIncomingMidiMessage` ignores non-CC |
| MIDI Out = VCO Env to hardware | Label says “MIDI out device”; behavior is envelope-only but UI does not say so; keyboard never belonged here |
| Out only if device plugged in | `openDefaultMidi()` opens first out device at launch |

**Root cause:** `desktop-qwerty-midi-in` deliberately mapped notes → single in-CC (`PushMidiCc`) because `CvMidiBridge` was CC-only. That matched engine limits, not the user-facing MIDI Settings model.

## Goals / Non-Goals

**Goals:**

- MIDI In drives the **MIDI** mod module via note velocity (QWERTY + hardware notes).
- MIDI Settings labels match behavior: **MIDI In** / **MIDI Out (VCO Env)**.
- Physical envelope out sends only when `m_midiOut` is open.
- Piano legend, refresh, hardware open-failure status.

**Non-Goals:**

- QWERTY or mod rack → physical MIDI note output (that is not this app’s MIDI In path).
- Polyphonic per-cable mod (still one `m_mods[0]` level).
- Web / VCV / firmware changes.

## Decisions

### 1. Note → mod value in `CvMidiBridge` (single drain site)

Track held notes on configured **in channel**:

```text
PushMidiNote(channel, note, velocity, isNoteOn)
  → update held-note set for matching channel
  → m_modLevel = max(velocity of held notes) / 127, or 0 if none

drainMidiIn:
  → mods[0] = m_modLevel (after CC events on configured in CC, if any)
```

CC path unchanged for knob controllers on `m_inChannel` + `m_inCc`. Note and CC both write `mods[0]` in one drain; last matching event in queue wins for CC; note level is stateful across blocks.

**OMNI:** one bridge, one drain, accumulate note state then apply once per `drainMidiIn`.

### 2. QWERTY path

`MainComponent` on note on/off (via `MidiKeyboardState` listener):

```text
m_audio.feedMidiInNote(channel, note, velocity, isNoteOn)
  → CvMidiBridge::PushMidiNote
```

Remove `syncQwertyModFromKeyboardState` → `feedComputerKeyboardMod` CC collapse. Iterate `kQwertyPianoKeys` only if needed for `keyStateChanged` sync.

Guard: `isComputerKeyboardMidiEnabled()`, focus/modal guards unchanged.

### 3. Hardware MIDI In

`handleIncomingMidiMessage`:

- `isNoteOn` / `isNoteOff` → `PushMidiNote` on message channel
- `isController` → existing `PushMidiCc`

Selecting hardware device disables QWERTY capture (unchanged). Selecting **Computer keyboard** closes hardware in.

### 4. MIDI Settings page copy and layout

```text
MIDI In
  Device:     [ Computer keyboard ▼ ]
  Channel:    [ 1 ]
  In CC:      [ 1 ]   (mod from CC controllers; notes use velocity)

  Legend: QWERTY piano A…P → MIDI mod jack (patch cables)

MIDI Out (VCO Env)
  Device:     [ none / interface ▼ ]
  Channel:    [ 1 ]   CC: [ 74 ]
  Help: VCO Envelope → physical MIDI port when device selected
```

Rename `m_outLabel` to **MIDI Out (VCO Env)**. Rename `m_inCcLabel` tooltip to clarify CC is for controllers; notes use piano/keyboard velocity.

### 5. Physical out only when device open

- Do not auto-open first MIDI out at launch unless a device exists and user has not chosen none.
- `tickMidiOut` / lambda: send only if `m_midiOut != nullptr`.
- Optional: **None** entry in out device list.

### 6. Refresh + status (unchanged intent)

- **Refresh devices** for mid-dialog hot-plug.
- `isHardwareMidiInputOpenFailed()` for mod-controller status.

## Risks / Trade-offs

- **[Risk] One mod level for chords** → max velocity among held notes; documented in legend.
- **[Risk] Note + CC fight for mods[0]** → acceptable; CC for knobs, notes for keyboards.

## Migration Plan

1. Extend `CvMidiBridge` with note state + `PushMidiNote`.
2. Rewire QWERTY and hardware in to notes path; remove CC collapse from keyboard.
3. MIDI Settings labels/legend; optional out-device none; gate `tickMidiOut`.
4. Manual test: QWERTY → MIDI scope moves when patched; envelope → MIDI monitor only on VCO Env port.

## Open Questions

- None blocking.
