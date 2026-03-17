# Dazed And Con Fielded

This repo now vendors the Daisy SDK and bootloader assets so the Daisy-specific build inputs live in-tree.

## Vendored Dependencies

- `External/libDaisy`
  - pinned to `v8.1.0`
  - includes the Daisy bootloader binaries in `External/libDaisy/core/`
- `External/DaisySP`
  - pinned to `V1.0.0`
  - optional, only used when `USE_DAISYSP=1`
- `External/DaisySP/DaisySP-LGPL`
  - included with DaisySP for optional LGPL modules

The build no longer depends on `~/Desktop/DaisyExamples`.

## Host Requirements

The repo is self-contained for Daisy sources, linker scripts, libraries, and bootloader binaries, but it still expects a few host tools to be installed:

- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- `arm-none-eabi-ar`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`
- `dfu-util`
- `make`

The build requires the Arm 14.3 toolchain at:

`/Applications/ArmGNUToolchain/14.3.rel1/arm-none-eabi/bin`

If that toolchain is missing, the makefiles now fail immediately with an error instead of falling back to a different `arm-none-eabi-*` toolchain from `PATH`.

## Repo Layout

- `Makefile`
  - root convenience targets for building vendored libraries and flashing the bootloader
- `src/mk/config.mk`
  - shared toolchain and Daisy path configuration
- `src/mk/daisy.mk`
  - shared app build rules
- `src/<AppName>/Makefile`
  - per-app wrapper makefiles

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

With the repo-wide `BOOT_NONE` policy, larger programs may not fit in internal flash. In particular, `Froggers` previously exceeded the 128 KB internal flash region and would need to be reduced in size to build in this mode.

`src/TestControl` still has existing compile errors in project code and external dependencies that are unrelated to the vendoring change.
