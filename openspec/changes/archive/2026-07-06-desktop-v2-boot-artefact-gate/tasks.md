## 1. Documentation and launch path

- [ ] 1.1 Confirm `desktop-v2/PACKAGING.md` macOS table lists `Release/FroggersTigaV2.app` (not root-level bundle)
- [ ] 1.2 Add `scripts/open-desktop-v2.sh` — resolve repo root, `open` Release `.app`, exit 1 with message if missing
- [ ] 1.3 Update `QUICK_DICT.md` boot-outcome gloss: Release path + `open-desktop-v2.sh`; run `scripts/sync-help-docs.sh`

## 2. Stale bundle prevention

- [ ] 2.1 Add CMake `POST_BUILD` on `FroggersTigaDesktopV2` to remove stale `FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app` when it differs from current Release output
- [ ] 2.2 One-time delete existing stale root bundle in working tree; rebuild Release and verify only Release path exists

## 3. Boot smoke hardening

- [ ] 3.1 Extract `resolveV2BinaryPath()` to shared test helper (`desktop-v2/tests/V2BinaryPath.hpp`)
- [ ] 3.2 Rewrite `BootSmoke_test.cpp`: `waitpid(WNOHANG)` poll loop; FAIL on early exit/signal; SIGTERM cleanup
- [ ] 3.3 Register helper + BootSmoke in `CMakeLists.txt` if new source file added

## 4. Verification

- [ ] 4.1 Build Release: `cmake --build desktop-v2/build --config Release`
- [ ] 4.2 Run `./scripts/open-desktop-v2.sh` — window stays open ≥3s
- [ ] 4.3 Run `ctest --test-dir desktop-v2/build -R BootSmoke --output-on-failure` — PASS against Release binary
- [ ] 4.4 Confirm stale root `.app` absent or non-launchable after POST_BUILD
- [ ] 4.5 OMNI compliance: no hedge language in changed files; nesting ≤3 in new test loops

## 5. Manual QA commands (operator)

```sh
cmake --build desktop-v2/build --config Release
./scripts/open-desktop-v2.sh
ctest --test-dir desktop-v2/build -R BootSmoke --output-on-failure
```

Expected: app window opens and stays open; BootSmoke PASS.
