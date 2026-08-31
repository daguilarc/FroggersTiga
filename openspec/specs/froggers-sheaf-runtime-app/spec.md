# froggers-sheaf-runtime-app Specification

## Purpose
Froggers satisfies `synth::SynthApplication`; it runs under Sheaf Runtime via `sheaf-patch` launcher registration and under the browser host via the browser app entry macro, with a JUCE-free app core and the Daisy firmware under `src/` left untouched.
## Requirements
### Requirement: Froggers is a Sheaf SynthApplication
The Froggers app SHALL be a type satisfying `synth::SynthApplication` — providing `Config()`, `Init(context)`, `ProcessBlock(block)`, and `PortableSurface()` — and SHALL assert that conformance at compile time. The app core SHALL NOT depend on JUCE.

#### Scenario: Concept conformance is enforced at build time
- **WHEN** the app target is compiled
- **THEN** a static assertion confirms the app type satisfies `synth::SynthApplication`
- **THEN** the app core translation units include no JUCE header

#### Scenario: Headless process produces audio
- **WHEN** a test harness calls `Init` then `ProcessBlock` without any host shell
- **THEN** the block is filled with finite stereo samples

### Requirement: Desktop hosting through the Sheaf launcher
Froggers SHALL be launchable on desktop by registering with the `sheaf-patch` launcher, reaching `Runtime<App>` through the launcher's generic registration path. Froggers SHALL NOT define its own JUCE application entry point.

#### Scenario: App appears in the launcher and runs
- **WHEN** the operator selects Froggers in the `sheaf-patch` launcher
- **THEN** a Runtime session is created for the Froggers app
- **THEN** audio processes and the portable surface renders

### Requirement: Browser hosting through the Sheaf browser ABI
The same app type SHALL be hostable in the Sheaf browser/patcher host via the browser app entry macro, with no app-core changes between the desktop and browser hosts.

#### Scenario: One app core serves both hosts
- **WHEN** the browser build is produced from the same app type as the desktop build
- **THEN** no host-specific branching exists in the app core
- **THEN** both hosts drive the identical `ProcessBlock` and `PortableSurface`

### Requirement: Operator documentation ships with the app
THE app SHALL carry its manual and quick dictionary locally in every host
it ships in — standalone, VST3 and AU — and SHALL let the operator open
both from inside the app without a network connection. The documents SHALL
be embedded from the repository's single copy at build time, so that no
second checked-in copy exists to drift from the first. The browser build
MAY instead link to the published documents, because it is already running
in a browser with the network available.

Neither the way the operator reaches the documents nor the way the app
locates them SHALL assume a single platform's conventions. Where a host's
platform has no equivalent of the macOS main menu, the app SHALL present the
same entries by that platform's own means; where it has no application
bundle, the app SHALL find the documents where that platform's build places
them.

#### Scenario: Reading the manual offline in a DAW
- **WHEN** the plugin is loaded in a DAW on a machine with no network
- **THEN** the operator can open the manual and the quick dictionary from
  the plugin itself
- **AND** the content matches the repository's copy for that build

#### Scenario: The standalone app carries its own documentation
- **WHEN** the standalone app is opened with no network
- **THEN** both documents are reachable from inside the app

#### Scenario: Every shipped standalone platform reaches its documents
- **WHEN** the standalone app is opened on any platform it is released for
- **THEN** the manual and quick dictionary open from inside the app
- **AND** the files opened are the ones that build placed, not a path that
  resolves only on the platform the feature was written on

#### Scenario: One copy, not two
- **WHEN** the manual or the quick dictionary is edited in the repository
- **THEN** the next build carries the edit
- **AND** no checked-in duplicate of either document has to be re-synced

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

### Requirement: Form controls are sized to what they contain

A button in a runtime configuration page SHALL be sized to its own label rather
than stretched to the width of the page's control column, and SHALL sit at the
control column's left edge. A two-word button rendered several hundred pixels
wide reads as a mistake, and invites the reader to look for the rest of it.

