# Delta — `froggers-sheaf-runtime-app`

Carried from `frogg3rs-web-release-repair`, which wrote this requirement and did
not satisfy it.

**Operator decision, 2026-08-27.** An earlier pass reverted this work on the
grounds that `criteria::ColumnAlignment`
(`tests/support/VisualCriteria.hpp:430`) asserts every control in a form-grid
column shares a width, and that a passing assertion should not be weakened to
let a change through. That was the wrong call. `ColumnAlignment` is not a
requirement anyone stated; it is an invariant a previous session inferred and
froze into a test. The operator's stated want is the current decision, and a
test encoding the opposite is the thing that moves.

What the criterion actually checks is two separate things per column: a shared
left edge (`x`) and an equal width. Only the second is in dispute. The shared
left edge IS the alignment — a narrower control pinned to the same `x` reads as
left-aligned within its column, which is what was asked for. So the shared-`x`
invariant stays and keeps being asserted; the equal-width assumption is retired
because control width becomes a per-control declaration.

Why the layout model has to change for this at all: `Builder::FinishControl`
(`PortableUIBuilders.hpp:438-480`) applies the author's `style.layout` to the
`.row` WRAPPER, where `.main` is the row's HEIGHT in the vertical form column,
then hands the control node a fresh `LayoutOptions` with
`.main = Extent::Weight(1.0f)` hardcoded (`:465-467`), where `.main` is its
WIDTH along the row. One field name, two axes, and the author's declaration
never reaches the control. `ApplyFormGrid` (`PortableUILayout.hpp:813-857`)
then sets the control cell's width from the row's leftover width at `:845`
unconditionally. Both erase the declaration, so both must change.

## ADDED Requirements

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
