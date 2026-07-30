# Toolchain parity audit (task 1.1)

Compared `FroggersTiga/src/mk/config.mk` + `daisy.mk` against proto Froggers baseline intent (`e0ae431` / dazed-and-con-fielded).

| Flag / setting | Proto baseline | FroggersTiga HEAD | Drift |
|----------------|----------------|-------------------|-------|
| `APP_TYPE` | `BOOT_NONE` | `BOOT_NONE` | None |
| `OPT_LEVEL` | `-Os` | `-Os` | None |
| `USE_LTO` | `1` | `1` | None |
| Toolchain | Arm GNU 14.3.rel1 | Arm GNU 14.3.rel1 | None |
| `GCC_PATH` discovery | fixed path | `bin` + `arm-none-eabi/bin` candidates | Path only; behavior unchanged |

**Conclusion:** No flag regression. `sim/check_firmware_toolchain_parity.sh` guards future drift.
