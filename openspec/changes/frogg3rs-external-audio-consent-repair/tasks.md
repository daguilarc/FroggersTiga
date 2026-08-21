# Tasks — `frogg3rs-external-audio-consent-repair`

Hygiene is step zero of every change (omni-rule §13.0), but this change is a
narrow conformance repair on files a sweep just covered; group 0 is therefore
a check, not a sweep.

Gates: `cd app && nice make -j2 test` (300/300); plugin targets from
`app/vst/build` — `FroggersVstHostTests` 46/46, smoke 1/1, editor 3/3; Sheaf
`nice make -C projects/synth -j2 test`, green means 920 passed / 2 failed and
the two are the known braid-4 deadline tests, NEVER a zero exit code. Never
above `-j2`, always `nice`.

## 0. Hygiene check

- [ ] 0.1 Confirm the files this change touches carry no planning-doc labels
      and no debug markers before and after. They were swept on 2026-08-21;
      this is a check that the sweep held, not a re-sweep.

## 1. The routed signal stops believing a device nobody chose

- [ ] 1.1 Trace and report, before changing anything: `RefreshInputRoutedState`
      in `External/Sheaf/projects/synth/runtime/Runtime.hpp`, the browser's
      own `RefreshInputRoutedState` in `include/synth/browser/BrowserRuntime.hpp`,
      and `BuildDeviceOptions`/`kSystemDefaultOptionId` in
      `include/synth/RuntimePages.hpp`. State for each host what currently
      makes routed true.
- [ ] 1.2 Add an explicit no-input option to the INPUT device list, and make
      it the default. "System default" as the only starting state is what
      makes a host-opened device indistinguishable from a choice. Output
      device selection is NOT affected — only input.
- [ ] 1.3 Make the routed signal report routed only for an operator-selected
      device, in the standalone and in the browser. With no-input as the
      default, a selected device genuinely is the operator's act — so the
      predicate becomes honest rather than accidentally true.
- [ ] 1.4 Do not open a platform input device at launch merely because one
      channel was requested. If that is not avoidable from the app side,
      report what it would take rather than working around it.
- [ ] 1.5 Tests in Sheaf's own suite and style: a host-opened default device
      reports NOT routed; selecting a device reports routed; selecting
      no-input returns to not routed. POSITIVE CONTROL required — the routed
      case must prove the signal can be true, or "not routed" proves nothing.

## 2. The encoders look and behave unusable

- [ ] 2.1 While not connected, the two external-audio cells render in a
      disabled appearance distinguishable at a glance from a connected cell.
      Use the framework's existing disconnected treatment if it is already
      distinct enough; report whether it is, rather than adding a second
      mechanism on top of one that already works.
- [ ] 2.2 A disconnected cell REJECTS edits rather than accepting an
      adjustment that does nothing. Trace where an encoder edit reaches the
      depth parameter and refuse it at that seam.
- [ ] 2.3 Tests: a disconnected cell renders disabled and an edit against it
      changes nothing; a connected cell accepts the same edit. That second
      half is the positive control and is not optional.

## 3. Verify against the running app, not only the suite

- [ ] 3.1 Launch the standalone and confirm what the operator sees on the
      Audio page: input reads as no input, and the two external-audio cells
      are visibly disabled. The suite passing is not this step; this defect
      shipped under a passing suite because nothing asserted the operator's
      view.
- [ ] 3.2 Confirm the plugin still behaves as it already does — its input
      selection already defaults to None and gates consent correctly. This
      change must not regress the one host that was right.

## 4. Close

- [ ] 4.1 All gates green, counts reported.
- [ ] 4.2 Postflight: implementation versus proposal, plus a duplication pass
      over the whole diff for every new named concept.
- [ ] 4.3 Push Sheaf before the superproject pin.
- [ ] 4.4 OPERATOR: re-test. This change exists because a requirement was
      satisfied on paper and not in the running app.
