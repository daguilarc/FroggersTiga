# Desktop v2 packaging (local-only)

**FroggersTigaV2** is a local preview target on the `froggerstiga-desktop-v2` feature branch. Public GitHub releases and CI continue to ship **v1 desktop** and **web sim** only until §10 QA sign-off and merge to `main`.

## Version source of truth

`desktop-v2/CMakeLists.txt` declares:

```cmake
project(FroggersTigaDesktopV2 VERSION x.y.z)
```

Current preview: **2.0.0**. No public release tag exists for v2 yet.

## Prerequisites

- CMake 3.22+
- C++17 toolchain (Xcode CLT on macOS, Visual Studio 2022 on Windows)
- Network or local JUCE cache for first configure (`FetchContent` pins JUCE **8.0.4**)

Optional offline configure:

```sh
cmake -S desktop-v2 -B desktop-v2/build \
  -DBUILD_DESKTOP_V2=ON \
  -DFROGGERS_JUCE_SOURCE_DIR=/path/to/JUCE-8.0.4
```

Reuse the v1 desktop JUCE checkout when present:

```sh
cmake -S desktop-v2 -B desktop-v2/build -DBUILD_DESKTOP_V2=ON \
  -DFROGGERS_JUCE_SOURCE_DIR=desktop/build/_deps/juce-src
```

## Build Release

From repository root:

```sh
cmake -S desktop-v2 -B desktop-v2/build -DBUILD_DESKTOP_V2=ON
cmake --build desktop-v2/build --config Release
```

Artefact locations:

| Platform | Release output |
|----------|----------------|
| macOS | `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app` |
| Windows | `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.exe` (+ DLLs in same folder) |

Run tests:

```sh
ctest --test-dir desktop-v2/build --output-on-failure -R ControlCoreBridge
```

## Embedded docs

The app bundles canonical operator docs from the repo root:

- `SIM_MANUAL.md` (includes **Desktop v2** section)
- `QUICK_DICT.md`
- `LICENSE`

Update root `SIM_MANUAL.md` first; mirrors under `docs/` and `web/public/` must stay in sync (`sim/check_operator_docs_sync.sh`).

## Unsigned builds

Like v1 desktop packages, local v2 builds are **not** code-signed. macOS Gatekeeper or Windows SmartScreen may block first launch — use right-click **Open** or local dev workarounds.

## Release policy

| Phase | Where | Gate |
|-------|--------|------|
| Development | Feature branch `froggerstiga-desktop-v2` | `BUILD_DESKTOP_V2=OFF` in CI |
| Local QA | Developer machine | Desktop v2 app, web subset, Playwright e2e |
| Public release | After merge to `main` | User approves; packaging scripts and release tag TBD |

Do **not** publish v2 DMG/installer or move a public release tag until desktop v2, web expansion + Crunchy, and shared engine paths pass manual QA (OpenSpec §10).
