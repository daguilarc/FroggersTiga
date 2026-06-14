## Context

```
Data flow (unchanged — bug is display-only):

  User edits In CC slider
       │
       ▼
  m_inCc.onValueChange → CvMidiBridge.m_inCc (uint8_t 0–127)
       │
       ▼
  drainCcQueue filters events where event.cc == m_inCc

  UI path today:

  MidiSettingsComponent::resized()
       │
       ▼
  m_inCc.setBounds(..., width=50)     ← entire slider + text box
  m_outCc.setBounds(..., width=50)
       │
       ▼
  JUCE Slider default: LinearHorizontal + TextBoxRight
  Value "74" or "127" → ellipsis in 50 px bounds
```

Current code in `MidiSettingsComponent.cpp`:

```244:249:desktop/Source/MidiSettingsComponent.cpp
    auto inChRow = area.removeFromTop(24);
    m_inChLabel.setBounds(inChRow.removeFromLeft(70));
    m_inChannel.setBounds(inChRow.removeFromLeft(50));
    inChRow.removeFromLeft(8);
    m_inCcLabel.setBounds(inChRow.removeFromLeft(50));
    m_inCc.setBounds(inChRow.removeFromLeft(50));
```

CC sliders have **no** `setTextBoxStyle` call (unlike rotary knobs in `SubModulePanel`, which use `NoTextBox`). Channel sliders (range 1–16) tolerate 50 px; CC sliders (range 0–127) do not.

**OMNI rule mapping:**

| Rule | Application |
|------|-------------|
| Data flow | Fix display layer only; `CvMidiBridge` untouched |
| Repetition | Replace four magic `50` literals with named constants; initialize both CC sliders in one loop |
| One-time helper | **No** extracted helper — layout stays inline; trigger count &lt; 2 |
| Efficiency | O(1) layout constants; no runtime cost |
| Verification | Manual check at CC values 10, 74, 127 after change |

## Goals / Non-Goals

**Goals:**

- In CC and Out CC text boxes show full numeric values 0–127 at default dialog size (480×420).
- Layout widths defined once as file-scope constants (channel vs CC).
- Both CC sliders share identical JUCE configuration (single init loop).

**Non-Goals:**

- Redesigning the entire MIDI Settings dialog or increasing window size.
- Web or VST MIDI settings (VST hides MIDI Settings per manual; browser has no MIDI UI).
- Changing CC semantics, defaults (`m_inCc=1`, `m_outCc=74`), or `CvMidiBridge` queue logic.

## Decisions

### D1 — Named layout constants in `MidiSettingsComponent.cpp`

**Choice:** Add anonymous-namespace constants:

- `kChannelControlWidth = 50` (fits 1–16)
- `kCcControlWidth = 80` (slider track + text box for three digits)
- `kCcTextBoxWidth = 44` (passed to `setTextBoxStyle`)
- `kRowControlHeight = 24`

**Why:** OMNI repetition rule — one authority for widths used in both input and output rows.

**Alternative rejected:** Per-slider hardcoded literals — caused the original drift (same `50` copied for channel and CC).

### D2 — Explicit `TextBoxRight` on CC sliders only

**Choice:** In constructor, loop `{&m_inCc, &m_outCc}`:

```cpp
slider->setSliderStyle(juce::Slider::LinearHorizontal);
slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
slider->setNumDecimalPlacesToDisplay(0);
```

**Why:** Reserves fixed text-box width independent of thumb drag area; eliminates ellipsis for 127.

**Alternative rejected:** `NoTextBox` + separate `Label` — duplicates value display (two widgets, sync burden).

### D3 — Widen CC bounds only in `resized()`

**Choice:** Replace CC `removeFromLeft(50)` with `removeFromLeft(kCcControlWidth)` on lines 249 and 270. Keep channel controls at `kChannelControlWidth`.

**Why:** Minimal diff; dialog width (480 − 24 margin = 456 px) still fits:

```
70 + 50 + 8 + 50 + 80 = 258 px  (input row — plenty of slack)
60 + 50 + 8 + 30 + 80 = 228 px  (output row)
```

**Alternative rejected:** Enlarge dialog to 560 px — unnecessary when slack already exists.

### D4 — No shared row-layout helper

**Choice:** Keep `resized()` inline with constants.

**One-time helper review:**

| Criterion | Met? |
|-----------|------|
| Complexity (≥3 nesting / ≥3 branches / ≥2 loops) | No |
| Domain boundary | No |
| Explicit contract | No |
| Local scope | Yes |

Trigger count: **1** → helper **disallowed** per OMNI One-Time Helper Extraction Rule.

## Risks / Trade-offs

- **[Risk] Font/laf change on future OS could re-truncate** → Mitigation: `kCcTextBoxWidth = 44` leaves margin above three-digit width; bump constant if QA fails on HiDPI.
- **[Risk] Very narrow host embedding** → Mitigation: MIDI Settings opens as standalone 480 px dialog, not embedded in narrow strip; out of scope.
- **[Trade-off] Slightly wider CC column** → Acceptable; row still fits with ~200 px remaining slack.

## Migration Plan

1. Edit `MidiSettingsComponent.cpp` (constants + constructor loop + `resized()`).
2. Rebuild desktop target.
3. Manual QA: MIDI Settings → set In CC to 10, 74, 127; repeat for Out CC; confirm no `…`.
4. No preset migration — display-only.

## Open Questions

(none — root cause verified in source)
