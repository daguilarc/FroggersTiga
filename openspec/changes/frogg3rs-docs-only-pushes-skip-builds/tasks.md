# Tasks — `frogg3rs-docs-only-pushes-skip-builds`

- [x] 1. Add `paths-ignore: ['openspec/**', '**.md']` under `push:` in
  `pages.yml` and under both `push:` and `pull_request:` in
  `vst-plugin.yml`. Touch nothing else in either file.
- [x] 2. Commit and push (this push changes workflow files, so both builds
  SHOULD run — that is the positive control's first half). Pushed as
  `b70db29`.
- [ ] 3. Positive control, both directions: after the workflow push's runs
  appear, make a docs-only commit (openspec/** only), push it, and confirm
  via `gh run list` that it starts NO Pages or VST run. Record both
  observations here.
- [ ] 4. Confirm the `frogg3rs_vst` tag trigger and `workflow_dispatch`
  still fire on demand the next time either is used; path filters do not
  apply to tags, so no pre-verification run is spent on this.
