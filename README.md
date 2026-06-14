# FroggersTiga

Firmware for **Daisy Field**: three loosely coupled oscillators with phase modulation and cross-coupling, polynomial drive / digital reshaping, comb filter and resonant bump, algorithmic reverb, CV modulation routing, and Marbles-style random modulation sources.

This repository ships **self-contained** Daisy tooling (`External/libDaisy`, DaisySP optional): synth sources and boot workflow live **in-tree**—you do not need `~/DaisyExamples`.

- **Operator docs:** [`MANUAL.md`](MANUAL.md) — pages, buttons, modulation workflow, safe flash sequence  
- **Quick Dict:** [`QUICK_DICT.md`](QUICK_DICT.md) — abbreviated parameter glossary  
- **License:** MIT — see [`LICENSE`](LICENSE) (copyright JoYoFresh and Diego Aguilar-Canabal)

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

The build requires **Arm GNU Toolchain 14.3.rel1** installed under:

`/Applications/ArmGNUToolchain/14.3.rel1`

(Apple Silicon: `arm-gnu-toolchain-14.3.rel1-darwin-arm64-arm-none-eabi.tar.xz` from [Arm GNU downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Extract and `sudo mv` the folder to that path.)

`make` looks for `arm-none-eabi-g++` in `14.3.rel1/bin` or `14.3.rel1/arm-none-eabi/bin`. It does not use Homebrew or other versions from `PATH`.

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

`program-dfu` writes to `0x08000000` with `:leave` (auto-reset into firmware). Full procedure is in [`MANUAL.md`](MANUAL.md).

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

## Browser simulator (GitHub Pages)

**Release v1.0.3** — static WASM + Web Audio sim at repo `docs/` (published from `main`).

**Local dev (first clone):**

```sh
cd web
npm install
npm run build:all       # build WASM + sync docs + web bundle
npm run dev
```

If WASM is already built:

```sh
cd web
npm install
npm run dev             # predev checks web/public/froggers.wasm exists
```

Rebuild WASM after core changes (requires [Emscripten](https://emscripten.org/)):

```sh
cd web && npm run build:wasm    # verifies exports after copy
```

- Paged Field-style UI (8 knobs, SW1/SW2, OLED mock); mod dropdown **below** each slider (`None | VCO feat | Random 1 | Random 2`)
- Default **44.1 kHz** (`audioContext.sampleRate`)
- **Mic** toggle default **off** (VCO-only until enabled)

**Help docs:** `SIM_MANUAL.md` (sim operators — embedded in desktop/web Help → Manual) and `MANUAL.md` (Daisy Field firmware — repository only, not shipped to sim hosts). `QUICK_DICT.md` is a short parameter glossary.

**Host page labels:** `sim/ParamDisplayNames.hpp` is the single authority. Web UI reads row names from wasm (`rows[].name`); run `sim/check_param_display_names.sh` and `sim/check_mod_source_labels.sh` after header changes.

**Publish:** GitHub **Settings → Pages → branch `main` / `/docs`**. CI workflow `.github/workflows/pages.yml` rebuilds on push to `main`.

## Desktop simulator (JUCE)

**Release v1.0.3** — standalone app, VST3, and AU (see `SIM_MANUAL.md`).

Native app with **five adjacent sub-module panels** (Audio → Drive), **mod rack + patch cables**, and shared global strip. No page switching.

```sh
cd desktop
cmake -B build
cmake --build build --config Release
./build/FroggersTigaDesktop_artefacts/Release/FroggersTiga.app/Contents/MacOS/FroggersTiga   # macOS
```

Links `src/core/` + `DesktopHostIO` only (no libDaisy). Transport bar: **Play/Stop**, format toggles (**WAV/MP3/FLAC/OGG**), **Record** (stereo export), **MIDI**, **Audio**. **WAV** and **OGG** export work in default JUCE builds; **MP3** needs `JUCE_USE_MP3AUDIOFORMAT` + LAME at compile time; **FLAC** needs `JUCE_USE_FLAC`. macOS menu **FroggersTiga → Manual / Quick Dict / License** (embedded docs).

**Release packages:** See [`desktop/PACKAGING.md`](desktop/PACKAGING.md) for DMG / Windows installer commands. Push tag `desktop-vX.Y.Z` (must match `project(FroggersTigaDesktop VERSION ...)` in `desktop/CMakeLists.txt`) to trigger `.github/workflows/desktop-release.yml` and publish GitHub Release assets.

Parameter and host UX reference: [`SIM_MANUAL.md`](SIM_MANUAL.md) and [`QUICK_DICT.md`](QUICK_DICT.md).
