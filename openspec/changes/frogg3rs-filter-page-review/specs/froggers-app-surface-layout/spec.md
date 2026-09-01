# Delta — `froggers-app-surface-layout`

## ADDED Requirements

### Requirement: The Filter page groups its controls by stage

The Filter page SHALL present its controls grouped by the stage they
belong to, whatever slot indices carry them: the Peak stage's frequency,
gain, and Q together; the Comb stage's offset, delay, feedback, lowpass,
and drive together; the Scoop stage's blend, frequency, width, and depth
together; and the two routing controls — Comb/Peak and Topology — at the
end of the page's own controls, before the bank's Crispy and the global
Crunchy, which keep their fixed positions. Whether grouping is achieved by
re-slotting or by a display-order mapping is the implementation's decision,
made only after the patch persistence format's addressing is read; saved
patches SHALL keep meaning what they meant.

#### Scenario: Stages read as clusters

- **WHEN** the operator opens the Filter page
- **THEN** every Peak control is adjacent to the other Peak controls, every
  Comb control adjacent to the other Comb controls, and every Scoop
  control adjacent to the other Scoop controls
- **THEN** Comb/Peak and Topology sit at the end of the page's own
  controls, with Crispy and Crunchy unmoved after them

#### Scenario: Saved patches survive the regrouping

- **WHEN** a patch saved before the regrouping is loaded after it
- **THEN** every parameter carries the value it was saved with, applied to
  the control it was saved from
