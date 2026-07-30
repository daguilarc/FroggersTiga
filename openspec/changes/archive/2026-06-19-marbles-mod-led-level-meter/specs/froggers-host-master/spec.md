## MODIFIED Requirements

### Requirement: Mod rack topology differs by host

Mod rack cells SHALL come from `HostPanelLayout::kModRackCatalog`. Scope capacity SHALL be `HostPanelLayout::kScopeSampleCapacity` (96 samples).

| Mod index | Source | Desktop | Web | VST/AU | VCV | Presentation (desktop/web/VST) | VCV presentation |
|-----------|--------|---------|-----|--------|-----|--------------------------------|------------------|
| 0 | MIDI CC 1 | yes | yes | no | no | Scope | — |
| 1 | MIDI CC 2 | yes | no | no | no | Scope | — |
| 4 | VCO Envelope | yes | yes | yes | yes | Scope | LED (binary; unchanged) |
| 5 | Random 1 | yes | yes | yes | yes | LED (level-proportional) | LED (level-proportional) |
| 6 | Random 2 | yes | yes | yes | yes | LED (level-proportional) | LED (level-proportional) |

**Cell counts:** desktop **5**; web **4** (0, 4, 5, 6); VST/AU **3** (4, 5, 6); VCV **3** (4, 5, 6).

Random LED cells (indices 5 and 6) on desktop, VST/AU, and VCV SHALL use `ModLedDisplayBrightness` from `sim/ModLedBrightness.hpp`; web SHALL use its projection from `web/src/hostDisplay.generated.ts`. No host SHALL retain a local binary on/off threshold for those indices. Brightness SHALL reach full at CV ≥ 0.55 when the host presentation is active (audio running on desktop/web/VST; during `process()` on VCV).

#### Scenario: Web mod bay excludes CC 2

- **WHEN** the browser sim renders the mod bay
- **THEN** exactly four entries appear in order CC 1, VCO Envelope, Random 1, Random 2
- **THEN** no CC 2 scope, enable control, assignment path, or ingestion exists

#### Scenario: VST mod rack excludes CC cells

- **WHEN** the hosted plugin editor renders the mod rack
- **THEN** only indices 4, 5, and 6 appear with scopes on 4 and level-proportional LEDs on 5/6

#### Scenario: Cross-host LED curve parity

- **WHEN** `GetCvOut(5)` is `0.3` with active presentation on desktop, web, VST, or VCV
- **THEN** Random 1 LED brightness equals `ModLedDisplayBrightness(0.3, true)` on that host
