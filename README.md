# Frogg3rs

This is a synthesizer with three oscillators, an envelope per oscillator, a
filter, a drive stage, a delay and a reverb. It runs as a desktop app, as a VST3
or Audio Unit plugin, and in a browser. Every parameter is a knob on one of six
banks of sixteen.

Two things here work differently from other modular synthesizers.

**Modulation goes two levels deep.** Any parameter can be modulated by any of
fifteen sources. Six of those sources are random generators, and the rest are
taken from the instrument itself: each oscillator's raw audio-rate output, each
oscillator's envelope follower, a noise source, and external audio with its own
envelope follower. So one oscillator can drive any parameter in the instrument
at audio rate. Each modulation depth is itself a knob, so it can be modulated by
those same fifteen sources — the amount by which one thing moves another is a
thing you can move. Most parameter ranges are exponential, so a source pushes a
parameter further at one end of its travel than the other.

**Randomization is weighted to stay playable.** Randomize rolls new values and
new modulation depths across a bank or the whole instrument. The number of
sources a parameter picks up is a weighted draw: two is the most common result,
one parameter in five comes out with no modulation at all, and four or more is
rare. A randomized patch comes out with some things moving and some holding
still.

There is also a bit-scramble called fuego, on two knobs named Crispy and
Crunchy, that corrupts parameter values on their way to the DSP. At zero it does
nothing. Turned up, knob moves stop being smooth and values snap between
islands.

Full parameter reference: [`MANUAL.md`](MANUAL.md).

