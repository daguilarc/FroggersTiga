# Delta — `froggers-app-surface-layout`

## MODIFIED Requirements

### Requirement: Bank selector with direct selection
The surface SHALL provide direct selection among banks. Arrow-based paging SHALL NOT be the primary navigation. Exactly one bank SHALL be active at a time, with a single authority for that selection.

Bank selection SHALL respond to a single click. The active bank SHALL be indicated by an inverted
background (full background-and-text inversion resumes once the pinned UI toolkit supports it on
single-click controls); a character appended to the label SHALL NOT be used as the selection
indicator. **Single-click operation takes precedence over the completeness of the inversion**
(operator decision 2026-07-28, superseding the earlier version of this requirement): a fully
inverted control that requires a double click is a failed implementation of this requirement.

Banks SHALL be ordered along the signal path: Audio, Envelope, Filter, Drive, Delay, Reverb. That
order SHALL have a single source of truth; the displayed labels SHALL be derived from the bank
definitions rather than maintained as a parallel list.

#### Scenario: Direct bank selection
- **WHEN** the operator selects a bank
- **THEN** that bank's parameters populate the encoder grid
- **THEN** no second, divergent bank-selection state exists

#### Scenario: Selection responds to a single click
- **WHEN** the operator single-clicks a bank button
- **THEN** that bank becomes active and its parameters populate the grid

#### Scenario: Selection is shown by inversion
- **WHEN** a bank is active
- **THEN** its button renders with an inverted background
- **THEN** no marker character is appended to any bank label

#### Scenario: Banks follow the signal path
- **WHEN** the bank selector is displayed
- **THEN** the banks read Audio, Envelope, Filter, Drive, Delay, Reverb in that order
- **THEN** the label list is derived from the bank definitions, not separately maintained

### Requirement: Layout integrity at the target window size
The surface SHALL lay out without overlap or clipping at the target window size, and its regions SHALL be verified by automated bounds tests.

The declared window size SHALL account for every region the runtime renders, including chrome
auto-flowed outside the app's own computed content area. A declared height that omits that chrome
clips it, because the runtime derives the window height from the declared height with no
additional slack.

#### Scenario: No overlapping regions
- **WHEN** the surface is laid out at the target size
- **THEN** the scope band, chrome, bank selector, and encoder grid regions do not overlap
- **THEN** every encoder cell lies fully within the grid region

#### Scenario: Nothing is clipped at the default size
- **WHEN** the application is launched at its default window size
- **THEN** the bank row, scene controls, tempo control and right sidebar are fully visible
- **THEN** no region requires the operator to resize the window to be reachable

## ADDED Requirements

### Requirement: Parameter short labels survive truncation
Rendered short labels SHALL remain distinguishable after the display layer's fixed-width
truncation. A short label SHALL NOT rely on characters beyond the truncation limit to carry the
information that distinguishes it from its siblings.

#### Scenario: Envelope labels keep their oscillator number
- **WHEN** the envelope bank's parameters are rendered
- **THEN** each label identifies which oscillator it controls
- **THEN** the identifying digit is still present after truncation

### Requirement: The scope band shows what the operator hears, in two distinct bands
The scope band SHALL comprise two visually distinct panels with different sources: the top panel
SHALL trace the audio-rate per-oscillator signal **after** envelope gating, so that silence
renders as flat lines; the bottom panel SHALL trace the envelope of those same post-gate signals,
so that it shows each voice's amplitude contour rising and falling with its envelope. Traces SHALL use blue, yellow, and magenta for the three oscillators. The display SHALL be
stable — waveforms cycle-aligned or window-stabilised, not free-scrolling phase slices.

A scope that animates while the instrument is silent, or two panels rendering identical content,
SHALL be treated as a failed scope band.

The tap-point and single-evaluation constraints on the top panel are specified in
`froggers-vco-topology`; both panels SHALL be built from the standard scope visualizer, with no
app-side waveform rasterization.

Envelope followers used for the display SHALL be separate instances from those registered as
modulation sources. Re-tapping a modulation source to feed a display would change what the
modulation system hears, which is a signal-path change made for a cosmetic reason.

*(Audit note, 2026-07-28, revised same day: the bottom panel's source was originally written as
"the LFO-rate envelope followers", meaning the existing modulation-source followers. Those are
tapped **pre-gate**, on an oscillator whose generator has no amplitude term, so they hold a fixed
level and the panel would have rendered three nearly-motionless lines. The defect was the tap
point, not the follower — the same defect the top panel has — so the fix is the same fix applied
twice, and no change of signal is needed.)*

#### Scenario: Silence renders flat
- **WHEN** the transport is stopped or the envelope gate is closed
- **THEN** the top panel's traces are flat lines

#### Scenario: The two bands differ
- **WHEN** the instrument is playing
- **THEN** the top panel shows audio-rate waveforms and the bottom panel shows their amplitude
  contours
