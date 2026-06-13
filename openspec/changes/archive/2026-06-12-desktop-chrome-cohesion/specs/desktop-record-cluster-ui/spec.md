## ADDED Requirements

### Requirement: Format toggles in header row 2 beside mod rack

The four export format toggles (WAV, MP3, FLAC, OGG) SHALL stack vertically in the **right column of header row 2**, beside the mod rack. They SHALL NOT occupy a full-height column that leaves empty space under the transport row.

#### Scenario: Formats beside mod rack

- **WHEN** the user views the desktop header
- **THEN** format toggles align vertically in the right 120 px of the mod rack row

### Requirement: Format toggles have equal row height and typography

The four export format toggles SHALL occupy **equal height** rows within the format column. All four SHALL use the same font size and weight (11 pt bold). Remainder pixels from integer division SHALL be distributed across rows, not assigned only to the last row.

#### Scenario: OGG matches WAV size

- **WHEN** the user views the record export cluster at default header size
- **THEN** the **OGG** label is the same visual size as **WAV**, **MP3**, and **FLAC**
- **AND** no format row is clipped below the others

#### Scenario: Four rows fit cluster

- **WHEN** the format column is allocated in row 2
- **THEN** all four format toggles are fully visible without vertical clipping

### Requirement: Record cluster dimensions are explicit

The record export cluster format column SHALL use explicit width **120 px**. The **RECORD** control SHALL sit in transport row 1 at ~28 px height. Format rows SHALL be ≥18 px each (preferred 20 px via `kFormatRowH`).

#### Scenario: RECORD label not clipped

- **WHEN** the user views the transport row
- **THEN** the red record circle and **RECORD** text are fully visible

## MODIFIED Requirements

### Requirement: Exclusive format toggles (checkbox appearance)

Four `juce::ToggleButton`s in radio behavior: WAV, MP3, FLAC, OGG. Default **WAV** on launch. Toggles SHALL use **checkbox tick** affordance (not radio-pill appearance), consistent chrome, and **equal layout bounds**.

#### Scenario: Format selection

- **WHEN** the user selects OGG
- **THEN** OGG is selected exclusively
- **AND** OGG label rendering matches other format labels

#### Scenario: Checkbox chrome

- **WHEN** the user views format toggles
- **THEN** each toggle shows a tick box beside its label
