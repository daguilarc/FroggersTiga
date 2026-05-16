# FroggersTiga

Firmware for **Daisy Field**: three loosely coupled oscillators with phase modulation and cross-coupling, polynomial drive / digital reshaping, comb filter and resonant bump, algorithmic reverb, CV modulation routing, and Marbles-style random modulation sources.

This repository ships **self-contained** Daisy tooling (`External/libDaisy`, DaisySP optional): synth sources and boot workflow live **in-tree**—you do not need `~/DaisyExamples`.

- **Operator docs:** [`MANUAL.md`](MANUAL.md) — pages, buttons, modulation workflow, safe flash sequence  
- **License:** MIT — see [`LICENSE`](LICENSE) (copyright JoYo and Diego Manuel)

## Vendored Dependencies

- `External/libDaisy`
  - pinned to `v8.1.0`
  - includes the Daisy bootloader binaries in `External/libDaisy/core/`
- `External/DaisySP`
  - pinned to `V1.0.0`
  - optional, only used when `USE_DAISYSP=1`
- `External/DaisySP/DaisySP-LGPL`
  - included with DaisySP for optional LGPL modules

## Host Requirements

The repo is self-contained for Daisy sources, linker scripts, libraries, and bootloader binaries, but it still expects a few host tools to be installed:

- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- `arm-none-eabi-ar`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`
- `dfu-util`
- `make`

The toolchain prefix is resolved in [`src/mk/config.mk`](src/mk/config.mk): by default it looks under **`/Applications/ArmGNUToolchain/14.3.rel1/arm-none-eabi/bin`**, then **`…/15.2.rel1/…`**, then `arm-none-eabi-*` on `PATH`. If none match, `make` errors immediately instead of failing obscurely later.

## Repo Layout

- `Makefile`
  - root convenience targets for building vendored libraries and flashing the bootloader
- `src/mk/config.mk`
  - shared toolchain and Daisy path configuration
- `src/mk/daisy.mk`
  - shared app build rules
- `src/<AppName>/Makefile`
  - per-app wrapper makefiles (primary synth firmware lives under **`src/FroggersTiga/`**)
- `src/FroggersTiga/`
  - **`FroggersTiga` firmware:** [`Makefile`](src/FroggersTiga/Makefile), [`FroggersTiga.hpp`](src/FroggersTiga/FroggersTiga.hpp), [`FroggersTiga.cpp`](src/FroggersTiga/FroggersTiga.cpp)

## Build Flow

Each app makefile sets a target name and source file, then includes `src/mk/daisy.mk`.

`src/mk/daisy.mk`:

- compiles the app source into `build/*.o`
- links against vendored `libdaisy.a`
- only links `libdaisysp.a` when `USE_DAISYSP=1` or `USE_DAISYSP_LGPL=1`
- selects the linker script based on `APP_TYPE`
- produces `build/<target>.elf` and `build/<target>.bin`
- can flash the resulting binary with `dfu-util`

The shared Daisy library artifacts are built in:

- `External/libDaisy/build/libdaisy.a`
- `External/DaisySP/build/libdaisysp.a`
  - only needed when `USE_DAISYSP=1` or `USE_DAISYSP_LGPL=1`
- `External/DaisySP/DaisySP-LGPL/build/libdaisysp-lgpl.a`

## Common Commands

Build the vendored libraries:

```sh
make vendor-libs
```

Build the vendored libraries including DaisySP:

```sh
make vendor-libs USE_DAISYSP=1
```

Clean the vendored libraries:

```sh
make clean-vendor
```

### FroggersTiga (primary firmware)

From [`src/FroggersTiga`](src/FroggersTiga):

```sh
make clean
make
make program-dfu
```

Full rationale for **`clean → make → program-dfu`** is in [`MANUAL.md`](MANUAL.md).

### Other apps (e.g. minimal Blink template)

Build a normal app that links for internal flash:

```sh
cd src/Blink
make
```

Build an app intended to run under the Daisy bootloader from SRAM:

```sh
cd src/Blink
make APP_TYPE=BOOT_SRAM
```

Build an app intended to run under the Daisy bootloader from QSPI:

```sh
cd src/Blink
make APP_TYPE=BOOT_QSPI
```

Flash a normal app over DFU:

```sh
cd src/Blink
make program-dfu
```

When `APP_TYPE=BOOT_NONE`, DFU writes to internal flash at `0x08000000`.

When `APP_TYPE=BOOT_SRAM` or `APP_TYPE=BOOT_QSPI`, DFU writes to the bootloader-managed QSPI application address at `0x90040000`.

## Bootloader Update

The latest vendored bootloader binary is:

`External/libDaisy/core/dsy_bootloader_v6_4-intdfu-2000ms.bin`

To flash the Daisy bootloader from the repo root:

```sh
make program-boot
```

That runs:

```sh
dfu-util -a 0 -s 0x08000000:leave -D External/libDaisy/core/dsy_bootloader_v6_4-intdfu-2000ms.bin -d ,0483:df11
```

Before running `make program-boot`, put the Daisy into the STM32 ROM DFU mode using the normal BOOT-button procedure.

For a Daisy Field connected over the normal Seed USB port, the vendored `intdfu` bootloader binary is the correct variant.

## App Types

All app builds in this repo now use `APP_TYPE=BOOT_NONE`.

That is a repo policy, not a suggestion. There is one supported app build mode.

The underlying Daisy build system still has these app types:

- `APP_TYPE=BOOT_NONE`
  - normal internal-flash app
  - linker script: `STM32H750IB_flash.lds`
  - DFU target address: `0x08000000`
- `APP_TYPE=BOOT_SRAM`
  - bootloader app loaded from SRAM
  - linker script: `STM32H750IB_sram.lds`
  - defines `BOOT_APP`
  - DFU target address: `0x90040000`
- `APP_TYPE=BOOT_QSPI`
  - bootloader app loaded from QSPI
  - linker script: `STM32H750IB_qspi.lds`
  - defines `BOOT_APP`
  - DFU target address: `0x90040000`

Repo rule:

- all app makefiles build as `BOOT_NONE`
- plain `make` is the supported way to build app targets
- do not use `APP_TYPE=BOOT_SRAM` or `APP_TYPE=BOOT_QSPI` for this repo unless the build system is intentionally redesigned

## Optional LGPL Modules

If an app needs DaisySP LGPL modules, build it with:

```sh
make USE_DAISYSP_LGPL=1
```

That adds:

- `External/DaisySP/Source` to include paths
- `libdaisysp.a` to the link step
- `External/DaisySP/DaisySP-LGPL/Source` to include paths
- `libdaisysp-lgpl.a` to the link step

## Verified State

The vendored Daisy SDK build path was verified by:

- building the vendored libraries with `make vendor-libs`
- building `src/Blink` with the repo-default `BOOT_NONE` configuration
- building **`src/FroggersTiga`** (`make` produces `build/FroggersTiga.{elf,bin}`)

With the repo-wide `BOOT_NONE` policy, larger programs may not fit in internal flash. **`FroggersTiga`** has previously approached internal-flash limits and may require size-conscious changes if flash usage grows.

`src/TestControl` still has existing compile errors in project code and external dependencies that are unrelated to the vendoring change.
