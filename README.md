# Frogg3rs

Three loosely coupled oscillators with phase modulation and cross-coupling, polynomial drive / digital reshaping, comb filter and resonant bump, algorithmic reverb, CV modulation routing, and Marbles-style random modulation sources.

**Current development is the Sheaf-based app under [`app/`](app/README.md)** — see
[Frogg3rs — Sheaf app](#frogg3rs--sheaf-app-current-development) below. The original **Daisy Field**
hardware firmware and the pre-Sheaf desktop/browser simulators (`desktop/`, `desktop-v2/`, `sim/`,
`src/`, `wasm/`, `vcv/`, `web/`) are **frozen** — kept in the tree, byte-identical, not under active
development — while that work continues.

- **Sheaf app docs:** [`app/README.md`](app/README.md) — build instructions, Sheaf submodule pin, status
- **Daisy Field operator docs:** [`MANUAL.md`](MANUAL.md) — pages, buttons, modulation workflow, safe flash sequence
- **Quick Dict:** [`QUICK_DICT.md`](QUICK_DICT.md) — abbreviated parameter glossary
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

## Browser simulator (GitHub Pages)

**Frozen** along with the rest of the pre-Sheaf tree — see the note at the top of this file. Kept
buildable; not receiving new feature work while the Sheaf app is active.

**Release v1.0.4** — static WASM + Web Audio sim at repo `docs/` (published from `main`).

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
- **External MIDI** — CC 1 only when enabled (no CC 2 UI or ingestion); four-entry mod bay (CC 1, VCO Envelope, Random 1/2)
- Default **44.1 kHz** (`audioContext.sampleRate`)
- **Mic** toggle default **off** (VCO-only until enabled)

**Help docs:** `SIM_MANUAL.md` (sim operators — embedded in desktop/web Help → Manual) and `MANUAL.md` (Daisy Field firmware — repository only, not shipped to sim hosts). `QUICK_DICT.md` is a short parameter glossary.

**Host page labels:** `sim/ParamDisplayNames.hpp` and `sim/HostPanelLayout.hpp` are the authorities; `web/src/hostDisplay.generated.ts` is generated for instant UI labels. `node scripts/generate-host-display.mjs --check` runs on every web build and e2e run.

**Publish:** GitHub **Settings → Pages → branch `main` / `/docs`**. CI: `pages.yml` deploys the web sim; repo hygiene and browser regression checks are local commands documented in `docs/CI.md`.

## Desktop simulator (JUCE)

**Frozen** along with the rest of the pre-Sheaf tree — see the note at the top of this file. This is
`desktop/`, not `desktop-v2/`: `desktop-v2/` was a separate, later spike toward migrating this same
app onto Sheaf's parameter model in place; that spike was cleared and frozen once the from-scratch
[Sheaf app](#frogg3rs--sheaf-app-current-development) under `app/` superseded it.

**Release v1.0.4** — standalone desktop app. The public simulator manual covers the launched desktop app and web sim.

Native app with **five adjacent sub-module panels** (Audio → Drive), **mod rack + patch cables**, and shared global strip. No page switching. **MIDI Settings** exposes two hardware CC→CV pairs (CC 1 on by default, CC 2 off); QWERTY drives CC 1 only.

```sh
cd desktop
cmake -B build
cmake --build build --config Release
./build/FroggersTigaDesktop_artefacts/Release/FroggersTiga.app/Contents/MacOS/FroggersTiga   # macOS
```

JUCE is pinned in `desktop/CMakeLists.txt` at `8.0.4`. For offline or cached desktop configuration, point CMake at an existing JUCE checkout:

```sh
cd desktop
cmake -B build -DFROGGERS_JUCE_SOURCE_DIR=/path/to/JUCE
```

This uses CMake `FetchContent`'s local source override and avoids silently depending on a live network fetch during configure.

**VST3 / AU (local-only, pre-launch):** `cmake -B build -DBUILD_VST=ON` after restoring plugin sources. This surface remains under local validation and is intentionally absent from the public SIM manual until launch.

**VCV Rack (local-only, pre-launch):** `vcv/` is local-only and not built on CI. This surface remains under local validation and is intentionally absent from the public SIM manual until launch.

Links `src/core/` + `DesktopHostIO` only (no libDaisy). Transport bar: **Play/Stop**, format toggles (**WAV/MP3/FLAC/OGG**), **Record** (stereo export), **MIDI**, **Audio**. **WAV** and **OGG** export work in default JUCE builds; **MP3** needs `JUCE_USE_MP3AUDIOFORMAT` + LAME at compile time; **FLAC** needs `JUCE_USE_FLAC`. macOS menu **FroggersTiga → Manual / Quick Dict / License** (embedded docs).

**Clean rebuild check:** `scripts/verify_clean_rebuild.sh` — generator freshness, sim tests, web TypeScript, desktop build, and workspace hygiene.

**Release packages:** See [`desktop/PACKAGING.md`](desktop/PACKAGING.md) for DMG / Windows installer commands. Move tag `froggerstiga-v1` on `main` (force-push) to trigger `.github/workflows/desktop-release.yml` and publish assets on [GitHub Releases](https://github.com/daguilarc/FroggersTiga/releases).

Parameter and host UX reference: [`SIM_MANUAL.md`](SIM_MANUAL.md) and [`QUICK_DICT.md`](QUICK_DICT.md). Local OpenSpec plans under `openspec/` are workspace planning state, not public repo documentation.

## Local Planning And Hygiene

OpenSpec artifacts under `openspec/` are planning state for this workspace: change proposals, specs,
design docs, and handoffs. They are git-tracked (as of the commit that removed the prior
`.git/info/exclude` entry) but OpenSpec helpers should still not perform git operations themselves —
committing planning-doc edits is handled the same way as any other change, by the primary agent when
explicitly requested.

Subagents are not allowed to run git commands. Any git inspection, staging, committing, branching, worktree setup, or pushing is handled only by the primary agent when explicitly requested.

Path hygiene uses `scripts/repo_path_policy.sh` to keep local caches, generated build output, local-only product surfaces, and published docs mirrors classified in one place.

Shared DSP/control logic belongs in `src/core/` or `sim/` first. `src/common/` is a firmware compatibility layer; headers mirroring `src/core/<name>.hpp` stay as thin include wrappers, while firmware adapter files such as `App.hpp`, `DaisyIO.hpp`, and `Include.hpp` remain firmware-side exceptions.

Existing build outputs under `src/FroggersTiga/build`, `src/TestControl/build`, and `src/Blink/build` are firmware-scoped leftovers, not host cleanup targets. A separate firmware cleanup should decide whether to keep, remove, or ignore them.
