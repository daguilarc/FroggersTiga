## Context

```
Desktop browser                Mobile browser
─────────────────              ─────────────────────────────────
Play / External                Play / External
     │                              │
     ▼                              ▼
OS routes normally             iOS: play-and-record → earpiece (no headset)
(macOS/Windows/Linux)          Android: usually loudspeaker; headset edge cases
                               Desktop-class routing NOT broken — do not touch
```

Today `disconnectExternalStream()` stops tracks but never resets the platform audio session. Froggers uses `(max-width: 720px)` for mobile mic constraints but has no session management.

### Routing model (mobile)

```
External ON + mic active
────────────────────────

  Headphones connected          Built-in output only
  ────────────────────          ─────────────────────
  play-and-record → headset     play-and-record → earpiece (iOS) — THE BUG
  ✅ acceptable                 ❌ bad for screen-facing synth
  (communication path OK)       (user looks at screen, hears top speaker)
```

**Truth-value:** Headphones + mic → communication-style routing to the headset is **expected and fine**. Built-in speaker only + mic → routing to earpiece instead of bottom loudspeaker is **the failure mode** we document and partially mitigate.

## Goals / Non-Goals

**Goals:**

- Mobile-only Audio Session lifecycle via `navigator.audioSession` where available (primarily iOS Safari)
- `playback` assert on mobile Play when External off (sticky session after prior External)
- Mobile status hints that distinguish **built-in earpiece** vs **headset OK**
- Subtitle copy (all viewports) separating Play-for-sound from External-for-mic
- Manual mobile-browser routing guidance

**Non-Goals:**

- **Desktop browser changes** — routing works; zero session hooks on desktop
- Guaranteed bottom loudspeaker on iPhone while External + built-in output (WebKit limit)
- Disabling echo cancellation on mobile (ring-mod input quality)
- Reliable programmatic headset detection on iOS web (labels often empty)

## Decisions

### D1 — Mobile guard (not iOS-only, not desktop)

**Choice:** `isMobileWeb()` — true when `matchMedia('(max-width: 720px)').matches` **or** `/Android|iPhone|iPad|iPod/i.test(navigator.userAgent)`. All session/hint logic gated on this. Desktop wide viewports on non-mobile UA → **no-op** (early return in helper).

**Why:** Aligns with existing `mobileMic` width check in `setExternalEnabled`. User scope: all mobile devices; desktop explicitly excluded.

### D2 — One helper: `applyMobileAudioSession(mode)`

**Choice:** Module-level function; **returns immediately when `!isMobileWeb()`**.

| `mode` | When | `navigator.audioSession.type` |
|--------|------|-------------------------------|
| `'playback'` | Play running, External off | `'playback'` |
| `'reset'` | Before `getUserMedia` on External enable | `'auto'` |
| `'externalOn'` | After mic stream connected | `'play-and-record'` |
| `'externalOff'` | After tracks stopped | `'playback'` then `'auto'` |

Guard: `'audioSession' in navigator`. Swallow errors (experimental API; Android may lack it).

**Helper extraction review (OMNI):** trigger ≥2, domain boundary, complexity ≥3 branches, explicit contract, local scope — all **yes**.

### D3 — Hook points (data flow)

```
Desktop:  (no calls)

Mobile startAudio (External off):
  applyMobileAudioSession('playback')

Mobile setExternalEnabled(true):
  applyMobileAudioSession('reset')
  getUserMedia → micSource
  applyMobileAudioSession('externalOn')

Mobile disconnectExternalStream (incl. stopAudio — no duplicate mic teardown):
  applyMobileAudioSession('externalOff')
```

**OMNI audit (apply):** `stopAudio()` duplicated mic track cleanup; route through `disconnectExternalStream()` so `'externalOff'` runs once. Replace inline `mobileMic` width check with `isMobileWeb()`.

### D4 — Status hint (mobile + External on)

**Choice:** When `isMobileWeb() && externalEnabled && audioRunning`, append:

`— without headphones, iOS may use the earpiece; plug in headphones or turn External off for speaker`

**Why:** Encodes built-in vs headset distinction without brittle device enumeration. Android users see iOS-specific clause only where relevant; harmless on Android.

Optional tighten at apply: prefix with `Mobile:` if status line too long.

### D5 — Subtitle copy (all viewports)

**Choice:** `Browser simulator — press Play for sound; External adds mic ring-mod input`

**Why:** Mobile users mis-tap External to hear sound; desktop users benefit from clarity too. Copy is viewport-agnostic; session code is mobile-only.

### D6 — Manual

**Choice:** Web **External** bullet adds **Mobile browsers** paragraph:

- External + mic → play-and-record session
- **No headset:** iPhone Safari often uses earpiece (top), not bottom speaker
- **With headset:** output in headphones is normal
- Turn External off or reload restores speaker; desktop app for full routing control
- Desktop browsers: not affected

Sync to `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`.

### D7 — Verification matrix

| Host | External | Headset | Expected |
|------|----------|---------|----------|
| iPhone Safari | off, Play | none | bottom speaker |
| iPhone Safari | on | none | earpiece likely; hint visible |
| iPhone Safari | on | wired/BT | headphones; hint optional |
| iPhone Safari | on→off | none | speaker restored without reload |
| Android Chrome | on | none | loudspeaker typical |
| Desktop Safari/Chrome | any | any | unchanged; no session calls |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Audio Session API missing on Android | Guard no-op; hints + manual remain |
| iOS earpiece persists with built-in + mic | Document; no false guarantee |
| `isMobileWeb()` true on small desktop window | Same as existing `mobileMic` echoCancellation scope |
| Change folder named `ios-*` | Scope documented in proposal; rename optional |

## Migration Plan

1. Land mobile-gated helper + hooks in `main.ts`
2. Subtitle + mobile status hint
3. Manual sync
4. Physical iPhone + Android spot-check; desktop regression (no session side effects)

## Open Questions

1. At apply: if `'playback'` on Play does not fix reporter who denies External, ship anyway — session reset on External off is the main win.
