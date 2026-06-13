## Context

**Current path** (`desktop-midi-input-clarity`, implemented):

```text
UI thread: PushMidiNote → mutates m_heldVelocity[], m_modLevelFromNotes
Audio thread: drainMidiIn → mods[0] = m_modLevelFromNotes (max vel / 127)
QWERTY: noteOn(..., 1.0f) → vel 127 always → mods[0] ∈ {0, 1}
```

Pitch (MIDI 60–75 for `kQwertyPianoKeys`) is stored per slot but never exported. Cross-thread held-state mutation is a data race. The CC path (`m_pendingIn` vector) has the same unsynchronized push/clear pattern.

**Reference pattern:** `DesktopHostIO` `HostMutation` — atomic SPSC ring buffer; UI enqueues, audio drains at start of `tickControls()`.

**QWERTY map:** A=60 … P=75 (16 keys). Pitch step spans 1/16 … 1.0 so the lowest key is not confused with silence.

## Goals / Non-Goals

**Goals:**

- Every QWERTY piano key including **A** produces a distinct non-zero `m_mods[0]` while held (visible on scope and patched knobs).
- Hardware MIDI notes use the same formula with real velocity.
- All held-note state updates happen only in `drainMidiIn` on the audio thread.
- Note enqueue uses the same thread-safe SPSC pattern as `HostMutation`.
- CC on configured in channel + in CC still overwrites `mods[0]` after note level (unchanged precedence).

**Non-Goals:**

- Sending note messages to physical MIDI Out.
- Separate gate + pitch jacks on the sim mod rack (still one MIDI mod lane).
- Web MIDI.
- Microtonal or extended keyboard range beyond current QWERTY map.
- Black-key velocity tiers (deferred; pitch steps suffice for v1).

## Decisions

### 1. Mod formula: highest-note pitch step × max velocity

```text
highestNote = max note index i where m_heldVelocity[i] > 0
pitchStep   = clamp((highestNote - 60 + 1) / 16, 0, 1)   // 0 if no notes
velNorm     = max(m_heldVelocity) / 127                    // 0 if no notes
mods[0]     = pitchStep * velNorm
```

**Examples (QWERTY, vel 127):**

| Key | Note | pitchStep | mods[0] |
|-----|------|-----------|---------|
| A   | 60   | 1/16      | 0.0625  |
| W   | 61   | 2/16      | 0.125   |
| P   | 75   | 16/16     | 1.0     |
| (none) | — | 0      | 0       |

**Rationale:** `pitchStep = (note − 60) / 15` made **A** output 0 — indistinguishable from no keys and invisible to `applyCvPresence` (threshold 0.02). The `+1 / 16` offset gives 16 distinct held-key levels with silence still at 0. Hardware keyboards retain velocity expression; notes outside 60–75 clamp into the same range.

**Alternatives rejected:**

- Max velocity only (current) — binary on QWERTY.
- `(note − 60) / 15` — **A** dead at 0.
- Last-note-wins pitch — harder to specify for chord releases; highest note is predictable for mono patches.

### 2. Note event queue (atomic SPSC ring, mirror `HostMutation`)

```text
struct MidiNoteEvent { uint8_t channel, note, velocity; bool isNoteOn; }

constexpr int kMidiNoteQueueSize = 64;
std::array<MidiNoteEvent, kMidiNoteQueueSize> m_noteQueue{};
std::atomic<int> m_noteWrite{0};
std::atomic<int> m_noteRead{0};

PushMidiNote(...)  → enqueue with memory_order_release (drop oldest on full)
drainMidiIn(...):
  while read < write: apply matching-channel events to m_heldVelocity[note]
  recomputeModLevelFromHeldNotes()  // single pass: highest note + max vel → pitchStep × velNorm
  mods[0] = m_modLevelFromNotes
  apply CC events from m_pendingIn (existing)
```

**OMNI:** enqueue on push; accumulate held state in drain loop; recompute once; apply `mods[0]` once; CC overwrites after.

Cap at 64 events; on overflow drop oldest with `jassert` in debug builds. Do not use an unsynchronized `std::vector` for the note path.

### 3. Single-pass recompute

`recomputeModLevelFromHeldNotes()` scans `m_heldVelocity[0..127]` once: track `highestNote` (max index with vel > 0) and `maxVel` in the same loop, then compute `pitchStep × velNorm`. No second pass over the array.

### 4. QWERTY velocity unchanged (v1)

Keep `noteOn(1, note, 1.0f)`. Pitch differentiation comes from decision 1.

### 5. Scope and `applyCvPresence`

No separate scope tap. `GetCvOut(0)` after drain already reflects pitch steps. Lowest key **A** yields `mods[0] ≈ 0.0625`, above the `applyCvPresence` absolute threshold (0.02).

### 6. Supersede `desktop-midi-input-clarity` velocity-only spec

Archive note: pitch semantics live in this change. On apply, update `desktop-midi-input-clarity` task 2.3 manual checklist to verify **different keys → different scope level**, including **A** alone.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Chord pitch = highest note surprises users | Document in MIDI Settings legend + Quick Dict |
| CC stomps note level every block | Unchanged; knob CC controllers still win when sent |
| Queue overflow on fast glissando | 64-event SPSC cap; drop oldest; typical QWERTY stays well under |
| mods[0] never updates when Play stopped | Unchanged; scope idle without audio — document in tooltip |
| Hardware notes outside 60–75 clamp to endpoints | Document in Quick Dict; full keyboard still modulates via clamp |

## Migration Plan

1. Refactor `CvMidiBridge` atomic note queue + pitch formula + single-pass recompute.
2. Verify `feedMidiInNote` and `handleIncomingMidiMessage` only enqueue.
3. Manual: Play + patch MIDI → filter cutoff; **A**, **W**, and **P** move knob to three distinct values.
4. Update Quick Dict + MIDI legend + In CC tooltip.
5. Close `desktop-midi-input-clarity` manual task 2.3 under new semantics.

## Open Questions

- None blocking. Last-note-wins pitch deferred unless user requests after hearing highest-note behavior.