**Current development is the Sheaf-based app under [`app/`](app/README.md)** — see
[Frogg3rs — Sheaf app](#frogg3rs--sheaf-app-current-development) below. The original **Daisy Field**
hardware firmware under `src/` is **frozen** — kept in the tree and buildable, not under active
development — while that work continues. The pre-Sheaf desktop and browser simulators are gone:
`desktop/`, `desktop-v2/`, `sim/`, `wasm/`, and `web/` were deleted once the Sheaf app superseded
them, and `vcv/` was never populated with tracked files.

- **Sheaf app docs:** [`app/README.md`](app/README.md) — build instructions, Sheaf submodule pin, status
- **Manual:** [`MANUAL.md`](MANUAL.md) — global controls and all six parameter banks for the current
  Sheaf app
- **Daisy Field manual:** [`DAISY_MANUAL.md`](DAISY_MANUAL.md) — the frozen Eurorack firmware (pages,
  buttons, modulation workflow, safe flash sequence)
- **Quick Dict:** [`QUICK_DICT.md`](QUICK_DICT.md) — terse, slot-ordered parameter glossary for the
  current Sheaf app
- **License:** MIT — see [`LICENSE`](LICENSE) (copyright JoYoFresh and Diego Aguilar-Canabal)

## Frogg3rs — Sheaf app (current development)

The active line of development: Froggers' DSP ported onto [Sheaf](https://github.com/jvictor0/Sheaf)
(`External/Sheaf`, a pinned submodule — see [`app/README.md`](app/README.md) for the exact commit and
why). This is a from-scratch app on Sheaf's parameter/UI framework, not a continuation of the Daisy
Field firmware or the `desktop-v2` bridge spike (`desktop-v2` was cleared of that spike and frozen
once this app superseded it).

Build and test from the repo root:

```sh
./app/build-launcher.sh
```

```sh
cd app && make test
```

`make test` has no `-k` and stops at the first failing binary of ten — check that all ten ran before
reading "green" into a partial result.

Planning and design history for this app live under [`openspec/`](openspec/) (change proposals,
specs, design docs, handoffs) — see [Local Planning And Hygiene](#local-planning-and-hygiene) below.

Parameter reference for this app: [`MANUAL.md`](MANUAL.md) (global controls, then all six banks —
Audio, Envelope, Filter, Drive, Delay, Reverb — parameter by parameter) and [`QUICK_DICT.md`](QUICK_DICT.md)
(the same six banks, one line per parameter).

## Daisy Field firmware (frozen)

The original target: Eurorack-format Daisy Field hardware. Not under active development — see the
note at the top of this file. Kept in the tree and buildable; the sections below describe that build
as it exists today, unchanged by the Sheaf app work.

This repository ships **self-contained** Daisy tooling (`External/libDaisy`, DaisySP optional): synth sources and boot workflow live **in-tree**—you do not need `~/DaisyExamples`.

### Vendored Dependencies

- `External/libDaisy`
  - pinned to `v8.1.0`
  - includes the Daisy bootloader binaries in `External/libDaisy/core/`
- `External/DaisySP`
  - pinned to `V1.0.0`
  - optional, only used when `USE_DAISYSP=1`
- `External/DaisySP/DaisySP-LGPL`
  - included with DaisySP for optional LGPL modules

### Host Requirements

The repo is self-contained for Daisy sources, linker scripts, libraries, and bootloader binaries, but it still expects a few host tools to be installed:

- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- `arm-none-eabi-ar`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`
- `dfu-util`
- `make`

The build requires **Arm GNU Toolchain 14.3.rel1** installed under:

`/Applications/ArmGNUToolchain/14.3.rel1`

(Apple Silicon: `arm-gnu-toolchain-14.3.rel1-darwin-arm64-arm-none-eabi.tar.xz` from [Arm GNU downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Extract and `sudo mv` the folder to that path.)

`make` looks for `arm-none-eabi-g++` in `14.3.rel1/bin` or `14.3.rel1/arm-none-eabi/bin`. It does not use Homebrew or other versions from `PATH`.

### Repo Layout

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

### Build Flow

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

### Common Commands

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

Same flow as [dazed-and-con-fielded](https://github.com/jvictor0/dazed-and-con-fielded), but build **`src/FroggersTiga`** (not `src/Froggers`).

From repo root (first time):

```sh
make vendor-libs
```

From [`src/FroggersTiga`](src/FroggersTiga):

```sh
make clean
make
```

Enter **DFU mode** (hold BOOT, press RESET, release BOOT; `dfu-util -l` must show `0483:df11`), then:

```sh
make program-dfu
```

`program-dfu` writes to `0x08000000` with `:leave` (auto-reset into firmware). Full procedure is in [`DAISY_MANUAL.md`](DAISY_MANUAL.md).

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

### Bootloader Update

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

### App Types

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

### Optional LGPL Modules

If an app needs DaisySP LGPL modules, build it with:

```sh
make USE_DAISYSP_LGPL=1
```

That adds:

- `External/DaisySP/Source` to include paths
- `libdaisysp.a` to the link step
- `External/DaisySP/DaisySP-LGPL/Source` to include paths
- `libdaisysp-lgpl.a` to the link step

### Verified State

The vendored Daisy SDK build path was verified by:

- building the vendored libraries with `make vendor-libs`
- building `src/Blink` with the repo-default `BOOT_NONE` configuration
- building **`src/FroggersTiga`** (`make` produces `build/FroggersTiga.{elf,bin}`)

With the repo-wide `BOOT_NONE` policy, larger programs may not fit in internal flash. **`FroggersTiga`** has previously approached internal-flash limits and may require size-conscious changes if flash usage grows.

`src/TestControl` still has existing compile errors in project code and external dependencies that are unrelated to the vendoring change.

## Local Planning And Hygiene

OpenSpec artifacts under `openspec/` are planning state for this workspace: change proposals, specs,
design docs, and handoffs. They are git-tracked (as of the commit that removed the prior
`.git/info/exclude` entry) but OpenSpec helpers should still not perform git operations themselves —
committing planning-doc edits is handled the same way as any other change, by the primary agent when
explicitly requested.

Subagents are not allowed to run git commands. Any git inspection, staging, committing, branching, worktree setup, or pushing is handled only by the primary agent when explicitly requested.

Shared DSP/control logic belongs in `src/core/` first. `src/common/` is a firmware compatibility layer; headers mirroring `src/core/<name>.hpp` stay as thin include wrappers, while firmware adapter files such as `App.hpp`, `DaisyIO.hpp`, and `Include.hpp` remain firmware-side exceptions.

Existing build outputs under `src/FroggersTiga/build`, `src/TestControl/build`, and `src/Blink/build` are firmware-scoped leftovers, not host cleanup targets. A separate firmware cleanup should decide whether to keep, remove, or ignore them.
