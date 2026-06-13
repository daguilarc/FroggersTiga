## MODIFIED Requirements

### Requirement: Web mod dropdown options

On web, each knob row SHALL provide a `<select>` with options `None | VCO level | Marbles 1 | Marbles 2`. Selection SHALL update core mod assignment for that parameter on the current page.

#### Scenario: Web mod source set

- **WHEN** the user selects **Marbles 2** for a knob on the Filter page
- **THEN** core stores mod index `6` for that parameter
- **AND** the dropdown label reads **Marbles 2**

#### Scenario: VCO level option label

- **WHEN** the user views mod source options in the browser
- **THEN** index 4 is labeled **VCO level** (not VCO Envelope or VCO feat)
