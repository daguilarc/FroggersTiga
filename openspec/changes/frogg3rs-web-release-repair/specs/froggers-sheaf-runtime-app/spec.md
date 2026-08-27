# Delta — `froggers-sheaf-runtime-app`

Two things the app ships that misreport themselves: a load readout that shows a
held peak as though it were current, and form controls that stretch to the full
width of a page regardless of what they contain. Both are in the runtime chrome
this app ships, so this capability is where the app owes them.

The documentation requirement is here for the same reason: the docs ship with the
app, and they are currently written for whoever was building it rather than
whoever is using it.

## ADDED Requirements

### Requirement: The runtime chrome reports its load honestly

The load readout in the runtime sidebar SHALL hold its peak only briefly enough
that a reader can take it for the current load without being wrong.

It shows the highest sample over a recent window, so that a transient spike stays
visible long enough to be read — worth keeping, because a spike is what produces
an audible click. But the window SHALL be short enough that a peak does not
outlive the condition that caused it: a startup transient still on screen seconds
after the instrument went idle reports an overload that is not happening.

The window SHALL be expressed in time rather than in frames, because the UI tick
rate is per-application configuration and the same frame count is a different
hold on different hosts.

The displayed precision SHALL NOT exceed the precision the figure has. A held
maximum is not accurate to a tenth of a percent.

The readout SHALL render within the sidebar's own width at every value it can
take, including three digits.

#### Scenario: A stale peak does not outlive its cause
- **WHEN** the load spikes and then returns to a lower steady level
- **THEN** the readout returns to that level within the window's own span

#### Scenario: A spike is still visible
- **WHEN** a single sample spikes
- **THEN** the readout shows it rather than averaging it away

#### Scenario: It fits the column it renders in
- **WHEN** the load readout renders at three digits
- **THEN** it renders within the sidebar's width without truncation

#### Scenario: A held peak is not reported to a tenth of a percent
- **WHEN** the load readout renders
- **THEN** it shows whole percent

### Requirement: Form controls are sized to what they contain

A button in a runtime configuration page SHALL be sized to its own label rather
than stretched to the width of the page's control column. A two-word button
rendered several hundred pixels wide reads as a mistake, and invites the reader
to look for the rest of it.

This SHALL hold on every host. The defect is in the shared form-grid layout, not
in one backend: a combo box hides it because the browser draws a `select` at its
own width regardless of its box, while a button fills whatever box it is given.

Controls that genuinely want the column's width — a text field, a device
selector — SHALL keep it. The requirement is that filling the column is a choice
a control makes, not the only thing the grid can do.

#### Scenario: A captioned form button is label-width
- **WHEN** a configuration page renders a captioned button
- **THEN** its width is close to its label's width rather than the column's

#### Scenario: The controls that want the column keep it
- **WHEN** the same page renders its device selectors
- **THEN** their cells still span the control column as before

### Requirement: The shipped documentation addresses someone learning the instrument

`README.md` and `MANUAL.md` SHALL be written for a reader finding out what this
instrument is and how to play it.

Build instructions for a frozen hardware target, toolchain paths, flashing
procedures and repository working process SHALL NOT occupy the README's body.
Where that material is still true it belongs in the document that already covers
that target; where it describes something that no longer ships it goes.

The README SHALL open by saying what the instrument is and what is unusual about
it, and a reader SHALL be able to reach the parameter reference without reading
past material addressed to someone building the project.

#### Scenario: The README leads with the instrument
- **WHEN** a reader opens the README
- **THEN** what the instrument is, and what distinguishes it, comes before
  anything about building it

#### Scenario: Firmware build detail is not in the README body
- **WHEN** the README is read end to end
- **THEN** it contains no toolchain installation paths, DFU addresses, or linker
  script selection, all of which live in the hardware target's own manual

#### Scenario: Repository working process is not operator documentation
- **WHEN** the README is read end to end
- **THEN** it does not document planning artifacts, agent conventions, or where
  shared source belongs
