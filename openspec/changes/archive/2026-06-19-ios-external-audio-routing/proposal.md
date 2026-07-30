## Why

On **mobile browsers** (phone/tablet), enabling **External Audio** (`getUserMedia`) while the synth plays can mis-route output — most visibly on **iPhone Safari**, where iOS sends audio to the **earpiece (receiver)** instead of the bottom loudspeaker when no headset is connected. iOS WebKit treats simultaneous mic capture and Web Audio playback as a **play-and-record** session (telephony-style routing). Users report quiet, tinny audio and assume the sim is broken.

**Desktop browsers are out of scope:** macOS/Windows/Linux browsers handle output routing without this class of bug. No desktop code changes.

The web sim gates mic access behind **External Audio: Off** by default. This change targets **mobile web** session hygiene, copy, and honest routing guidance — not a guaranteed loudspeaker override while mic is active.

## What Changes

- **Web (`main.ts`), mobile only:** Audio Session lifecycle around External enable/disable when `isMobileWeb()` — reset to `auto` before capture, `play-and-record` after mic connects (helps headset routing on iOS), `playback` → `auto` on release; assert `playback` on Play when External is off (sticky-session cleanup)
- **Web UX (mobile):** Status hint when External is on — built-in speaker vs headset expectations (earpiece warning applies to built-in output, not headphones)
- **Web copy (all viewports):** Subtitle clarifies Play vs External so mobile users do not enable mic just to hear sound
- **Docs:** Mobile browser External Audio routing note in manual sync copies
- **Honest scope:** WebKit exposes no reliable loudspeaker override on iPhone while mic is active without a headset; mitigation + session reset + docs

**Non-goals:** Desktop/VST audio routing, disabling External on mobile, native apps, guaranteed loudspeaker during built-in-speaker + mic capture on iOS

## Capabilities

### New Capabilities

- `web-mobile-external-audio-routing`: Mobile-web Audio Session sync, Play/External lifecycle, status hints, subtitle copy, manual sync

### Modified Capabilities

- (none — no archived baseline specs in `openspec/specs/`)

## Impact

- `web/src/main.ts` — `isMobileWeb()`, session helper (no-op on desktop), hooks in `setExternalEnabled`, `disconnectExternalStream`, `startAudio`
- `web/index.html` — subtitle wording
- `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md` — Host guide Web subsection (mobile routing)
