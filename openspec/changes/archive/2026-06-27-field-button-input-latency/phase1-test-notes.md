# Phase 1 test notes (task 0.7)

## Binary

- Path: `src/FroggersTiga/build/FroggersTiga.bin`
- Flash: 87,928 B (67.08% of 128 KB)
- MD5: `9e739163341728ec095a41f98cd65e22`
- Flashed: 2026-06-26 via DFU to `0x08000000`

## OLED (diag mode)

- `SW1 r? p? n? s?` — RawState, Pressed, press count, suppression (1=active)
- `SW2 r? p? n? s?`
- `cfg def|swap|norm|swpi` — pin audit result
- `page N name` — current page

## Record at rest (nothing pressed)

| Switch | r | p | n | s |
|--------|---|---|---|---|
| SW1    | 1 | 1 | 1 | 1 |
| SW2    | 0 | 0 | 0 | 0 |

cfg label: **def** (inferred — SW2 healthy on default D29/D30 mapping; audit keeps first config scoring SW2 released)

Boot page: **page 0 V1VO** (no runaway page flips — suppression worked)

## Record on tap

- SW1 changes page when s=0? **N** — never responds to deliberate press
- SW2 changes page when s=0? **Y** — `n` increments each press (healthy)
- Suppression blocked spurious page flips when s=1? **Y** — stayed on page 0 at boot despite SW1 stuck

## Interpretation

- **SW1 `r=1` at rest** → GPIO on `seed::D30` reads as pressed *before debounce*. Pin audit tried def/swap/norm/swpi; none cleared SW1 raw. This is **not** a poll-latency or debounce-only bug.
- **SW1 `s=1`** → boot stuck detection fired correctly; page edges suppressed.
- **SW1 never responds** → expected with `s=1` + `r=1` stuck: no `FallingEdge()` to clear suppression, no usable `RisingEdge()` for page nav.
- **SW2 healthy** → `D29` + default inverted pull-up is correct for this unit; libDaisy pin mapping for SW2 is fine.
- **`n=1` on SW1 at boot** → likely one false count during debounce settle at init, not a user press.

## Phase 1 → Phase 2 decision

- [ ] `r=1` at rest, cfg fixed it → re-test, then Phase 2
- [x] **`r=1` at rest, cfg did not fix → hardware repair on SW1/D30 line; Phase 2 optional (latency only, will not fix SW1)**
- [ ] Healthy at rest → Phase 2 latency work

**Decision:** Hardware fault on SW1 (`D30`) or the Field switch circuit for that button. Firmware pin audit exhausted software options. Phase 2 (OLED throttle / mutation queue) proceeds only if intermittent slowness on **SW2/B-keys** still matters after hardware check.
