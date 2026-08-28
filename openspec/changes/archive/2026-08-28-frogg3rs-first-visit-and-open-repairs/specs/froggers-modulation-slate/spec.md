# Delta — `froggers-modulation-slate`

The encoder grid's disabled colour and the runtime chrome's disabled colours are
three different values in two modules. Two changes in a row have raised that as
duplication and stopped short of deciding it, because the only statement of the
reasoning lives in a comment at one of the three sites — findable if you already
know it is there, invisible otherwise.

It is not duplication. Enumerated by operand across both repositories: 197
`Rgb(` call sites, 92 distinct triples, 27 appearing more than once, and none of
the repeats is a disabled colour. The three are `Rgb(90, 96, 100)` for a
disabled encoder cell, and `Rgb(125, 132, 138)` / `Rgb(45, 49, 53)` for
disabled text and a disabled button in the configuration pages. Each appears
exactly once. There is no shared value being written three times, so §7 does not
apply and there is nothing to collapse.

What there is, is an undocumented decision. This records it.

## ADDED Requirements

### Requirement: The encoder grid owns its own colour language

The encoder grid SHALL define its own colours, including the colour that marks
a cell unavailable, rather than drawing them from the runtime configuration
pages' palette.

The two are separate visual systems addressed to different readers. A
configuration page is a form, read as text against a panel. The encoder grid is
an instrument surface, read as a field of illuminated cells at a glance. A
colour that reads as correctly de-emphasised in one is not the colour that reads
that way in the other, and a shared constant would make one of them wrong to
serve consistency no viewer experiences.

Where the two SHOULD agree, that is a palette decision taken once for both and
applied deliberately — not an import added at whichever site was being edited
when the mismatch was noticed.

#### Scenario: A disabled cell is not bound to the page palette
- **WHEN** the configuration pages' disabled colours change
- **THEN** the encoder grid's disabled cell colour is unaffected

#### Scenario: Neither palette is the other's source
- **WHEN** either surface's disabled colour is chosen
- **THEN** it is chosen for that surface's own reading conditions
