## ADDED Requirements

### Requirement: Bump and comb filters expose a transfer function
The resonant bump filter and the comb filter SHALL each expose Sheaf's transfer-function interface, reporting frequency response derived from their current coefficients.

#### Scenario: Response follows parameter changes
- **WHEN** the bump filter's frequency, gain, or Q is changed
- **THEN** its reported frequency response changes correspondingly

#### Scenario: Comb response reflects delay and feedback
- **WHEN** the comb filter's delay or feedback is changed
- **THEN** its reported response changes its peak spacing or peak sharpness accordingly

### Requirement: A visualizer plots filter frequency response
The app SHALL provide a visualizer that samples a transfer function across the audible frequency range and renders it as a polyline, using Sheaf's portable draw commands. It SHALL be attachable as an underlay to filter parameters.

#### Scenario: Bump response is rendered
- **WHEN** the bump filter's parameters are displayed
- **THEN** a response curve is drawn showing its peak or notch at the configured frequency

#### Scenario: Comb response is rendered
- **WHEN** the comb filter's parameters are displayed
- **THEN** a response curve is drawn showing its periodic peak structure

#### Scenario: Extreme settings remain renderable
- **WHEN** the comb feedback is set near or beyond self-oscillation
- **THEN** the plotted curve contains only finite values
- **THEN** the visualizer bounds the displayed magnitude rather than drawing out of frame

### Requirement: Filter visualizers follow the app's visualizer conventions
Filter response visualizers SHALL be implemented as standard Sheaf visualizers and SHALL be dimmed-behind-the-knob underlays consistent with other parameter visualizers, not bespoke UI panels.

#### Scenario: Underlay renders beneath the encoder
- **WHEN** a filter parameter with a response underlay is displayed
- **THEN** the response curve renders beneath the encoder body
- **THEN** the encoder body is dimmed so the underlay remains visible
