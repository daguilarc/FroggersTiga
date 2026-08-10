# Delta — `froggers-modulation-slate`

Fixes a regression the main spec's own text still contradicts (it currently reads "a one-level pop
is deliberately not provided" and "full exit from any level is the accepted behavior"). A sibling
live change (`frogg3rs-audio-safety-and-ui-rework`, amended 2026-07-31) already reversed that
decision and put the one-level pop into `FroggersModulationDrillIn::Back()`; this delta does not
re-litigate that decision, only carries it forward as context. What it fixes is narrower and
downstream of it: `Back()` itself was correct, but the operator's *actual* on-screen gesture never
reached it. Every real press — Target/Back included — dispatches through
`FroggersModulationDrillIn::PressEncoder()`, never through `Back()` directly, and `PressEncoder()`
still collapsed straight to level 0 on any press that cleared the current selection. Recorded as
landed in `49ce9af`/`9d0802c`; operator, 2026-08-07: *"the drilldown back button still doesn't go
one back, it goes all the way back."* The pre-existing test coverage for `Back()` called it directly
and so could not have caught this — it never exercised the dispatch path the regression lived in.
Traced and fixed in `PressEncoder()` (`app/FroggersModulation.hpp`).

## MODIFIED Requirements

### Requirement: Modulation drill-in is capped at two levels
The app SHALL permit drilling from a top-level parameter into its modulation depth grid, and from a
depth cell into that depth parameter's own modulation — and SHALL refuse any deeper drill-in. The cap
SHALL be enforced by the app, because the underlying framework provides no depth limit and no level
stack. *(The cap's own numeric depth is unchanged by this delta.)*

Target/Back SHALL pop exactly **one** level per activation, from any depth, landing on the same
parameter view that was open one level shallower — not a full exit to the parameter grid — except
that Back from the first level SHALL exit to the parameter grid, because there is no shallower
modulation view to return to.

This SHALL hold for **every** dispatch path capable of activating Target/Back, not only a direct call
to the underlying pop mechanism: the operator's on-screen press of the Target/Back cell
(`kEncoderPress` on that cell's encoder id, routed through `PressEncoder()`) SHALL resolve to the
same one-level pop as a direct call would. A press is recognized as the Target/Back press by
comparing the pressed cell's visible parameter — read *before* the press is dispatched — against the
parameter already selected; that comparison is the one and only condition under which the
framework's own `HandlePress` clears the current selection, so it is an exact predicate for "this was
the Target/Back cell," not a heuristic.

Selecting the bank that is already active SHALL exit a modulation drilldown to that bank's parameter
grid, rather than being a no-op. Re-selecting the active bank while already at the parameter grid
SHALL remain a no-op.

#### Scenario: Second level is permitted
- **WHEN** the operator drills into a modulation depth cell from the detail grid
- **THEN** that depth parameter's own modulation view opens

#### Scenario: A drill-in past the cap is refused
- **WHEN** the operator attempts to drill in from a depth cell already at the cap
- **THEN** no further modulation view opens
- **THEN** the current level and selection remain unchanged

#### Scenario: The real Target/Back press pops one level at a time, from the deepest reachable level
- **WHEN** the operator has drilled to the deepest reachable level and activates the on-screen
  Target/Back cell by pressing it, through the surface's own action dispatch rather than a direct
  call to the underlying pop mechanism
- **THEN** the level decreases by exactly one
- **THEN** repeating the same press from each resulting level continues to decrease it by exactly
  one, down to level 0, where the bank's parameter grid is showing and no modulation view remains
  open (`FroggersSurfaceTests.cpp`'s
  `pressing_target_back_cell_through_the_surface_pops_exactly_one_drill_level_at_a_time`)

#### Scenario: Back from the first level exits to the parameter grid
- **WHEN** the operator activates Target/Back from the first level
- **THEN** the bank's parameter grid is restored

#### Scenario: A one-level pop restores the same parameter, not merely some parameter at that depth
- **WHEN** the operator descends from a first-level parameter's view into a second-level depth cell,
  then pops back one level
- **THEN** the restored first-level view is the same parameter that was open before the descent,
  by identity, not merely a parameter at that depth

#### Scenario: Selecting the active bank escapes a drilldown
- **WHEN** the operator selects the bank they are already viewing, while drilled into a modulation
  level
- **THEN** that bank's top-level parameter grid is restored
- **WHEN** the operator selects the active bank while already at the parameter grid
- **THEN** nothing changes
