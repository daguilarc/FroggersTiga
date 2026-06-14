## Context

```
Hardware MIDI In
       │
       ▼
  PushMidiCc (filter per pair)
       │
       ▼
  CvMidiBridge latches
  ┌────────────────┬────────────────┐
  │ CC1 ch+cc      │ CC2 ch+cc      │
  │ → m_inCcLevel1 │ → m_inCcLevel2 │
  └───────┬────────┴────────┬───────┘
          ▼                 ▼
      mods[0]            mods[1]
          │                 │
          ▼                 ▼
   [MIDI CC 1]        [MIDI CC 2]       ← desktop mod rack columns ("OUT")
     scope + jack       scope + jack

   Web mod bay (same mods[0/1], scopes only — no patch cables):
   [MIDI CC 1 scope] [MIDI CC 2 scope] [VCO Env] [Random1] [Random2]

Hardware MIDI Out (unchanged, separate):
  VCO envelope → physical MIDI port (dialog section "MIDI Out")
```

The mod rack MIDI header in the UI is the **mod CV output**, not a MIDI hardware port.

## Goals / Non-Goals

**Goals:**

- Two inbound `(channel, CC)` pairs labeled **MIDI CC 1** and **MIDI CC 2**.
- Each matching CC message sets latched CV 0–1 on `mods[0]` or `mods[1]`.
- Mod rack row: rename **MIDI** → **MIDI CC 1** (left); add **MIDI CC 2** column immediately to its right; same box size/gap alignment as VCO Envelope / Random columns.
- MIDI Settings in row: `CC1: Ch|CC` | `CC2: Ch|CC` under device selector.
- Remove notes/QWERTY; fix CC slider width.
- Web: External MIDI permission gate; mod bay + dropdown parity for `mods[0]`/`mods[1]`.

**Non-Goals:**

- Dual hardware MIDI Out streams (envelope export stays one channel+CC).
- Note input, pitch bend, MPE.
- Full MIDI Settings dialog on web (use defaults ch1/CC1 and ch1/CC2; desktop has full config).

## Decisions

### D1 — Dual input state in `CvMidiBridge` (shared desktop + wasm)

Replace single `m_inChannel` / `m_inCc` with:

| Field | Default (UI 1-based ch) | Drives |
|-------|-------------------------|--------|
| `m_inChannel1`, `m_inCc1` | ch 1, CC 1 | `mods[0]` |
| `m_inChannel2`, `m_inCc2` | ch 1, CC 2 | `mods[1]` |
| `m_inCcLevel1`, `m_inCcLevel2` | 0 | latched CV |

Delete all note-queue / QWERTY / held-note code.

### D2 — `PushMidiCc` routes to correct latch

On enqueue, O(1) match:

- `(channel, cc) == (m_inChannel1, m_inCc1)` → queue event tagged pair 1
- `(channel, cc) == (m_inChannel2, m_inCc2)` → queue event tagged pair 2
- else discard

**Alternative rejected:** Single queue filtered twice at drain—works but wastes slots on unrelated CC traffic.

**Choice:** One CC queue with pair id on event, or two small queues. Prefer **one queue + pair byte** to reuse existing ring buffer (OMNI: one structure).

### D3 — `drainMidiIn`

After draining queue, set:

```cpp
mods[0] = m_inCcLevel1;
mods[1] = m_inCcLevel2;
```

(Only when `modCount > 1`; guard index bounds.)

**Choice:** Add `CvMidiBridge m_midiBridge` to **`PagedHostIO`**; call `drainMidiIn` from `tickControls()` (same as `DesktopHostIO`). Single bridge implementation for desktop and wasm (OMNI data-flow rule).

### D3b — Labels and assignable mod indices

- `ParamDisplayNames::forModSource(0)` → `"MIDI CC 1"`; `(1)` → `"MIDI CC 2"`.
- `SimModSource` / `IsSimAssignableModIndex`: add mod index **1** alongside 0, 4, 5, 6.

### D4 — Mod rack row layout (five columns, desktop)

**Choice:** Two separate `ModModuleBox` columns at the **left** of the mod rack row—same width, height, and gap as existing boxes:

```
┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
│ MIDI CC 1│ │ MIDI CC 2│ │VCO Envel.│ │ Random 1 │ │ Random 2 │
│  scope   │ │  scope   │ │  scope   │ │   LED    │ │   LED    │
│   (○)    │ │   (○)    │ │   (○)    │ │   (○)    │ │   (○)    │
└──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
  mods[0]      mods[1]       mods[4]      mods[5]      mods[6]
```

- Rename `m_midi` label `"MIDI"` → `"MIDI CC 1"`, `modIndex` 0.
- Add `m_midiCc2` box `"MIDI CC 2"`, `modIndex` 1, immediately to the right of CC 1.
- Update `HostPanelLayout::kModRackGroupWidth` to **5** boxes: `5 * kModBoxWidth + 4 * kModBoxGap`.
- `ModRackPanel::resized()`: single loop over five boxes (OMNI repetition—replace hardcoded `4` and box array literals in three places with one shared box list).

