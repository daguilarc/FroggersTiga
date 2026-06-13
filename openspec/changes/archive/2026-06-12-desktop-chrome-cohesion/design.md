## Context

**Ad hoc layout decisions (current code):**

| Region | Layout rule | Effect |
|--------|-------------|--------|
| Window | `setSize(1680,720)`; no `setResizable` | Wide default; macOS may block shrink |
| Header | 100px single block; cluster `removeFromRight(108)` full height | **68px dead band** under transport row |
| Mod rack | `boxW = max(120, width/4)`; `kBoxGap=18` | Scopes stretch to ~400px at 1680 |
| Global strip | `font.getStringWidth + 4`, left-aligned | Labels clip; strip underused |
| Record cluster | `rowH = max(18, h/4)` × 4 `removeFromTop` | **OGG gets remainder only** (14px vs 18px) |
| Marbles scope | StepHold trace; idle = 0.5 midline | Held CV invisible; stop erases level |
| External box | `InputEnvelopeIndicator` | Level bar; empty unless Play+External |

```
CURRENT (broken cohesion):

  ┌── header 100px ──────────────────────────────────────────────┐
  │ [Play][Stop][Ext][In] ... [MIDI][Audio]  │ [● RECORD      ] │
  │ ~~~~~~~~ dead ~68px ~~~~~~~~              │ [☐ WAV         ] │
  │                                          │ [☐ MP3         ] │
  │                                          │ [☐ FLAC        ] │
  │                                          │ [☐ ogg←tiny    ] │
  ├──────────────────────────────────────────┴──────────────────┤
  │ [MIDI~~~~stretch~~~~][VCO~~~~][Marb1~~~~][Marb2~~~~]        │
  ├──────────────────────────────────────────────────────────────┤
  │ [Rand All?][Rand Mods?][Rand waves][Marbl…]                  │
  └──────────────────────────────────────────────────────────────┘
```

## Goals / Non-Goals

**Goals:**

- One `DesktopChromeLayout.hpp` drives header rows, mod rack, strip, record cluster.
- Window resizable 1024–8192 wide; default 1440×720.
- Two-row header per `desktop-audio-export` design §6 v2 — no dead band.
- Strip labels **Rand All**, **Rand Mods**, **Rand waves**, **Marbles**; mod scopes ~96px wide centered.
- Marbles held CV visible as level fill + horizontal trace.
- All four format toggles equal height, uniform font, checkbox tick chrome.

**Non-Goals:**

- Web UI layout.
- Removing External checkbox (`desktop-host-corrections` deferred).
- Audio waveform scope for external input (level meter only).
- Re-layout six panel columns (still `width/6`).

## Decisions

### 1. Default window 1440×720

Supersedes `desktop-compact-layout` 1680. Rationale: scopes and chrome do not need 1680; panels absorb extra width at ~237px each. `desktop-compact-layout` rejected 1440 for Audio wave rows — **verification task 7.7** re-checks Audio panel at 1440 before marking complete. User can resize to 1680+.

### 2. `MainWindow` resize

```cpp
setResizable(true, true);
setResizeLimits(1024, 600, 8192, 4320);
centreWithSize(1440, 720);
```

### 3. Two-row header (`MainComponent::resized`)

**Hit-test correction (`desktop-header-hit-test`):** `RecordExportCluster` bounds SHALL be `recordGlobal.getUnion(formatGlobal)`, not the full header rectangle. Cluster shell uses `setInterceptsMouseClicks(false, true)`. Transport controls are `toFront` below `PatchCableOverlay`.

```text
kTransportRowH = 32
kModRackRowH   = 72
kHeaderHeight  = kTransportRowH + kModRackRowH   // 104

row 1: Play | Stop | External | In env | … | MIDI | Audio | ● RECORD
row 2: mod rack (left, remaining width)          | ☐ WAV
                                                 | ☐ MP3
                                                 | ☐ FLAC
                                                 | ☐ OGG
```

- **RECORD** in row 1 immediately right of Audio.
- Format toggles in row 2 right column (`kRecordClusterW = 120`), beside mod rack.
- Mod rack row height 72px (unchanged visual target).

### 4. Shared constants (`DesktopChromeLayout.hpp`)

```text
kModBoxWidth      = 96
kModBoxGap        = 16    // satisfies desktop-sim-ux-polish ≥16px
kModBoxMinWidth   = 80
kRecordClusterW   = 120
kFormatRowH       = 20
kTransportRowH    = 32
kModRackRowH      = 72
```

Record cluster format area height = `4 * kFormatRowH` with remainder distribution if fractional.

### 5. Mod rack — capped width, centered

```text
rackW = 4 * kModBoxWidth + 3 * kModBoxGap   // 448
center rackW in ModRackPanel bounds
each box = kModBoxWidth (shrink to kModBoxMinWidth only if window < rackW + margins)
```

Extra window width goes to six panel columns, not scopes.

### 6. Global strip — `getBestWidthForHeight`, centered

```cpp
const int w = btn.getBestWidthForHeight(area.getHeight());
```

Labels (title case on first two):

| Control | Label |
|---------|-------|
| Randomize all | **Rand All** |
| Randomize mod all | **Rand Mods** |
| Randomize VCO waveform | **Rand waves** |
| Marbles | **Marbles** |

Measure total button width + gaps; center the group in strip bounds. Never use raw `getStringWidth + 4`.

### 7. OGG row height + checkbox chrome

**Root cause:** integer division assigns remainder to last row only.

**Fix:** distribute `height % 4` across first rows, or use fixed `kFormatRowH = 20`.

**§5.3:** `ToggleButton` tick-box style (not radio-pill appearance); uniform 11pt bold font on all four labels.

### 8. Marbles scope visibility

`CvScopeDisplay` for `StepHold`:

- Cache `m_lastLevel` on every `pushSample`.
- Paint **level fill** (alpha ~0.25) from bottom to `m_lastLevel * height` before trace.
- On step (`rangeMin != rangeMax`): push 4 samples at old level + 4 at new level per UI tick.
- `paintIdle`: draw at `m_lastLevel` dimmed when known; else grid only.

`ModModuleBox::refresh`: on stop, `setIdle(true)` but scope retains `m_lastLevel` — not forced 0.5.

### 9. Input envelope indicator

Tooltip: `"Input level (Play + External on)"`.

### 10. Absorb `desktop-audio-export` §5

| Task | Delivered here |
|------|----------------|
| 5.1 Two-row header reflow | Decision §3 |
| 5.2 Cluster width 120 + row fit | Decisions §4, §7 |
| 5.3 Checkbox tick chrome | Decision §7 |

Mark 5.1–5.3 complete in `desktop-audio-export/tasks.md` on apply.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 1440 tight on Audio wave rows | Task 7.7 verification; user can resize |
| Centered mod rack wastes side space | Intentional — scopes stay readable |
| Level fill clutters Marbles trace | Low alpha; trace on top |
| Strip label rename vs Quick Dict | Update `QUICK_DICT.md` transport lines to **Rand All** / **Rand Mods** |

## Migration Plan

1. Add `DesktopChromeLayout.hpp`; MainWindow resize + default 1440.
2. Reflow `MainComponent::resized` two-row header.
3. ModRackPanel + GlobalStrip + RecordExportCluster use shared constants.
4. CvScopeDisplay level fill + idle level; GlobalStrip label text.
5. Visual verify + footnote `desktop-compact-layout` design §1.

## Open Questions

- None blocking.
