## Why

VCV plugin development is blocked by incorrect local tooling: `vcv/DEVELOPMENT.md` and `build.sh` instruct developers to `git clone https://github.com/VCVRack/Rack-SDK`, but that repository does not exist (HTTP 404). The official Rack SDK is distributed only as versioned zip archives from [vcvrack.com/downloads](https://vcvrack.com/downloads/). Even with a valid SDK, `build.sh` runs `make` instead of `make dist` and copies to `plugins/` instead of Rack 2's arch-specific `plugins-mac-arm64/` (or `plugins-mac-x64` / `plugins-lin-x64`), so `--install` cannot succeed. This change unblocks the remaining manual verification tasks on `vcv-vst-field-parity-panel`.

## What Changes

- Add `vcv/fetch-rack-sdk.sh` to download and unzip the correct SDK zip for the host OS/CPU from vcvrack.com.
- Fix `vcv/build.sh`: optional `--fetch-sdk`, run `make dist`, install via `make install` (delegate to official `plugin.mk` paths).
- Rewrite `vcv/DEVELOPMENT.md` with zip-based setup, Homebrew deps (`jq`, `zstd`), and correct install location.
- Keep `Rack-SDK/`, `vcv/dist/`, and build artifacts gitignored (already in root `.gitignore` / `vcv/.gitignore`).
- Document prerequisite: Rack 2 installed and launched once (creates user folder).

## Capabilities

### New Capabilities

- `vcv-sdk-fetch`: Download, verify, and extract official Rack-SDK zip; select correct arch artifact.
- `vcv-build-install`: Build plugin with `make dist` and install via SDK `make install` to arch-specific plugins dir.

### Modified Capabilities

- (none — tooling only; no change to `vcv-field-parity-module` runtime requirements)

## Impact

- `vcv/build.sh`, new `vcv/fetch-rack-sdk.sh`, `vcv/DEVELOPMENT.md`, `vcv/README.md`
- Root `.gitignore` / `vcv/.gitignore` (verify entries; no new tracked SDK files)
- Unblocks manual tasks on `vcv-vst-field-parity-panel` (4.6, 5.1, 5.5) once a developer runs the fixed workflow
