## ADDED Requirements

### Requirement: v2-global-crunchy-encoder
Desktop v2 and VST v2 SHALL expose a **Crunchy** encoder ring in the global strip. **Crunchy is global fuego for everything** — it SHALL run `Fuegoize` on **every** persisted knob on **every** module page, including **all per-page Crispy instances** (Audio row 7; expanded modules 1–5 and ADSR row 9) and **all musical rows** (Audio rows 0–6; expanded modules 1–5 rows 0–8; ADSR rows 0–8).

#### Scenario: Crunchy fuegoizes all musical rows on all pages
- **WHEN** global Crunchy is non-zero
- **THEN** every musical row on modules 0–6 receives global fuego via `Fuegoize(value, globalCrunchy, row)`
- **THEN** the XOR scramble matches v1 FUEG semantics

#### Scenario: Crunchy fuegoizes all Crispy instances
- **WHEN** global Crunchy is non-zero
- **THEN** each per-page Crispy knob value (`CrispyRowForPage`: row 7 on Audio; row 9 on expanded modules 1–5 and ADSR) is also passed through `Fuegoize(crispyValue, globalCrunchy, crispyRow)`
- **THEN** global Crunchy affects Crispy parameters themselves, not only musical rows

#### Scenario: Per-page Crispy stacks after global Crunchy
- **WHEN** both global Crunchy and a page's Crispy are non-zero
- **THEN** musical rows on that page receive global Crunchy fuego first
- **THEN** page Crispy applies a second fuego pass on that page's musical rows using the page's (post-Crunchy) Crispy amount
- **THEN** Crunchy and page Crispy are independent controls that stack; Crunchy does not replace page Crispy

#### Scenario: Crunchy is moddable and automatable
- **WHEN** a mod source is assigned to global Crunchy
- **THEN** effective global fuego follows modulation depth rules
- **THEN** VST v2 exposes `Global/Crunchy` as a host parameter

#### Scenario: Rand All may include Crunchy
- **WHEN** Rand All runs (default policy)
- **THEN** global Crunchy is randomized along with other non-Crispy targets unless user policy excludes globals

### Requirement: v2-encoder-ring-scene-voices
Encoder rings on v2 SHALL use two concentric scene rings plus a blended indicator dot: outer = Scene L effective value, inner = Scene R effective value, dot = current blended center.

#### Scenario: Ring geometry with two scene voices
- **WHEN** scene L and R centers differ for a parameter
- **THEN** the encoder displays two concentric value rings in scene endpoint colors
- **THEN** the indicator dot shows the blended value used after scene blend

#### Scenario: Mono scene collapse
- **WHEN** scene L equals scene R for a parameter
- **THEN** both rings coincide visually

### Requirement: v2-encoder-range-arcs
Encoder rings SHALL render min/max reachability arcs from `ProcessLite` slewed min/max values in v2.0. Those arcs SHALL match the effective parameter bounds produced by the unified Smart Grid range-preserving modulation path on the audio thread.

#### Scenario: Arcs visible in v2.0
- **WHEN** a parameter has non-zero modulation depth
- **THEN** the encoder displays min and max reachability arcs around the value rings

#### Scenario: Arcs match effective bounds
- **WHEN** `FroggersV2HostBridge` applies modulation for a row
- **THEN** UIState `minValues` / `maxValues` for scene voices equal the bridged effective bounds within test tolerance
- **THEN** painted arcs reflect those UIState values

### Requirement: v2-encoder-badges
Encoder rings SHALL display modulator badges and gesture badges.

#### Scenario: Modulator badge visible
- **WHEN** mod source VCO2 EF is routed to a row with non-zero depth
- **THEN** the encoder shows a modulator badge for that source

### Requirement: v2-encoder-hardware-banks
When the active module has more rows than physical encoders in the selected bank slot, desktop v2 SHALL provide bank switching to map encoders to row subsets (Sheaf bank/slot pattern).

#### Scenario: Four encoders on eight-row module
- **WHEN** a bank slot has four physical encoders and Audio module is visible
- **THEN** the user can switch banks to access rows 0–3 and 4–7 (including Crispy)
- **THEN** encoder hardware sends `ParamIncDec` for the visible bank mapping only
