## ADDED Requirements

### Requirement: Six Marbles-style Random S&H sources
The app SHALL provide six random sample-and-hold modulation sources, labelled **Random S&H 1** through **Random S&H 6**, modelled on the Marbles random sampler. Five SHALL be stepped random voltages maintaining a remembered loop of stored values, with deja-vu behaviour selecting between repeating those values and overwriting them with fresh randoms. The sixth SHALL be the framework's own ganged random-walk generator, moving continuously rather than stepping, and is **not** affected by deja-vu.

#### Scenario: High deja-vu repeats a fixed loop
- **WHEN** a stepped source's deja-vu character is high and it is advanced repeatedly
- **THEN** the output cycles through the same remembered values
- **THEN** the cycle length matches that source's loop length

#### Scenario: Low deja-vu keeps producing new values
- **WHEN** a stepped source's deja-vu character is low and it is advanced repeatedly
- **THEN** stored values are progressively replaced with new randoms

#### Scenario: The smooth source ignores deja-vu
- **WHEN** Random S&H 6 is active
- **THEN** its output moves smoothly rather than stepping
- **THEN** its behaviour does not change with deja-vu

#### Scenario: Smooth source movement duration follows tempo
- **WHEN** the master clock's tempo changes
- **THEN** Random S&H 6 recomputes its movement duration so that one move spans sixteen quarter notes at the new tempo
- **THEN** its movement is tempo-proportional rather than locked to the quarter-note grid, which is what allows it to reuse the framework's ganged-random visualizer unmodified

### Requirement: The sources carry no source-level parameters
Random S&H sources SHALL expose no controls over their own behaviour — no loop length, deja-vu, spread, bias, rate, or slew control — and SHALL NOT occupy any bank slot. No dedicated page or additional bank SHALL be created for them. Each source's character SHALL be fixed at construction, chosen so the six differ usefully from one another.

This is distinct from their **modulation depth**, which is an ordinary bipolar encoder per target parameter, defaulting to neutral (no modulation). Those depth encoders are the operator's only direct control over a source, and modulating a depth is the second drill-in level.

#### Scenario: No Random S&H parameter appears on the grid
- **WHEN** every bank is enumerated
- **THEN** no bank contains a Random S&H source parameter
- **THEN** the number of banks is unchanged by the presence of these sources

#### Scenario: Depth is the control, and it starts off
- **WHEN** a parameter's modulation detail grid is opened for the first time
- **THEN** each Random S&H depth encoder reads neutral, contributing no modulation
- **THEN** the depth is bipolar, so the source can be applied in either direction

#### Scenario: Shaping happens through modulation
- **WHEN** the operator wants a source's effect to vary over time
- **THEN** they drill into that source's depth cell and modulate the depth itself
- **THEN** no control over the source's own behaviour exists anywhere

### Requirement: Stepped source character constants are new behavior
The stepped sources' character constants — spread and bias in particular — SHALL be new behavior implemented for this capability, not carried over from the original Froggers random sampler, which exposes no range-narrowing or centring control. Of the six sources, exactly one SHALL be configured narrow-range and centred; the remaining five SHALL be full-range.

#### Scenario: Exactly one source is narrow and centred
- **WHEN** the six sources' character constants are enumerated
- **THEN** exactly one source is narrow-range and centred
- **THEN** the other five sources are full-range

### Requirement: Sources advance on the master clock quarter-note pulse
Every source SHALL derive its rate from the master clock's quarter-note pulse, rather than from a free-running internal timer, each at its own fixed multiple or division of that pulse. These per-source rates SHALL be fixed: Random S&H 1 advances once per quarter note; Random S&H 2 advances twice per quarter note (eighth notes); Random S&H 3 advances three times per quarter note (eighth-note triplets); Random S&H 4 advances once per quarter note; Random S&H 5 advances once per four quarter notes; Random S&H 6 is smooth and is not stepped at all, per its own requirement.

#### Scenario: Advances track the clock
- **WHEN** the transport runs for a known number of quarter notes
- **THEN** each stepped source advances exactly that number multiplied by its own fixed rate ratio

#### Scenario: A bar means four quarter notes
- **WHEN** the master clock supplies quarter notes and no time signature
- **THEN** this capability defines one bar as four quarter notes
- **THEN** every "bar" referenced in a source's character means four quarter notes

#### Scenario: Tempo change tracks immediately
- **WHEN** the tempo is changed
- **THEN** the advance rate follows the new tempo without drift or reset

#### Scenario: External MIDI clock drives advancement
- **WHEN** the host is synchronized to an external MIDI clock
- **THEN** the sources advance on that external clock's quarter notes

#### Scenario: A missing clock plan is handled
- **WHEN** a processing block arrives with no clock plan
- **THEN** the sources do not advance and do not fault

### Requirement: Sources render their character as a visualizer
Each source SHALL provide a visualizer attached to the modulation source itself, so it appears as the underlay on every depth cell derived from that source. Stepped sources SHALL render their remembered values as a waveform across the loop, indicating the currently active position. The smooth source SHALL use the framework's existing ganged-random visualizer rather than a new one.

#### Scenario: Stepped visualizer matches the remembered values
- **WHEN** a stepped source's remembered values change
- **THEN** the rendered waveform reflects the new values
- **THEN** the indicated position matches the currently active value

#### Scenario: Smooth source reuses the framework visualizer
- **WHEN** Random S&H 6 is rendered
- **THEN** it uses the framework's ganged-random visualizer unmodified

#### Scenario: Depth cells show the visualizer
- **WHEN** a modulation detail grid is open
- **THEN** each Random S&H depth cell shows that source's visualizer as an underlay
