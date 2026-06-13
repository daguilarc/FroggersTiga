## Why

The web Delay page was built with ad-hoc chrome (orange border, per-row hint show/hide) that breaks the unified knob-column grid. **Randomize mod** on Delay assigns mod sources, which triggers `applyDelayKnobHints()` to hide hint text on patched rows — column heights jump while unpatched rows keep hints, producing the uneven box sizes in the screenshot. Separately, VCO morph buttons do not update reliably (gated on `wasmPage === 0`, no engine guard on click), the global button still reads **Rand waves**, mod sources still say **VCO level** (desktop already says **VCO Envelope**), and Marbles mod indicators use oscilloscope traces that read as noise — a green on/off LED matches how Marbles actually behaves (stepped CV).

## What Changes

- **Delay page parity:** Remove delay-only orange chrome; unify knob-column layout across all six host pages. Reserve fixed hint slot height; never toggle hint visibility based on mod assignment.
- **Randomize mod layout stability:** Patching rows via randomize mod SHALL NOT change knob-column dimensions.
- **VCO Envelope naming:** Rename **VCO level** → **VCO Envelope** in web mod labels, mod bay, mod dropdowns, and `ParamDisplayNames` row 6 on Audio page (align with desktop mod rack).
- **VCO morph fixes:** Show morph buttons when `hostPage === 0` (Audio); require engine for click; rename button to **Rand waveforms**; update SVG on click via immediate local cycle + worklet `postScreen`.
- **Marbles LED (web + desktop):** Replace Marbles 1/2 oscilloscope traces with a green LED that is on when CV level > threshold, off otherwise. Keep continuous trace for VCO Envelope only.
- **Marbles S&H labeling:** Every user-visible Marbles mod source label SHALL include **S&H** (e.g. **Marbles 1 S&H**, **Marbles 2 S&H**) on web mod bay, mod dropdowns, and desktop mod rack — reflecting sample-and-hold behavior.

## Capabilities

### New Capabilities

- `sim-mod-marbles-led`: Marbles mod sources use S&H LED indicator instead of scope trace; labels read **Marbles 1 S&H** / **Marbles 2 S&H**.

### Modified Capabilities

- `web-knob-column-cells`: All six pages share identical column cell geometry; no delay-only border or mod-gated hint hiding.
- `web-vco-morph-inline`: Morph visibility, click guard, button label **Rand waveforms**.
- `sim-parameter-display-names`: Audio row 6 and mod source label **VCO Envelope**.
- `host-ui-delay-page`: Remove delay-accent chrome requirement; delay hints use reserved slot without layout shift.

## Impact

- `web/src/main.ts` — delay hints, morph render, mod labels, mod bay init
- `web/src/style.css` — remove `.delay-page` accent; fixed hint min-height
- `web/index.html` — Rand waveforms button text
- `web/src/CvScopeCanvas.ts` or new `ModLedIndicator.ts`
- `sim/ParamDisplayNames.hpp` — VCO Envelope
- `desktop/Source/ModModuleBox.cpp` — Marbles LED instead of stepHold scope
- `desktop/Source/ModRackPanel.cpp` — Marbles 1 S&H / Marbles 2 S&H module titles
- `desktop/Source/CvScopeDisplay.*` or new `ModLedIndicator` component
- Docs: `sim-manual.md`, `quick-dict.md` sync via existing script
