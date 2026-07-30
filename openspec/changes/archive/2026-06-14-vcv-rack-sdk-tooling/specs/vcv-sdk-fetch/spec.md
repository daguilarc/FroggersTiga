## ADDED Requirements

### Requirement: SDK zip is fetched from official VCV downloads

The fetch script SHALL download Rack-SDK from `https://vcvrack.com/downloads/` only. It MUST NOT use `git clone` or any GitHub URL for Rack-SDK.

#### Scenario: macOS Apple Silicon fetch

- **WHEN** the host is `Darwin` with CPU `arm64` and no valid `RACK_DIR` exists
- **THEN** the script downloads `Rack-SDK-<latest-or-pinned>-mac-arm64.zip` and extracts it so `plugin.mk` exists at the target `Rack-SDK/` directory

#### Scenario: macOS Intel fetch

- **WHEN** the host is `Darwin` with CPU `x86_64` and no valid `RACK_DIR` exists
- **THEN** the script downloads `Rack-SDK-<version>-mac-x64.zip` and extracts it so `plugin.mk` exists at the target `Rack-SDK/` directory

#### Scenario: Linux x64 fetch

- **WHEN** the host is `Linux` with CPU `x86_64` and no valid `RACK_DIR` exists
- **THEN** the script downloads `Rack-SDK-<version>-lin-x64.zip` and extracts it so `plugin.mk` exists at the target `Rack-SDK/` directory

#### Scenario: Existing SDK is reused

- **WHEN** `RACK_DIR` is set and `$RACK_DIR/plugin.mk` exists
- **THEN** the fetch script exits successfully without downloading

### Requirement: SDK extract path matches build auto-detect

The default extract target SHALL be `FroggersTiga/Rack-SDK/` (repo root sibling of `vcv/`, i.e. `vcv/../Rack-SDK`). That path MUST remain gitignored.

#### Scenario: Extract layout

- **WHEN** the zip is extracted at the repo root
- **THEN** `Rack-SDK/plugin.mk` exists and `build.sh` auto-detect resolves `RACK_DIR` to that directory

### Requirement: Fetch failures surface actionable errors

The fetch script SHALL verify HTTP 200 and presence of `plugin.mk` after extract. On failure it MUST print the vcvrack.com downloads URL and exit non-zero.

#### Scenario: Download 404

- **WHEN** the download URL returns a non-200 status
- **THEN** the script exits with an error naming the URL and suggesting manual download from vcvrack.com/downloads
