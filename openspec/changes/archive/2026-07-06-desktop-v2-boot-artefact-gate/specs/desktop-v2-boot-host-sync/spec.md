## MODIFIED Requirements

### Requirement: Boot smoke verification

The desktop-v2 test target SHALL include an automated boot smoke check that launches the **current Release** standalone binary (when built), waits up to 3 seconds, and **fails** if the process exits early. The test SHALL use `waitpid` (or equivalent) to detect termination; it SHALL NOT treat a zombie process as alive. The test SHALL resolve the binary from `FROGGERS_TIGA_V2_BIN` or the canonical Release path under `FroggersTigaDesktopV2_artefacts/Release/`. If no binary exists, the test SHALL skip with an explicit message.

#### Scenario: CI or local ctest boot gate

- **WHEN** `ctest` runs desktop-v2 BootSmoke on a machine where `Release/FroggersTigaV2` is built
- **THEN** the test passes only if the child process is still running after the launch delay

#### Scenario: Early exit fails boot smoke

- **WHEN** the launched binary crashes during construction (e.g. SIGBUS)
- **THEN** BootSmoke reports FAIL with exit/signal status, not PASS

#### Scenario: Binary absent skips explicitly

- **WHEN** no built `FroggersTigaV2` binary is found
- **THEN** BootSmoke prints SKIP and does not fail the suite
