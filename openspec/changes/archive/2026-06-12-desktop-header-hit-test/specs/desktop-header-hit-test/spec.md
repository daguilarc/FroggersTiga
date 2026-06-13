## ADDED Requirements

### Requirement: Transport buttons receive clicks at startup

Play, Stop, External, Audio, and MIDI controls in the header transport row SHALL receive mouse clicks without requiring any other UI interaction first.

#### Scenario: Play starts audio

- **WHEN** the user clicks **Play** immediately after launching the app
- **THEN** audio processing starts
- **AND** the Play button becomes disabled and Stop becomes enabled

#### Scenario: Audio settings opens

- **WHEN** the user clicks **Audio** in the transport row
- **THEN** the audio device settings dialog opens

#### Scenario: MIDI settings opens

- **WHEN** the user clicks **MIDI** in the transport row
- **THEN** the MIDI settings dialog opens

### Requirement: Record cluster does not cover the full header

`RecordExportCluster` component bounds SHALL equal the union of the RECORD control bounds and the format-toggle column bounds. The component SHALL NOT use the full header rectangle as its bounds.

#### Scenario: Cluster bounds vs header

- **WHEN** the header is laid out at default window size
- **THEN** the record cluster bounds width is at most `kRecordClusterW` on the right side
- **AND** the cluster bounds do not extend over the Play button area

### Requirement: Record cluster passes clicks to siblings

The record cluster container SHALL call `setInterceptsMouseClicks(false, true)` so clicks on non-child areas pass through to components behind it.

#### Scenario: Click outside cluster children

- **WHEN** the user clicks the transport row left of the RECORD control
- **THEN** the click reaches the intended transport button
- **AND** the record cluster does not consume the event

### Requirement: RECORD and format toggles remain clickable

RECORD and the four format toggles SHALL remain fully clickable after the hit-test fix.

#### Scenario: Record click

- **WHEN** the user clicks **RECORD** while audio is playing
- **THEN** recording starts or stops per existing export flow

#### Scenario: Format toggle

- **WHEN** the user clicks the **OGG** format toggle
- **THEN** OGG is selected exclusively

## MODIFIED Requirements

### Requirement: Chrome layout uses shared constants

`DesktopChromeLayout.hpp` SHALL define mod rack box width, gap, min width, record cluster width, format row height, transport row height, and mod rack row height in one place. **Record/export chrome SHALL be positioned via explicit child rectangles; the cluster component bounds SHALL match those rectangles only.**

#### Scenario: Record cluster height matches format rows

- **WHEN** the record export cluster is laid out in row 2
- **THEN** its format area accommodates four equal rows without clipping the last row
- **AND** transport buttons outside the cluster remain clickable
