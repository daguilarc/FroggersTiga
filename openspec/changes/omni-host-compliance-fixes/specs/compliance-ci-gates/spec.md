## ADDED Requirements

### Requirement: Label parity CI gate

GitHub Actions Pages workflow SHALL run `sim/check_param_display_names.sh` and fail the job if labels diverge from `ParamDisplayNames.hpp`.

#### Scenario: CI catches TS drift

- **WHEN** `HOST_PAGE_LABELS` (or generated fallback) diverges from the header
- **THEN** the Pages workflow fails before deploy

### Requirement: Mod source label CI gate

A CI script SHALL verify web mod-bay source titles match `ParamDisplayNames::forModSource` for indices 4, 5, and 6.

#### Scenario: Mod bay string drift detected

- **WHEN** `main.ts` hardcodes a mod source title that differs from the header
- **THEN** the compliance script exits non-zero

### Requirement: Manual doc sync in build

Web build SHALL copy root `SIM_MANUAL.md` and `QUICK_DICT.md` to `web/public/` (and optionally `docs/`) so operator docs cannot drift silently.

#### Scenario: Manual update propagates

- **WHEN** root `SIM_MANUAL.md` changes
- **THEN** `npm run build` copies the updated file into `web/public/sim-manual.md`
