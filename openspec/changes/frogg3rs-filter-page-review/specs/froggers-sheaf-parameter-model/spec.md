# Delta — `froggers-sheaf-parameter-model`

## MODIFIED Requirements

### Requirement: One sixteen-slot bank per Froggers page
Each existing Froggers page SHALL become exactly one bank of sixteen parameter slots. Pages SHALL NOT be
merged. A bank's own parameters SHALL occupy the leading slots; remaining parameter slots MAY be empty,
or MAY hold additional named parameters where a bank's slate has been explicitly decided and expanded.

#### Scenario: The Filter bank holds fourteen parameters, grouped by stage

- **WHEN** the Filter bank is enumerated
- **THEN** it holds fourteen named parameters at slot indices 0 through 13, grouped by the stage they
  belong to, with saved patches unaffected because persistence addresses parameters by name
- **THEN** slots 0-2 are the Peak stage: Peak Freq (`PkFreq`), Peak Gain (`PkGain`), Peak Q (`PkQ`)
- **THEN** slots 3-7 are the Comb stage: Comb Offset (`CmbOff`), Comb Delay (`CmbDly`), Comb Feedback
  (`CmbFb`), Comb LP (`CmbLP`), and Comb Drive (`CDrv`) — a saturation-depth control whose drive factor
  multiplies the in-loop saturator's input while the same factor divides its output, so the loop's
  small-signal gain is unity at every setting, the fed-back term never exceeds the delayed signal it is
  computed from, and the loop's decay argument is preserved at every drive
- **THEN** slots 8-11 are the Scoop stage: Scoop Mix (`ScMix`, the notch's wet/dry blend into the chain's
  shared input, ahead of both the Comb and Peak stages), Scoop Freq (`ScFq`, the notch's own center
  frequency, independent of the Peak stage's), Scoop Width (`ScWd`, the notch's own width, independent
  of the Peak stage's), Scoop Depth (`ScDp`, the notch's own dip depth, independent of how much of it
  reaches the chain's input)
- **THEN** slots 12-13 are the routing pair: Comb/Peak (`Cmb/Pk`) and Topology (`Topo`) — Topology a
  continuous morph of the Comb and Peak stages from parallel at one end to series at the other, with no
  switched positions anywhere in its travel; at its minimum the Peak stage reads the chain's scooped
  input, the same input the Comb stage reads, and at its maximum it reads the Comb stage's output
  instead, with the Comb/Peak blend and every output trim and limiter staying in force at every
  position of both controls
- **THEN** the bank's Crispy and the global Crunchy keep slots 14 and 15