This SHALL hold on every host. The defect is in the shared form-grid layout, not
in one backend: a combo box hides it because the browser draws a `select` at its
own width regardless of its box, while a button fills whatever box it is given.

Controls that genuinely want the column's width — a text field, a device
selector — SHALL keep it. Filling the column SHALL be a choice a control makes,
and SHALL remain what a control that declares nothing does, so that adding this
capability changes no page that does not ask for it.

#### Scenario: A captioned form button is label-width
- **WHEN** a configuration page renders a captioned button
- **THEN** its width is close to its label's width rather than the column's

#### Scenario: A narrower control still starts where the column starts
- **WHEN** that button is rendered beside full-width device selectors
- **THEN** its left edge is the same as theirs

#### Scenario: The controls that want the column keep it
- **WHEN** the same page renders its device selectors
- **THEN** their cells still span the control column as before

### Requirement: A control's declared width survives to the control

A control style that declares a width SHALL have that declaration reach the
control node, not only the caption row that wraps it.

Where a style carries one layout declaration, the layout SHALL NOT silently
spend it on the wrapper and substitute a default for the control. A control that
declares nothing SHALL be laid out exactly as before.

The form grid SHALL still guarantee that a row's cells fit the row. Sizing a
control to its content SHALL NOT be able to make a row overflow, and a control
whose content is wider than the column available to it SHALL be held to that
column rather than escaping it.

#### Scenario: An undeclared control is unchanged
- **WHEN** a control style declares no width for the control itself
- **THEN** the control fills the control column exactly as it did before

#### Scenario: A declaration is not consumed by the wrapper
- **WHEN** a captioned control declares a content-sized width
- **THEN** the control node is content-sized and the row's own height is
  unaffected

#### Scenario: A content-sized control cannot overflow its row
- **WHEN** a control's content is wider than the column it is given
- **THEN** it is held to the column and the row still holds its children

### Requirement: Like-type controls share column positions

A form grid SHALL align its participating rows to shared column positions:
every row SHALL put its n-th cell at the same x offset, so the page reads down
a straight edge.

This capability had no written rule for that; it lived only in a test
criterion. It is stated here as ADDED rather than MODIFIED because there was
nothing in this spec to modify — which is why the rule could be changed twice
without anyone reading it first.

Cells in a column SHALL NOT be required to share a width. Control width is a
per-control declaration: a control that declares nothing fills the column, and
one that declares a content width is narrower and left-aligned within it. A
column of mixed widths sharing a left edge is the intended rendering, not a
violation.

#### Scenario: A column holds its left edge
- **WHEN** a form grid renders rows whose controls declare different widths
- **THEN** every control cell in the column reports the same x offset

#### Scenario: Differing widths in a column are not a violation
- **WHEN** one row's control is content-sized and another's fills the column
- **THEN** the alignment check reports no violation

### Requirement: A rendered control is routable

Every action a runtime page can emit SHALL be routed by the host that renders
that page. A control that renders and dispatches into nothing SHALL be caught by
a check rather than by an operator finding it inert.

Where a page's actions form a fixed set, that set SHALL have one definition,
read by both the page that emits from it and the host that routes from it. Two
lists expected to agree are the defect: adding a control to one of them is not
required to touch the other, and the button ships live and dead at once.

A page whose routing rule is not a fixed set — one that also admits actions by
prefix — SHALL share the fixed half and keep the prefix rule, which membership
cannot express.

#### Scenario: A page cannot emit an unroutable action
- **WHEN** the Audio, File or Sync page's tree is built with every control shown
- **THEN** every action it emits appears in the list its host routes from

#### Scenario: The page and the host read one list
- **WHEN** the host decides whether an action belongs to the Audio, File, Sync
  or sidebar surface
- **THEN** it reads that surface's own action list rather than restating it

#### Scenario: Removing an action from the shared list is caught
- **WHEN** an action a page emits is removed from that page's action list
- **THEN** a check fails naming the page that emits it

