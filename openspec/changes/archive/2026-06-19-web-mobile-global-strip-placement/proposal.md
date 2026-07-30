## Why

The global randomize strip (**Rand All**, **Rand Mods**, **Rand Resample**, **Rand waveforms**) sits at the bottom of the page below the knob grid and page pills. Operators must scroll past the entire panel to reach global actions that logically belong with transport and MIDI I/O. The same placement gap exists on mobile and desktop browser.

## What Changes

- **All viewports:** Move `.global-strip` to sit directly under **External MIDI** (still above mod bay / knobs)
- **Single DOM:** One `.global-strip`, one set of button IDs — no duplicate markup, no resize JS, no breakpoint-specific placement rules
- **HTML:** Nest strip inside `.transport-io` after `.external-controls` within `.controls-top`
- **CSS:** `.transport-io { flex-direction: column }` — strip follows External MIDI in document order on every viewport
- **Playwright:** Mobile and desktop emulation assert global strip appears above mod bay and below External MIDI

**Non-goals:** moving page-level **Randomize** / **Randomize mod** (those stay in page chrome); VCV/native desktop app parity

## Capabilities

### New Capabilities

- `web-global-strip-placement`: Browser layout for global randomize strip under External MIDI on all viewports; e2e regression

### Modified Capabilities

- (none)

## Impact

- `web/index.html` — `.transport-play`, `.transport-io`, move `.global-strip` from bottom of `#app`
- `web/src/style.css` — `.transport-io` column layout; remove bottom-only strip placement styles
- `web/e2e/global-strip-placement.spec.ts` — mobile + desktop layout order assertions
- No `main.ts` / WASM changes (button IDs and handlers unchanged)