- **THEN** the two panels are visibly non-identical

#### Scenario: The envelope band responds to the envelope
- **WHEN** a voice's envelope opens and then releases
- **THEN** that voice's trace in the bottom panel rises and then falls with it
- **THEN** the trace is not a fixed level

#### Scenario: Display followers do not disturb modulation
- **WHEN** the scope band's envelope followers are added
- **THEN** the followers registered as modulation sources are unchanged in input and output

#### Scenario: Stable waveform display
- **WHEN** a sustained tone plays
- **THEN** the displayed waveform is stable rather than scrolling chaotically between refreshes

### Requirement: Control labels are rendered, not merely set
Every labelled control SHALL have its label visibly rendered in the running application. Setting a
label field that the rendering backend does not draw SHALL NOT satisfy any labelling requirement
in this spec; verification of labelling requirements SHALL be visual.

#### Scenario: Tempo and blend labels are visible
- **WHEN** the surface is displayed
- **THEN** the text "BPM" and "Scene blend" are visible adjacent to their controls in the running
  application, confirmed by observation

### Requirement: Scene controls act on the blend directly
The two scene buttons SHALL move the scene blend to its opposite extremes, so that pressing one
produces an immediate, visible change in the blend control and an audible change in the patch.
They SHALL be labelled as scenes. The blend control SHALL be labelled.

The app SHALL NOT author a numeric readout of the blend value of its own. The pinned UI toolkit
attaches an unconditional value text box to every slider, which the app cannot suppress per
control; that readout is therefore **out of the app's control and outside this requirement**.
Removing it is an upstream item, not an acceptance condition here.

*(Revised 2026-07-28 by audit: as first written this requirement demanded that no raw
floating-point blend value be shown, which nothing in the app can satisfy at the pinned toolkit
version. A requirement no implementation can meet fails every review that reads it honestly and
gets quietly ignored by every review that does not.)*

#### Scenario: Scene buttons move the blend
- **WHEN** the operator presses either scene button
- **THEN** the blend control moves to the corresponding extreme
- **THEN** the patch changes audibly

#### Scenario: Blend control is labelled
- **WHEN** the scene area is displayed
- **THEN** the blend control carries a descriptive label, visibly rendered
- **THEN** the app contributes no numeric blend readout of its own

### Requirement: Tempo control communicates when it has no effect
The tempo control SHALL be labelled, and SHALL indicate when changing it cannot affect the
instrument — specifically while the transport is stopped, when the gate it drives is closed
outright.

#### Scenario: Tempo is identifiable
- **WHEN** the transport area is displayed
- **THEN** the tempo control is labelled and legible within the visible window

#### Scenario: Tempo indicates inactivity
- **WHEN** the transport is stopped
- **THEN** the tempo control indicates that changes take effect once the transport runs

### Requirement: Transport controls respond to a single click
Play and Stop SHALL each act on a single click. A transport control that requires a double click
SHALL be treated as broken, regardless of how it looks: an instrument whose Stop cannot be pressed
is not shippable, and appearance never outranks operation.

Where the pinned UI toolkit dispatches only double clicks for a control kind, the app SHALL choose
a control kind that dispatches on single click, even at the cost of the custom appearance. Any
control that genuinely cannot be converted SHALL be recorded as a known limitation and stated to
the operator — never left to be discovered in use.

#### Scenario: Play and Stop respond to one click
- **WHEN** the operator single-clicks Play, and then single-clicks Stop
- **THEN** the transport starts and then stops, confirmed in the running application

#### Scenario: Unconvertible controls are disclosed, not hidden
- **WHEN** a control cannot be made single-click at the pinned toolkit version
- **THEN** that limitation is recorded against the toolkit as an upstream item
- **THEN** the operator has been told which controls are affected

### Requirement: Modulation-source cells animate their source
An encoder cell showing a modulation source SHALL display that source's live motion. A visualizer
that exists, compiles and is unit-tested but is never attached to the cell it was written for
SHALL NOT be treated as satisfying this requirement — the attachment is the requirement.

#### Scenario: Random sample-and-hold animates
- **WHEN** a modulation detail page shows a random sample-and-hold lane and the clock is running
- **THEN** that cell animates on each new value, confirmed by observation

#### Scenario: Every visualizer has a live attachment
- **WHEN** the set of modulation-source visualizers is enumerated
- **THEN** each one is attached to the cells it serves by production code, not only by tests

### Requirement: No product name is drawn on the canvas
The surface SHALL NOT render the product name as canvas text. Launcher and window-title metadata
are unaffected and SHALL continue to identify the application.

#### Scenario: Canvas carries no title text
- **WHEN** the surface is displayed
- **THEN** no product-name text is drawn on the canvas
- **THEN** the launcher entry and window title still identify the application
