## Why

On sim hosts (desktop, web, VCV), Audio page row 7 must be a dedicated **Phase mod 3** knob — not firmware **OLVL** / “VCO level”. DSP and the canonical label table (`ParamDisplayNames.hpp`) already implement this, but operator docs and some duplicate label tables still describe the old OLVL semantics. That mismatch is why the change looked unfixed: users read stale docs or conflate **VCO Envelope** (mod source index 4) with the Audio page knob.

## What Changes

- Lock sim Audio row 7 (param index 6) to **Phase mod 3** label and PM3 DSP on all sim hosts — matching desktop UI and website.
- Align all operator docs with `ParamDisplayNames` (row 7 = Phase mod 3; row 8 = Crispy; mod source 4 = VCO Envelope).
- Remove or consolidate duplicate label tables (`docs/sim-manual.md`, `docs/quick-dict.md`, `web/src/main.ts` `HOST_PAGE_LABELS`) so they cannot drift from the MIT header.
- Document the distinction: **VCO Envelope** mod rack scope = slow CV from VCO mix output; **Phase mod 3** knob = VCO2→VCO3 PM depth when cross-coupler is CW.
- **Non-breaking** for Daisy Field firmware: hardware keeps OLVL on knob 7; sim-only `SetSimDedicatedPm3Knob(true)` path unchanged.

## Capabilities

### New Capabilities

- `sim-pm3-knob-parity`: Sim Audio page row 7 label, DSP routing, and cross-host consistency with desktop/website authority.
- `sim-operator-doc-parity`: Operator manuals and quick-dict entries match `ParamDisplayNames` on every sim host.

### Modified Capabilities

- (none — no baseline `openspec/specs/` yet)

## Impact

- `sim/ParamDisplayNames.hpp` — authority (verify, no semantic change expected)
- `src/core/FroggersEngine.hpp` — `SetSimDedicatedPm3Knob` path (verify only)
- `docs/sim-manual.md`, `docs/quick-dict.md` — stale “VCO level” / “Crunch” rows
- `SIM_MANUAL.md`, `QUICK_DICT.md`, `web/public/*` — verify parity; rebuild embedded/dist assets
- `web/src/main.ts` — duplicate `HOST_PAGE_LABELS` must mirror header or be generated
- `desktop/Source/PanelBackend.hpp` — already uses `ParamDisplayNames` (verify)
- `openspec/changes/vcv-vst-field-parity-panel` — prerequisite for VCV Phase B; PM3 row 7 + VCO Envelope mod-source separation captured in that change's design/specs/tasks
