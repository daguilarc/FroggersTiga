## MODIFIED Requirements

### Requirement: Performance band controls are labeled at 1280px
The performance band SHALL label every operator-visible slider and toggle so an operator can identify scene blend endpoints, performance macro lanes, and marbles indicators without opening other pages.

#### Scenario: Scene blend is identifiable
- **WHEN** the performance band renders at 1280px width
- **THEN** the scene blend slider shows left and right endpoint labels tied to the active S1/S2/S3 ordinals
- **THEN** an operator can determine which scene endpoints the blend interpolates between

#### Scenario: Performance macros are not anonymous toggles
- **WHEN** the performance band renders at 1280px width
- **THEN** performance macro toggles and depth sliders use manifest-backed display labels rather than single-letter codes alone

### Requirement: Random source indicators use Random S&H UI names
Performance-band random mod source indicators SHALL display **Random S&H 1** and **Random S&H 2**. The substring **Marbles** SHALL NOT appear in operator-visible chrome. Manual copy MAY reference Mutable Instruments Marbles per `sim-operator-doc-parity`.

#### Scenario: Random S&H labels legible at default width
- **WHEN** the performance band renders at 1280px width
- **THEN** labels read **Random S&H 1** and **Random S&H 2** without ellipsis
- **THEN** no label reads **Marbles 1** or **Marbles 2**

### Requirement: Performance band grid has no overlapping controls
At 1280px width, performance band controls SHALL occupy non-overlapping grid cells with readable labels. Sliders and toggles SHALL NOT share bounds with scene scope radios or other unrelated controls.

#### Scenario: Scene and gesture row readable
- **WHEN** the performance band renders at 1280px width
- **THEN** scene buttons, blend slider, gesture toggles, and random S&H indicators each have distinct non-overlapping bounds
- **THEN** no control label is truncated to `"..."` for lack of width
