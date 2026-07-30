## 1. Fetch script

- [x] 1.1 Add `vcv/fetch-rack-sdk.sh`: detect OS/CPU, download `Rack-SDK-${RACK_SDK_VERSION:-2.6.6}-<arch>.zip` from vcvrack.com/downloads, extract to `vcv/../Rack-SDK`, verify `plugin.mk`
- [x] 1.2 Skip download when `RACK_DIR/plugin.mk` already exists; exit non-zero with vcvrack.com URL on HTTP/extract failure
- [x] 1.3 `chmod +x` fetch script; add `RACK_SDK_VERSION` env override to DEVELOPMENT.md

## 2. Build and install

- [x] 2.1 Fix `vcv/build.sh`: add `--fetch-sdk` / `-f` calling fetch script when SDK missing
- [x] 2.2 Replace bare `make` with `make dist`; replace manual `cp` with `make install` when `--install` passed
- [x] 2.3 Pre-flight check for `jq` (plugin.mk reads plugin.json); print `brew install jq zstd` hint on failure
- [x] 2.4 Remove all GitHub Rack-SDK clone messages from build.sh error output

## 3. Documentation and gitignore

- [x] 3.1 Rewrite `vcv/DEVELOPMENT.md`: zip-based SDK setup, host deps, Rack 2 prerequisite, `./build.sh --fetch-sdk --install`, correct `plugins-mac-arm64` install path
- [x] 3.2 Update `vcv/README.md` quick start to match (no git clone)
- [x] 3.3 Verify root `.gitignore` and `vcv/.gitignore` cover `Rack-SDK/`, `vcv/dist/`, `vcv/dep/`, `vcv/build/`

## 4. Verification

- [ ] 4.1 Run `./build.sh --fetch-sdk --install` on macOS arm64; confirm `.vcvplugin` in `plugins-mac-arm64/`
- [ ] 4.2 Launch Rack; confirm Froggers Tiga + Expander appear in Module Browser
- [ ] 4.3 Run `sim/check_vcv_license_boundary.sh` after build (artifact check path)
