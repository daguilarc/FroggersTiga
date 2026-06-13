## ADDED Requirements

### Requirement: Five adjacent sub-modules

The desktop JUCE application SHALL display five panels side by side labeled Audio, Marbles, Reverb, Filter, and Drive. Each panel SHALL have **Randomize** and **Randomize mod**, eight parameter rows (7 + FUEG) with full name labels, mod input jacks, and vertical sliders. v2.1 SHALL NOT use per-knob mod dropdowns or duplicate mini-OLED rows.

#### Scenario: All panels visible

- **WHEN** the desktop window opens at the current five-column width with mod dropdowns removed
- **THEN** all five panels are visible and parameter names are not truncated to `V...`

### Requirement: Mod rack with patch cables

Above the five panels, the app SHALL show four mod **module boxes**: MIDI, VCO feat, Marbles 1, Marbles 2 — each with a live meter and **output jack**. `PatchCableOverlay` SHALL sit above the panel row, own drag gestures, and register port bounds from module boxes and sub-module rows (including FUEG). Patch cables SHALL follow **VCV Rack** drag-and-drop semantics ([manual](https://vcvrack.com/manual/GettingStarted)): drag from mod **output** or empty gray **input** jack → cable follows cursor (after ≥4px move) → drop on the opposite port type to connect; drop on empty space to cancel; drag existing plug to void to disconnect.

#### Scenario: Drag-and-drop patch (not two-click)

- **WHEN** the user drags from Marbles 1 output and drops on Audio V1VO input
- **THEN** a cable connects without a prior “arm source” click step

#### Scenario: One source to two destinations

- **WHEN** the user completes two drag patches from Marbles 1 output to Audio V1VO and Audio FUEG inputs
- **THEN** two cables are visible and both assignments are stored

### Requirement: Native audio I/O

The desktop app SHALL use JUCE audio device I/O. It SHALL link `froggers_core` and `DesktopHostIO` only.

#### Scenario: Device callback runs engine

- **WHEN** audio device delivers a buffer at 44.1 kHz after Play
- **THEN** `DesktopHostIO::ProcessBlock` fills the output buffer

### Requirement: Transport bar

The desktop app SHALL provide **Play** (green) and **Stop** (red). Audio SHALL NOT start on launch.

#### Scenario: Stop silences output

- **WHEN** the user presses Stop while audio is running
- **THEN** the audio callback is removed and output is silent

### Requirement: External ring-mod input

The desktop app SHALL expose **External: Off | L | R**. **Off** feeds zero external input (VCO-only per MANUAL).

#### Scenario: Off is VCO-only

- **WHEN** External is Off and Play is pressed
- **THEN** `ProcessBlock` receives zero external samples

### Requirement: MIDI settings

The desktop app SHALL provide a MIDI settings dialog: input device, output device, **one** input channel + CC for the MIDI mod source, envelope CC out channel + number.

#### Scenario: Not an info alert

- **WHEN** the user clicks MIDI...
- **THEN** an editable settings dialog opens

### Requirement: Shared control strip

Global strip (human labels — not hardware B1–B7 ids): **Randomize all**, **Randomize mod** (all pages), **Marbles**, **Randomize waves** (short labels per `desktop-compact-layout`: Rand all, Randmod all, Rand waves). **XCPL** is adjusted only via the **XCPL knob** on the Audio panel — no strip buttons (B6/B7 semantics duplicate the knob). Mod assignment is **not** on the strip; it lives in the mod rack + patch cables above the panels.

#### Scenario: Per-panel vs global randomize

- **WHEN** the user clicks per-panel **Randomize mod** on Filter
- **THEN** only Filter page mod assignments change; global strip **Randomize mod** still affects all pages

### Requirement: VCO wave on Audio rows

Audio panel rows 0–2: wave glyph button beside `V1VO`/`V2VO`/`V3VO` that cycles morph.

### Requirement: UI refresh respects drags

Periodic refresh SHALL NOT overwrite slider values during active drags.

### Requirement: No SW1/SW2

Desktop SHALL NOT show SW1/SW2 page switches.
