# desktop-audio-export Specification

## Purpose
TBD - created by archiving change desktop-audio-export. Update Purpose after archive.
## Requirements
### Requirement: Transport bar record control

The desktop sim transport bar SHALL provide a **Record** control (red circle plus **RECORD** label) in a cluster on the **right** side of the header, to the right of the **MIDI** and **Audio** buttons. The first click while audio is playing SHALL start stereo capture. The second click SHALL stop capture and open a native save dialog for file name and location.

#### Scenario: Start recording during Play

- **WHEN** audio is playing and the user clicks **Record**
- **THEN** recording becomes active and the button shows a recording state
- **AND** stereo output from the sim is captured

#### Scenario: Stop and save

- **WHEN** recording is active and the user clicks **Record** again
- **THEN** capture stops
- **AND** a file chooser prompts for file name and export location
- **AND** the captured audio is written in the selected export format

#### Scenario: Record without Play

- **WHEN** audio is not playing and the user clicks **Record**
- **THEN** recording does not start
- **AND** the user receives feedback to press **Play** first

### Requirement: Export format selection

The record cluster SHALL provide exclusive format selectors for **WAV**, **MP3**, **FLAC**, and **OGG** stacked **vertically below** the Record row in the same right-aligned column. Selectors SHALL use **checkbox** affordance (tick box + label), not horizontal pill buttons. Exactly one format SHALL be selected at a time. **WAV** SHALL be the default on launch. The format column SHALL share row 2 with the mod rack (no empty header band under the transport controls).

#### Scenario: Layout order

- **WHEN** the user views the desktop header
- **THEN** **MIDI** and **Audio** appear to the left of **RECORD** on the transport row
- **AND** format checkboxes appear vertically under **RECORD** beside the mod rack

#### Scenario: Default format

- **WHEN** the desktop app launches
- **THEN** **WAV** is the selected export format

#### Scenario: Switch format before recording

- **WHEN** the user selects **MP3** (or FLAC or OGG) while not recording
- **THEN** the next completed recording exports in that format
- **AND** the save dialog default extension matches the selected format

### Requirement: Stereo export content

Exported files SHALL contain stereo audio at 44100 Hz sampled from the post-`applyStereoBus` left and right channels — the same stereo image the user hears during Play.

#### Scenario: Stereo file contents

- **WHEN** the user records while delay stereo width is active and exports WAV
- **THEN** the file has two channels reflecting L/R bus separation
- **AND** sample rate is 44100 Hz

### Requirement: Encoder availability

WAV export SHALL always be available. MP3, FLAC, and OGG export SHALL be attempted using JUCE audio format writers. If the selected encoder is unavailable in the build, the app SHALL show an error and SHALL NOT write a corrupt file.

#### Scenario: WAV always works

- **WHEN** the user exports with **WAV** selected
- **THEN** a valid stereo WAV file is written to the chosen path

#### Scenario: Missing encoder

- **WHEN** the user selects a format whose encoder is not linked in the build
- **AND** completes a recording
- **THEN** the app reports that the format is unavailable
- **AND** the user can select **WAV** and export the same captured buffer

