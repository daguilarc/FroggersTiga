## ADDED Requirements

### Requirement: Sim mod source set (no external CV)

Sim hosts (desktop and web) SHALL expose only modulation sources that exist without Field CV jacks:

| UI label | Core `m_modIndex` | Available on |
|----------|-------------------|--------------|
| MIDI | `0` | Desktop (and VCV phase 2) |
| VCO feat | `4` | All sim hosts |
| Marbles 1 | `5` | All sim hosts |
| Marbles 2 | `6` | All sim hosts |

Sim hosts SHALL NOT present M1–M4 as four separate CV sources. `m_mods[1..3]` remain unused in sim assignment UI. Firmware and VCV Field-parity CV jacks are unchanged.

#### Scenario: Web mod dropdown options

- **WHEN** the web sim renders mod assignment on a knob row
- **THEN** options are `None`, `VCO feat`, `Marbles 1`, `Marbles 2` — not M1–M7

#### Scenario: Desktop MIDI is one source

- **WHEN** the desktop mod rack is shown
- **THEN** exactly one external module labeled **MIDI** exists, not four CV lanes

### Requirement: One source to many destinations

Assignment SHALL follow Field semantics: a single mod source MAY drive many parameters simultaneously, each with its own depth (`Parameter::m_modIndex` + `m_modAmount` per destination).

#### Scenario: Marbles 1 to multiple knobs

- **WHEN** the user assigns Marbles 1 to Audio V1VO depth 0.6 and Audio FUEG depth 0.3
- **THEN** both parameters use `m_modIndex == 5` with independent `m_modAmount` values

### Requirement: Desktop patch cables (VCV Rack interaction)

Desktop v2.1 SHALL assign modulation via **VCV Rack-style patch cables**, per [VCV Rack Getting Started](https://vcvrack.com/manual/GettingStarted): drag from **output** port to **input** port **or** from an empty gray **input** jack to a mod **output** (bidirectional — see `desktop-host-mutation-safety`); drag a plug to empty space to delete. **Not** two-click arm-then-click. A drag shorter than 4px from any jack SHALL NOT create or change a cable.

Mod sources SHALL render as **module boxes** (label, live meter, **output jack**). Each parameter row on each panel SHALL have a **mod input jack**, including **FUEG** (position 7).

#### Scenario: Drag from output instantiates cable

- **WHEN** the user mousedown-drags from the Marbles 1 **output** jack
- **THEN** a bezier cable appears attached to the cursor before mouseup

#### Scenario: Drop on valid input connects

- **WHEN** the user releases the mouse over a parameter **input** jack while dragging from a mod output
- **THEN** `SetPageModSource(page, position, sourceIndex)` is called and a persistent cable is drawn between those jacks

#### Scenario: Drop on invalid target cancels

- **WHEN** the user releases the mouse over empty space (not an input jack) while dragging a new cable
- **THEN** the cable disappears and no mod assignment is created or changed

#### Scenario: Drag plug to void disconnects

- **WHEN** the user grabs an existing cable plug and drops it on empty space
- **THEN** that route is removed (`SetPageModSource(..., 255)`) and the cable is deleted

#### Scenario: Replace on re-patch (Field semantics)

- **WHEN** parameter row already has Marbles 1 patched and the user drops a VCO feat cable on the same input jack
- **THEN** the assignment changes to VCO feat — inputs do **not** sum multiple mod sources (unlike VCV voltage summing)

#### Scenario: Hover highlights valid input

- **WHEN** the user drags a cable over a valid input jack
- **THEN** that input jack is visually highlighted

#### Scenario: No mod dropdown on desktop panels

- **WHEN** inspecting `SubModulePanel` after v2.1
- **THEN** no per-row `ComboBox` mod selectors exist

#### Scenario: FUEG accepts modulation

- **WHEN** the user drops a VCO feat cable on the FUEG input jack on the Filter panel
- **THEN** `SetPageModSource(filterPage, 7, 4)` is stored and depth adjusts via the FUEG slider

#### Scenario: One output to many inputs

- **WHEN** the user drags from Marbles 1 output to Audio V1VO input, then drags again from the same output to Audio FUEG input
- **THEN** two cables exist from one output and both destinations retain independent depths

#### Scenario: Move existing cable to new input

- **WHEN** the user grabs the plug at a connected input jack and drops it on a different parameter input jack
- **THEN** the route moves to the new destination and the cable redraws — the old destination is cleared (`SetPageModSource(..., 255)` on the former row)

#### Scenario: Drag from empty input instantiates cable

- **WHEN** the user mousedown-drags from a parameter input with `modIndex == 255` (gray ring)
- **THEN** a bezier cable appears after the 4px threshold

#### Scenario: Drop empty-input drag on mod output connects

- **WHEN** the user releases over a mod rack output while dragging from an empty parameter input
- **THEN** that row is assigned to the output mod index and a persistent cable is drawn

#### Scenario: Cables repaint after Randomize mod

- **WHEN** the user clicks per-panel **Randomize mod** or global **Randomize mod** (all pages)
- **THEN** `PatchCableOverlay` redraws to match updated `GetPageModSource` values (removed routes lose cables)

#### Scenario: Sim-invalid mod index rejected

- **WHEN** the host layer receives a patch to mod indices 1, 2, or 3 from desktop or web UI
- **THEN** the assignment is rejected or mapped only through `SimModSource` (indices 0, 4, 5, 6) — Field CV indices 1–3 are not sim-assignable

#### Scenario: Port hit radius

- **WHEN** the user releases the mouse within 14px of an input jack center while dragging a cable
- **THEN** the drop counts as a valid connection target

### Requirement: Web dropdown assignment

Web SHALL use per-knob **dropdown** mod assignment on the current page only. Cables are out of scope for web v2.1. Each knob column SHALL stack: **vertical slider**, then mod `<select>` **directly below** the slider — not beside the parameter name (names stay on the OLED row).

#### Scenario: Dropdown sets core index

- **WHEN** the user selects Marbles 2 on knob row 3's dropdown below the slider
- **THEN** WASM calls `SetRowModSource(3, 6)`

#### Scenario: Mod control not beside name

- **WHEN** inspecting web knob column layout after v2.1
- **THEN** the mod dropdown is under the slider; the knob column label is not sharing a row with the dropdown

### Requirement: Per-panel Randomize mod

Each desktop panel SHALL provide **Randomize** (params, knobs 0–6, skip FUEG) and **Randomize mod** (all 8 positions including FUEG) calling `RandomizePageMod(page)`.

#### Scenario: Filter panel randomize mod

- **WHEN** the user clicks **Randomize mod** on the Filter panel
- **THEN** only Filter page mod assignments are re-randomized

### Requirement: Readable parameter labels (desktop)

Desktop panels SHALL display full 4-character parameter names (`V1VO`, `RVMX`, etc.) without ellipsis. This SHALL be achieved by **removing** per-knob mod dropdowns from panel rows — **not** by widening panels.

#### Scenario: Audio VCO labels visible at current width

- **WHEN** the desktop window is at the current five-column width and mod dropdowns are removed
- **THEN** Audio rows 0–2 show `V1VO`, `V2VO`, `V3VO` in full
