## MODIFIED Requirements

### Requirement: Field manual documents button responsiveness expectations

`MANUAL.md` SHALL state that SW1/SW2 and B1–B4 are polled on a fast path independent of OLED refresh; that B2/B4 (Rand All / Rand All Mod) complete incrementally (one page per poll); and that lighter buttons are immediate. `MANUAL.md` and `docs/daisy-field-diagnostics.md` SHALL state that intermittent randomize freezes under load are addressed as CPU / poll headroom (block-rate params, LED throttle, dry-reverb early-out), not by removing the reverb page for memory.

#### Scenario: Troubleshooting SW1/SW2

- **WHEN** reader opens SW1/SW2 troubleshooting in `MANUAL.md`
- **THEN** text references control-loop responsiveness (not bootloader) and distinguishes dead switches (no LED) from slow OLED under load

#### Scenario: Randomize freeze cause documented

- **WHEN** reader opens Field troubleshooting or diagnostics covering randomize slowness
- **THEN** text attributes the issue to CPU/poll headroom and states the reverb page is retained
