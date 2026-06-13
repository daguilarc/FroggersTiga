## ADDED Requirements

### Requirement: Mod meters persist last CV when audio stops

When audio stops, mod bay meter fills for VCO level, Marbles 1, and Marbles 2 SHALL display the **last known CV level** at reduced opacity, not snap to zero.

#### Scenario: Stop after Marbles activity

- **WHEN** the user stops audio after Marbles produced non-zero CV
- **THEN** Marbles meter fills remain visible at dimmed opacity
- **AND** fill width reflects the last level before stop

### Requirement: Marbles meters emphasize steps while playing

When Marbles CV changes by more than a small threshold during Play, the corresponding meter cell SHALL show a brief visual step emphasis (CSS class or animation) within one UI refresh cycle.

#### Scenario: Marbles step during Play

- **WHEN** audio is playing and Marbles 1 level jumps to a new value
- **THEN** the Marbles 1 meter cell shows a visible step emphasis
- **AND** the new level is reflected in fill width

### Requirement: Mod bay documents meter semantics

The mod bay area SHALL include helper text stating meters show **CV level while playing**.

#### Scenario: Mod bay visible

- **WHEN** the mod bay is expanded
- **THEN** helper text “CV level while playing” is visible near the mod source controls
