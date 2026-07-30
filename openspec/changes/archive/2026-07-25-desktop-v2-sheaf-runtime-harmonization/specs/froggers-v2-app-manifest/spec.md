## ADDED Requirements

### Requirement: Manifest pages exclude Random S&H module
`froggers-v2-app-manifest` / product page inventory SHALL list Audio, Envelope, Filter, Drive, Reverb, and Delay (names per product contract) and SHALL NOT list a Random S&H module page. Host-parameter inventory SHALL omit Step chance, Deja vu, Bag size, Slew, and Random-page expansion/Crispy axes.

#### Scenario: Validators reject Random module page
- **WHEN** manifest validation runs
- **THEN** no product page display name equals Random S&H as a module page
- **THEN** host-parameter count matches the post-deletion inventory

### Requirement: Audio has no cross-coupler control
The Audio module SHALL NOT expose any cross-coupler parameter — neither the legacy bipolar `Cross-coupler` row nor per-pair explicit couplers. The cross-coupler is removed as redundant with the drilldown modulation matrix (design D11): VCO→VCO phase coupling, when a patch wants it, is a drilldown modulation assignment — never a dedicated control and never a hardcoded DSP term.

#### Scenario: no cross-coupler present
- **WHEN** the Audio module section is shown
- **THEN** no cross-coupler control (bipolar or per-pair) is present
- **THEN** the manifest emits no `crossCouplers` entry

### Requirement: Envelope module is ASR
The Envelope module (formerly Pair-AR) SHALL expose per-VCO Attack, Sustain, and Release with full-word labels. Sustain is a level held while gated; Release runs after gate-off. Full ADSR decay knee is out of scope.

#### Scenario: Three ASR controls per VCO
- **WHEN** the Envelope module section is shown
- **THEN** each of VCO 1/2/3 shows Attack, Sustain, and Release labels
- **THEN** the section title is Envelope