**Why:** Neat row alignment; each input gets its own labeled column matching VCO Envelope / Random layout.

### D5 — MIDI Settings layout

```
MIDI In
  Device [……………………………]
  MIDI CC 1:  Ch [1–16]  CC [0–127]   |   MIDI CC 2:  Ch [1–16]  CC [0–127]

MIDI Out (hardware — unchanged)
  Device … envelope export …
```

Dialog width **520 px**. CC slider init loop: `{&m_inCc1, &m_inCc2}` (+ hardware out CC if kept).

### D6 — Hardware MIDI Out

Leave `tickMidiOut` envelope → single `m_outChannel` / `m_outCc` unchanged. Not part of mod rack dual-input scope.

### D7 — No layout helper

Trigger count 1 → inline `resized()` with constants.

### D8 — Web External MIDI (permission-gated)

**Choice:** Add `#external-midi-btn` **directly under** `#external-btn` in `controls-top` (stacked column or wrapped block). Default **External MIDI: Off**. On enable:

1. Require secure context (HTTPS) like External Audio.
2. Call `navigator.requestMIDIAccess({ sysex: false })` — browser shows MIDI permission prompt.
3. On success: attach `onmidimessage` to all inputs; on CC match push to worklet.
4. On disable: remove listeners; stop updating `mods[0/1]` from Web MIDI (latched values hold until next CC).

Mirror External Audio error/status copy pattern (`NotAllowedError`, insecure context).

**Alternative rejected:** Auto-open Web MIDI on Play — violates explicit permission ask.

### D9 — Web mod bay + modulation dropdown

**Choice:**

- Single ordered list `kScopeModIndices = {0, 1, 4, 5, 6}` drives mod bay construction, scope reads, and `modSourceNames` payload (OMNI: one structure, no duplicate TS/C++ drift beyond the paired constants).
- Prepend scopes for mod **0** and **1** (labels from `froggers_mod_source_name`).
- Build mod-source `<select>` options at worklet `ready` from wasm assignable indices + `froggers_mod_source_name` — **no hardcoded option HTML** (extends `host-label-single-authority` to dropdowns).
- Replace `modSelectIndex()` switch with `selectedIndex = options.findIndex(v => Number(v) === modSource)`.

**Why:** Same deal as other modulation—pick source in dropdown, knob controls depth, scope shows CV. Hardcoded `<option value="4">VCO Envelope</option>` was an OMNI violation; do not extend it for CC 1/2.

### D10 — Wasm MIDI push export

**Choice:** Add to `wasm/bindings.cpp`:

```cpp
froggers_push_midi_cc(host, channel, cc, value)
  → host->io.m_midiBridge.PushMidiCc(...)
```

Worklet receives `{ type: "midiCc", channel, cc, value }` from main thread; CC config defaults live in bridge (ch1/CC1, ch1/CC2). Optional later: `froggers_set_midi_cc_config` if web needs UI—out of scope now.

### D11 — Move bridge from DesktopHostIO to PagedHostIO

**Choice:** `CvMidiBridge m_midiBridge` lives on **`PagedHostIO`** (base class). `DesktopHostIO` inherits it; remove duplicate member. Wasm and desktop share one drain path in `PagedHostIO::tickControls()`.

**Why:** Wasm uses `PagedHostIO` directly — bridge on `DesktopHostIO` only blocked web MIDI (OMNI data-flow audit finding).

### D12 — CI label gate extends to mods 0 and 1

**Choice:** Extend `sim/check_mod_source_labels.sh` to parse `forModSource(0)` and `(1)`; reject hardcoded `<option value="N">Label</option>` patterns in `main.ts`.

## Risks / Trade-offs

- **[Risk] `mods[1]` previously unused on mod rack** → Mitigation: second jack + `applyCvPresence` already tracks index 1.
- **[Risk] Five columns wider than header** → Mitigation: update `kModRackGroupWidth`; shrink path uses `(width - 4 * gap) / 5` when narrow.
- **[Risk] Web MIDI unsupported browser** → Mitigation: disable button + status message when `navigator.requestMIDIAccess` missing.
- **[Risk] Mod bay crowded with five scopes** → Mitigation: CSS flex wrap or horizontal scroll consistent with existing mod-bay styling.

## Migration Plan

1. Shared `CvMidiBridge` on `PagedHostIO` + labels + `SimModSource`.
2. Desktop: settings, mod rack, remove QWERTY.
3. Wasm bindings + scope indices.
4. Web: External MIDI button, Web MIDI handler, mod bay + dropdown.
5. Docs + QA (desktop patch cables + web dropdown modulation).

## Open Questions

(none)
