## Why

On Daisy Field hardware, **SW1/SW2** and **B1–B4** (randomize) feel intermittently slow or unresponsive since the **3-VCO FroggersTiga** firmware landed. Proto Froggers (`src/Froggers`, ~81 KB) felt fine; the regression correlates with heavier `FroggersEngine` audio load and an unchanged main loop that couples **one control poll** to a **full OLED redraw** every iteration. This is a firmware UX reliability issue, not a bootloader or sim-host concern.

## Implementation order

**Phase 1 (first — flash and test before latency work):** SW1 stuck-input coda below — diagnostic mode, libDaisy pin/polarity audit, boot-time stuck-switch suppression. Delivers a test firmware you flash once; you report `r`/`p` at rest and whether suppression stops spurious page changes.

**Phase 2 (after Phase 1 findings):** latency work — OLED throttle, mutation queue, toolchain parity. Do not treat Phase 2 as fixing SW1 stuck-at-pressed; proceed with Phase 2 only after Phase 1 test results are recorded (hardware vs pin-config vs debounce).

## What Changes

### Phase 1 — SW1 stuck-input test firmware (first)

- **Input diagnostic mode**: surface `Switch::RawState()` vs debounced `Pressed()` for SW1/SW2 on OLED (`r` / `p`), plus stuck-suppression flag and pin-audit label (formalizes `UpdateScreenDiag`).
- **libDaisy SW polarity/pull/pin audit**: at boot, verify `PIN_SW_1 = seed::D30`, `PIN_SW_2 = seed::D29`, `POLARITY_INVERTED`, `PULLUP` against this board; try swapped D29/D30 if default reads stuck at rest; reconcile libDaisy `// IT LOOKS LIKE THESE MAY NEED TO GET SWAPPED` and issue #534.
- **Boot-time stuck-switch suppression**: sample `RawState()` in `Init`; if a switch reads pressed with nothing held, flag it and suppress page edges until a clean release. Mitigation, not cure.

**Phase 1 ready-to-test:** built `FroggersTiga.bin` flashed via DFU; OLED shows SW1/SW2 `r`, `p`, suppression state; audio runs; normal page/randomize behavior except suppressed stuck SW.

**Phase 1 user test (you):** at rest, note SW1/SW2 `r` and `p`; tap SW1/SW2; confirm whether suppression stops runaway page changes. Record results before Phase 2 starts.

### Phase 2 — Latency (after Phase 1)

- Split **fast input polling** from **slow OLED refresh** in `DaisyIO` main loop.
- Add a **mutation queue** for heavy actions (Rand All, Rand All Mod) mirroring `DesktopHostIO` — enqueue on button edge, drain incrementally.
- Keep **SW1/SW2** and light actions (page step, Rand Page) on the fast path; page changes mark OLED dirty only.
- Add a **toolchain parity check** against proto Froggers (`e0ae431` baseline): `APP_TYPE=BOOT_NONE`, `-Os`, `USE_LTO=1`, Arm 14.3 — document in change; fix only if drift found (current audit: flags match; path discovery improved, not behavior).
- Update **MANUAL.md** only if verification proves SW behavior differs from current claims.
- **No bootloader change** (`BOOT_NONE` internal flash remains; ~84 KB fits 128 KB).

## Capabilities

### New Capabilities

- `field-button-input-latency`: Daisy Field firmware control-loop architecture — poll rate, mutation queue, OLED throttle, and acceptance criteria for SW1/SW2/B1–B4 responsiveness under audio load.

### Modified Capabilities

- `field-operator-doc-parity`: SW1/SW2 / randomize responsiveness claims in `MANUAL.md` must match verified post-fix behavior.

## Impact

- `src/common/DaisyIO.hpp` — Phase 1: diag screen, boot pin audit, stuck-switch suppression; Phase 2: main loop, `ProcessControls`, queue/drain.
- `External/libDaisy/src/daisy_field.cpp` — Phase 1 only if pin audit proves default D29/D30 or polarity wrong for this unit.
- `src/common/App.hpp` — wire drain tick if needed from audio callback.
- `src/core/DesktopHostIO.hpp` — reuse `HostMutation` / `HostMutationType` subset (no duplication of randomize logic).
- `src/mk/config.mk` — verify/document toolchain parity only.
- `MANUAL.md` — conditional doc fix.
- **Out of scope**: sim/desktop/web hosts, bootloader/QSPI, DSP algorithm changes, knob pickup semantics.

## Coda: SW1 Stuck-Input Investigation

On-device diagnostic shows **SW1 reads pressed at rest**. This is a distinct failure mode from the latency root cause above, recorded here by request rather than as a separate change.

### Established facts (verified)

- On-chip app at `0x08000000` is byte-identical to the known-good build: md5 `37da0d1cb7f89fd837de5aade6817450` (DFU read-back + `cmp`).
- Same physical Field unit, same binary as the prior "working" report.
- The "worked before" observation is unconfirmed and is excluded as evidence pending retest.

### Conclusion

- Identical bytes mean identical SW1 handling. A different app implementation is not the cause.
- Root cause is hardware/electrical on this unit or a libDaisy SW1/SW2 pin-config assumption — **not** the OLED-throttle, mutation-queue, or poll-cadence work in this proposal. The additions below do not fix latency, and the latency items do not fix a stuck input.

### Phase 1 test procedure (after flash — user action)

1. At rest, read OLED lines for SW1 and SW2: `r` (RawState, pre-debounce) and `p` (Pressed, post-debounce).
   - `r=1 p=1` at rest → pin electrically low or wrong polarity/pull → hardware or libDaisy pin fix.
   - `r=0 p=1` at rest → debounce/state stuck → firmware debounce path.
   - `r=0 p=0` at rest, `r=1 p=1` on press → healthy; stuck symptom was timing or prior binary confusion.
2. Tap SW1/SW2: page should change only on deliberate press; suppression flag should block spurious page flips if switch was stuck at boot.
3. Optional: second-computer md5 of Phase 1 `.bin` (host does not affect runtime).

### Phase 1 → Phase 2 decision

| Phase 1 result | Next step |
|----------------|-----------|
| `r=1` at rest, pin audit fixes it (swap/polarity) | Merge pin fix; re-test; then Phase 2 |
| `r=1` at rest, audit does not fix | Hardware repair/replace; Phase 2 optional for latency only |
| Switches healthy at rest | Phase 2 latency work addresses intermittent slowness |

### Scope note

Phase 1 acceptance: diagnostic OLED + suppression behavior verified on device. Phase 2 acceptance criteria (latency under load) unchanged and apply only after Phase 1 results are recorded.

**Recorded findings:** see `phase1-findings.md` and `phase1-test-notes.md`.
