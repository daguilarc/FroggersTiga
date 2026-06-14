## ADDED Requirements

### Requirement: Build uses SDK dist target

`vcv/build.sh` SHALL run `make dist` (not bare `make`) after `RACK_DIR` is resolved, producing both `dist/FroggersTiga/` and a `dist/*.vcvplugin` package per official `plugin.mk`.

#### Scenario: Successful build

- **WHEN** `RACK_DIR` points at a valid SDK and `jq` is installed
- **THEN** `./build.sh` completes with `dist/FroggersTiga/plugin.dylib` (macOS) or equivalent and at least one `dist/*.vcvplugin` file

### Requirement: Install delegates to SDK make install

When invoked with `--install`, `build.sh` SHALL run `make install` from `vcv/` with `RACK_DIR` exported, delegating plugin path resolution to SDK `plugin.mk`.

#### Scenario: macOS arm64 install path

- **WHEN** `./build.sh --install` succeeds on macOS arm64
- **THEN** a `.vcvplugin` file is copied to `~/Library/Application Support/Rack2/plugins-mac-arm64/`

#### Scenario: Linux install path

- **WHEN** `./build.sh --install` succeeds on Linux x64
- **THEN** a `.vcvplugin` file is copied to `~/.local/share/Rack2/plugins-lin-x64/`

### Requirement: Build documents host dependencies

`vcv/DEVELOPMENT.md` SHALL list required host tools: `make`, `curl` or `wget`, `unzip`, `jq`, `zstd`, and a C++ toolchain compatible with the SDK. It MUST NOT reference GitHub Rack-SDK clone instructions.

#### Scenario: Developer reads setup doc

- **WHEN** a developer follows `vcv/DEVELOPMENT.md` from a clean machine with Rack 2 installed
- **THEN** they can obtain SDK via zip, build, and install without git access to VCVRack

### Requirement: Optional fetch flag on build script

`vcv/build.sh` SHALL accept `--fetch-sdk` (or `-f`) that runs `fetch-rack-sdk.sh` before build when `RACK_DIR` is missing.

#### Scenario: One-shot fetch build install

- **WHEN** developer runs `./build.sh --fetch-sdk --install` with no prior SDK
- **THEN** SDK is downloaded, plugin is built with `make dist`, and installed with `make install`
