## ADDED Requirements

### Requirement: Mod source grid derives eligibility from manifest
Desktop v2 mod source cells, any compact route/dropdown summaries, depth editing targets, Rand Mods, and sequencer mod snapshot fields SHALL derive route eligibility from the Froggers v2 app manifest and permanent source catalog.

#### Scenario: Compact route summaries use manifest eligibility
- **WHEN** a row renders a compact modulation route summary or dropdown
- **THEN** it represents only source lanes allowed by the row manifest entry
- **THEN** all-zero depths remain equivalent to the cleared/off route state

#### Scenario: Rand Mods respects manifest eligibility
- **WHEN** Rand Mods randomizes a row
- **THEN** the selected source is allowed by that row's manifest entry
- **THEN** randomized depth uses the manifest-declared depth range and default policy

### Requirement: Permanent parameter modulation rack has fifteen Froggers source lanes
When a user opens a parameter's modulation detail view, desktop v2 SHALL expose a permanent 15-lane Froggers source rack. The rack SHALL contain VCO 1+2, VCO 2+3, VCO 1+3, VCO 1 EF, VCO 2 EF, VCO 3 EF, VCO 1+2 EF, VCO 2+3 EF, LFO 1, LFO 2, LFO 3, Random/Marbles 1, Random/Marbles 2, External Audio (audio rate), and External Audio (envelope follower). Each lane SHALL have depth zero/off by default and SHALL NOT require a separate source assignment step before depth can be edited.

#### Scenario: Parameter detail shows all permanent source lanes
- **WHEN** the user opens modulation detail for a manifest-eligible parameter
- **THEN** all fifteen permanent source lanes are represented
- **THEN** the audio-rate VCO lanes are VCO 1+2, VCO 2+3, and VCO 1+3
- **THEN** raw VCO 1, VCO 2, and VCO 3 audio-rate lanes are absent
- **THEN** every lane with depth zero contributes no modulation to the target
- **THEN** lane colors and labels match the manifest source catalog

#### Scenario: External audio source lanes are visible but unavailable when input is absent
- **WHEN** no external audio source is active or available
- **THEN** External Audio (audio rate) remains visible in the source rack with an unavailable/off state
- **THEN** External Audio (envelope follower) remains visible in the source rack with an unavailable/off state
- **THEN** Rand Mods does not choose unavailable external-audio lanes

#### Scenario: MIDI is not duplicated as a source lane
- **WHEN** the permanent source rack renders
- **THEN** MIDI CC A and MIDI CC B are absent from the source lanes
- **THEN** MIDI/controller input remains available through controller mappings and host-parameter mappings

#### Scenario: Whole-cluster EF is intentionally absent
- **WHEN** the permanent source rack renders
- **THEN** VCO 1+2+3 EF is absent
- **THEN** VCO 1+2 EF and VCO 2+3 EF remain available as adjacent-pair envelope followers

#### Scenario: VCO targets avoid self-feedback pair buses
- **WHEN** the user opens modulation detail for a VCO 1-owned parameter
- **THEN** VCO 1+2 and VCO 1+3 audio-rate lanes are unavailable for that target
- **THEN** VCO 2+3 can remain available for that target
- **WHEN** the user opens modulation detail for a VCO 2-owned parameter
- **THEN** VCO 1+2 and VCO 2+3 audio-rate lanes are unavailable for that target
- **THEN** VCO 1+3 can remain available for that target
- **WHEN** the user opens modulation detail for a VCO 3-owned parameter
- **THEN** VCO 2+3 and VCO 1+3 audio-rate lanes are unavailable for that target
- **THEN** VCO 1+2 can remain available for that target

### Requirement: Parameter detail uses a 4x4 encoder grid
Desktop v2 parameter-detail modulation view SHALL render sixteen encoder cells at the default standalone size: fifteen source-depth encoders plus one dedicated Crispy/target encoder. The Crispy/target encoder SHALL remain visible while source depths are edited.

#### Scenario: Parameter detail renders sixteen cells
- **WHEN** the user opens modulation detail for a manifest-eligible parameter at 1280x920
- **THEN** the view shows a 4x4 grid of encoder cells
- **THEN** fifteen cells correspond to the permanent source lanes
- **THEN** one cell corresponds to the dedicated Crispy/target encoder
- **THEN** no source lane is hidden behind scrolling at the default standalone size

#### Scenario: Crispy target remains available during depth editing
- **WHEN** the user is editing source depths for a target parameter
- **THEN** the dedicated Crispy/target encoder remains visible
- **THEN** the target cell reports the target parameter context and Crispy eligibility declared by the manifest

### Requirement: Encoders expose modulation through attenuated-centered CV LED and MOD drill-in
Desktop v2 parameter encoders SHALL show modulation state through an in-encoder CV LED indicator and a small clickable `MOD` label, not through per-source badges. The CV LED SHALL be centered on the centerpoint of the attenuated modulation range. Its red/green balance and intensity SHALL derive from rolling positive and negative displacement energy around that center, so sub-audio modulation reads like instantaneous signed movement while audio-rate modulation reads like signed energy/bias. Clicking the `MOD` label SHALL open the parameter-detail modulation page for that encoder.

#### Scenario: Encoder displays CV modulation state
- **WHEN** a parameter has nonzero modulation influence
- **THEN** its encoder shows a CV LED indicator inside the encoder
- **THEN** the LED center is aligned to the centerpoint of the attenuated range
- **THEN** negative displacement below the center contributes to red intensity
- **THEN** positive displacement above the center contributes to green intensity
- **THEN** near-zero displacement contributes little or no LED intensity
- **THEN** no per-source modulation badge is required to identify the source

#### Scenario: CV LED transitions continuously into audio-rate display
- **WHEN** modulation moves slowly enough for UI frames to follow the signed displacement
- **THEN** the LED appears red below the attenuated center, green above it, and dim near the center
- **WHEN** modulation crosses the center faster than UI frames can follow
- **THEN** the LED uses a rolling window of positive and negative displacement energy
- **THEN** balanced bipolar audio-rate modulation appears as mixed red/green brightness
- **THEN** biased audio-rate modulation leans toward the dominant red or green side

#### Scenario: MOD label opens parameter detail
- **WHEN** the user clicks the `MOD` label below or inside a parameter encoder
- **THEN** the UI opens the 4x4 parameter-detail modulation grid for that parameter
- **THEN** returning from detail restores the carousel module page and its parameter grid

### Requirement: LFO sources are first-class modulatable module outputs
Froggers v2 SHALL model LFO 1, LFO 2, and LFO 3 as first-class module outputs in the modulation source rack. LFO module parameters SHALL be manifest-declared modulation targets when eligible, so VCO, EF, random, external, and other LFO sources can modulate the LFOs themselves through the same depth semantics as other parameters.

#### Scenario: LFO parameter can receive modulation
- **WHEN** the user opens modulation detail for an eligible LFO parameter
- **THEN** the same permanent source rack is available
- **THEN** depth-zero lanes remain off
- **THEN** nonzero source depths can affect that LFO parameter according to manifest eligibility

### Requirement: Sheaf depth view preserves Froggers v2 UX
Froggers v2 SHALL keep the CV LED and `MOD` drill-in affordance on normal parameter encoders while using Sheaf-style modulation-depth drill-down semantics for parameter targets.

#### Scenario: Eligible row opens depth view
- **WHEN** the operator opens modulation-depth view for an eligible row
- **THEN** visible depth cells correspond to manifest-eligible sources
- **THEN** the Crispy/target cell remains visible as the sixteenth cell
