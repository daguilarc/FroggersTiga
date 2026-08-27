# Deferred out of `frogg3rs-web-release-repair` — form control width

This requirement was written into this change's `froggers-sheaf-runtime-app`
delta and is NOT satisfied, so it does not ship with it. It is kept here whole
so the next change starts from the requirement rather than rewriting it.

Why it did not land: a captioned control has no way to declare its own width.
`Builder::FinishControl` (`PortableUIBuilders.hpp:438-480`) applies the author's
`style.layout` to the `.row` WRAPPER, where `.main` is the row's HEIGHT in the
vertical form column, and gives the control node a fresh `LayoutOptions` with
`.main = Extent::Weight(1.0f)` hardcoded (`:465-467`), where `.main` is its
WIDTH inside the horizontal row. One field name, two axes. So `FormButton` and
`Field` are indistinguishable by the time `ApplyFormGrid` (`:845`) runs, and no
edit confined to the grid can separate them. `BackButton` (`:435-439`) declares
a width with `.cross`, but on a captioned control that sizes the whole row.

Giving a captioned control a way to declare its width is a new layout-model
capability, not a button fix, and it belongs in a change that decides it
deliberately.

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

