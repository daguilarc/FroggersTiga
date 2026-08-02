# Delta — `froggers-modulation-slate`

Recorded 2026-07-31. §E shipped behaviour that contradicts three requirements in the main spec, and
this delta was missing — the code landed and the spec was never updated. **Where the two disagree,
the shipped behaviour is correct and the spec is stale**; each requirement below is amended to match
what the instrument actually does, with the operator's reasoning recorded so it is not "corrected"
back later.

## MODIFIED Requirements

### Requirement: Modulation drill-in is capped at two levels
The app SHALL permit drilling from a top-level parameter into its modulation depth grid, and from a
depth cell into that depth parameter's own modulation — and SHALL refuse any deeper drill-in. The cap
SHALL be enforced by the app, because the underlying framework provides no depth limit and no level
stack.

**The cap itself is unchanged.** What changes below is only how the operator gets *back out*.

Back from the **second** level SHALL return to the first-level view of the same parameter — a
one-level pop — rather than exiting to the parameter grid. Back from the first level SHALL exit to
the parameter grid as before.

Selecting the **bank that is already active** SHALL exit a modulation drilldown to that bank's
parameter grid, rather than being a no-op. Re-selecting the active bank while already at the
parameter grid SHALL remain a no-op.

*(Amended 2026-07-31. The main spec's Note claimed a one-level pop was "deliberately not provided"
because "the call that would re-open an intermediate level is not part of its public surface, so
synthesizing a pop would mean working around a private API." **That justification was factually
wrong.** The pop is synthesized entirely from public API — deselect, then re-press a remembered
encoder id — with no private surface touched and no framework change. The operator overruled the
original choice on use: dropping two levels at once reads as broken. Note also that the framework
still has no native one-level pop; that remains an open upstream ask.)*

#### Scenario: Second level is permitted
- **WHEN** the operator drills into a modulation depth cell from the detail grid
- **THEN** that depth parameter's own modulation view opens

#### Scenario: Third level is refused
- **WHEN** the operator attempts to drill in from a depth cell at the second level
- **THEN** no further modulation view opens
- **THEN** the current level remains the active editing context

#### Scenario: Back from the second level pops one level
- **WHEN** the operator activates Back from the second level
- **THEN** the first-level modulation view of the same parameter is restored
- **THEN** the parameter it reopens is the one that was open before descending, not merely some parameter

#### Scenario: Back from the first level exits to the parameter grid
- **WHEN** the operator activates Back from the first level
- **THEN** the bank's parameter grid is restored

#### Scenario: Selecting the active bank escapes a drilldown
- **WHEN** the operator selects the bank they are already viewing, while drilled into a modulation level
- **THEN** that bank's top-level parameter grid is restored
- **WHEN** the operator selects the active bank while already at the parameter grid
- **THEN** nothing changes

### Requirement: Two randomize affordances
The app SHALL provide exactly two randomize affordances: **Randomize All** (global) and **Randomize
Page** (per-page).

Randomize All, pressed while a parameter page is active, SHALL randomize every parameter value in
every bank — **excluding both the global Crunchy control and each bank's local Crispy control** —
plus all first-level modulation depths, and SHALL NOT descend to the second level. Randomize All,
pressed while a first-level modulation detail grid is active, SHALL randomize that parameter's depths
and SHALL also materialize and randomize their second-level depths. Randomize All pressed at the
second level SHALL behave identically to Randomize Page.

Randomize Page SHALL always randomize exactly what is displayed: on a parameter page, that bank's
values **including that page's own local Crispy**, with no depths; on a modulation detail grid, that
grid's depths only.

*(Amended 2026-07-31, operator decision. The main spec required Randomize All to include each bank's
Crispy. Randomizing local Crispy on all six banks at once is **effectively randomizing global
Crunchy**, which this app deliberately never randomizes — so the original requirement reached the
excluded outcome by another route. One page's own Crispy remains that page's business, which is why
the two affordances now differ on it.)*

#### Scenario: Global press does not create second-level depth
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** every bank's parameter values and first-level modulation depths are randomized
- **THEN** no second-level modulation depth is materialized

#### Scenario: A first-level press materializes that parameter's second-level depths
- **WHEN** Randomize All is pressed while a first-level modulation detail grid is active
- **THEN** that parameter's depths are randomized
- **THEN** their second-level depths are materialized and randomized
- **THEN** no other parameter's second-level depths are materialized

#### Scenario: The global Crunchy control is never randomized
- **WHEN** Randomize All or Randomize Page is pressed, at any level
- **THEN** the global Crunchy control's value is unchanged

#### Scenario: Randomize All leaves every bank's Crispy alone
- **WHEN** Randomize All is pressed while a parameter page is active
- **THEN** no bank's local Crispy control changes
- **THEN** the reason is that changing all six at once is equivalent to changing global Crunchy

#### Scenario: Randomize Page still randomizes its own page's Crispy
- **WHEN** Randomize Page is pressed while a parameter page is active
- **THEN** that bank's parameter values change, including that bank's local Crispy
- **THEN** no other bank's Crispy changes
- **THEN** no modulation depth, at either level, changes

### Requirement: Randomized source count is biased toward few, and depth storage is allocated once
**The app** SHALL select how many modulation sources a randomize call affects, rather than
inheriting the framework's own count distribution. The selected count SHALL never be zero, SHALL
have a **median of 3**, and SHALL fall off sharply above 4 while remaining able to reach the full
connected-source count.

Counts above 4 and below 2 SHALL be about equally rare. The tail above 4 SHALL decay geometrically
rather than uniformly, so that a large count stays reachable without being common.

The sources chosen SHALL be **distinct** — a single call SHALL NOT randomize the same source twice.
Only sources the framework reports as connected SHALL be eligible.

Depth storage for a given source SHALL be allocated once, on first use, rather than accumulating
additional storage across repeated randomization presses.

*(Amended 2026-07-31. The main spec described the **framework's** distribution — "SHALL affect zero
modulation sources on about half of its calls, and four or more sources only on about one call in
sixteen" — which accurately characterised the framework's geometric-from-zero draw. The app no
longer relies on it: a call that does nothing half the time is invisible inside a bank-wide
randomize but makes a single deliberate press on a modulation page a coin flip, which the operator
judged broken. The framework itself is unchanged and still behaves as originally described; that
sentence simply stopped describing this app. Distinctness also fixes a framework behaviour we chose
not to reproduce — its loop draws independently and can select the same source twice, making the
effective count lower than the nominal one.)*

#### Scenario: A randomize call always does something
- **WHEN** a modulation-depth randomize call is made against a parameter with connected sources
- **THEN** at least one source's depth changes

#### Scenario: The count is centred on three
- **WHEN** many randomize calls are sampled
- **THEN** the median number of sources affected is 3
- **THEN** counts above 4 and counts below 2 occur about equally often

#### Scenario: Sources within one call are distinct
- **WHEN** a randomize call affects more than one source
- **THEN** no source is selected twice within that call

#### Scenario: Repeated presses do not grow allocated depth parameters
- **WHEN** a randomize affordance is pressed repeatedly against the same target
- **THEN** the number of allocated depth parameters for a given source does not increase across presses
- **THEN** only the depth values change
