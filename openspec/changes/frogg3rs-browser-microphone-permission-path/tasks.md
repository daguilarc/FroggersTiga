# Tasks — `frogg3rs-browser-microphone-permission-path`

Successor to `frogg3rs-browser-audio-device-selection`, which is deployed and
leaves a fresh visitor with no route to a microphone.

There is no time pressure. Limits on what is printed are not limits on what is
checked.

## 0. Reproduce the defect before touching anything

- [ ] 0.1 POSITIVE CONTROL FIRST, and this one is not optional: reproduce the
      deadlock in a test that FAILS against the code as it stands today. A page
      whose `enumerateDevices` reports the real unpermitted shape — entries with
      empty `label` and empty `deviceId` — must be shown to have no reachable
      action that results in a `getUserMedia` call. Record the failure text.
      Do NOT use `--use-fake-device-for-media-stream` to build this: that flag
      populates labels without permission and deletes the condition under test.
      Stub the enumeration instead, the way the existing unpermitted-page test
      already does.
- [ ] 0.2 Confirm the Retry dead end by running it, not by reading it: on a
      fresh page with No Input selected, clicking Retry must be shown to arm the
      release sentinel rather than a capture request. If it does something else,
      the proposal's trace is wrong and the design changes.

## 1. Establish the route before building it

- [ ] 1.1 Trace and STATE the design before writing it, including which control
      the operator uses, what it is called, when it appears, and what happens to
      the stream a permission prompt necessarily opens. `getUserMedia` is the
      only call that prompts, so granting permission necessarily opens a device
      for some interval; say explicitly how that interval ends and how it stays
      distinct from capture nobody selected.
- [ ] 1.2 Check the standing requirement it must not break:
      `froggers-modulation-slate:89-93` forbids opening a capture device on the
      strength of a declared channel count, and `:169-176` requires that
      selecting a device opens it. Say which of these the new control is, and
      why it is not the other.
- [ ] 1.3 Decide when the control is offered. A page that already holds
      permission does not need it; a machine with no input devices should not be
      invited to ask for one. Establish how each state is distinguished —
      `navigator.permissions.query({name:"microphone"})` reports state without
      prompting, and its availability across browsers is a fact to measure, not
      to assume.

## 2. Build it

- [ ] 2.1 Implement the design from 1.1. If it needs a new action name, follow
      the existing pattern: C++ arms a pending request during dispatch, JS polls
      and performs the effect. Do NOT add a second consumable export — one
      pending request already carries which control it belongs to.
- [ ] 2.2 After permission is granted, re-enumerate and resubmit so labels
      populate and the list fills. The selection stays at No Input: earning
      labels is not choosing a device.
- [ ] 2.3 A denied permission is reported, not swallowed. The status line
      already carries input diagnostics.

## 3. Tests that can enter the state

- [ ] 3.1 The 0.1 test now passes: an unpermitted page has a reachable action
      that results in a `getUserMedia` call.
- [ ] 3.2 After a grant, the list contains the real device and the selection is
      still No Input.
- [ ] 3.3 A denial surfaces in the status line.
- [ ] 3.4 The permission request does not leave a stream running.
- [ ] 3.5 REVIEW THE HARNESS, not just the tests: every existing device test
      runs with `--use-fake-device-for-media-stream`. Enumerate what that flag
      makes untestable and say which of this feature's states no test can
      currently enter. A suite that cannot reach a state cannot defend it.

## 4. Operator — each one verified observable BEFORE it is written

- [ ] 4.0 GATE ON THIS. For every operator item below, name the code path that
      produces the state being checked, with file:line. An item whose path
      cannot be named is deleted, not shipped. The previous change sent an
      operator to a browser to confirm behaviour nothing could render.
- [ ] 4.1 OPERATOR: a browser that has never been granted permission reaches a
      prompt through the interface.
- [ ] 4.2 OPERATOR: after granting, the Input list names the real microphone.
- [ ] 4.3 OPERATOR: selecting it starts capture from that device.
- [ ] 4.4 OPERATOR: loading the page still prompts for nothing on its own.

## 4b. The desktop host, same subject

- [ ] 4b.1 ANSWER THE QUESTION FIRST. Build with `app/build-launcher.sh`,
      select an input device, and record exactly what happens. Nothing in this
      section may be acted on before that result exists.
- [ ] 4b.2 Act on the result, whichever it is: if input selection fails or the
      process dies, establish the cause and fix it, adding
      `NSMicrophoneUsageDescription` only if the observed cause is its absence.
      If input selection works, record that and close this section — the absent
      key is then a fact with no consequence and nothing is owed.
- [ ] 4b.4 Check the VST bundle's own plist for the same omission before
      concluding it is unaffected — the DAW owns the device, but the plugin
      bundle still declares its own Info.plist.

## 4c. Delay's Wet mix

- [ ] 4c.1 POSITIVE CONTROL FIRST: a test that FAILS today, showing Wet mix at
      maximum with Send at its default produces silence. Record the failure text
      before changing anything.
- [ ] 4c.2 Cap the mapped mix with `kMaxDelayWetMix`, mirroring
      `kMaxReverbWetMix`'s placement and idiom. Cap the MAPPED value, not the
      knob range, so the control keeps its full sweep.
- [ ] 4c.3 Wet mix's authority scales with the wet path's MEASURED level, not
      with Send. A low-Send high-Feedback patch holds a loud echo and the knob
      must earn its full travel there. `ToReverbMono` receives `dmix` and the
      wet pair but not the measurement, so decide where the measurement lives
      and say why.
- [ ] 4c.4 With Send at zero the follower's TARGET is forced to zero, so
      authority fades at the release constant rather than cutting. No second
      envelope and no extra branch: the fade is the follower doing its job.
- [ ] 4c.4b Measurement: follow `monoWet`, which `ToReverbMono` already
      computes, so one follower serves both channels. Attack 10ms, release
      `kSharedReleaseSeconds` (100ms, already applied to this signal by the wet
      limiter). Do NOT reuse the limiter's own envelope: it is a gain
      multiplier that sits at 1.0 under threshold and cannot distinguish a
      quiet echo from silence.
- [ ] 4c.4c Measure the added cost rather than asserting it is small: report
      the per-sample operation count and the ISR headroom before and after.
      A wet/dry control's authority is not worth a measurable dropout risk.
- [ ] 4c.5 §7, FORWARD: `kMaxDelayWetMix` is a new named concept. Enumerate
      every other wet/dry crossfade in the DSP by operand and report which have
      a cap and which do not. Reverb was capped alone and Delay was left for two
      days; do not repeat that by capping Delay alone.
- [ ] 4c.6 Assert the default patch is audible with Wet mix at maximum. That is
      the claim the operator made when reporting this.

## 5. Delivery

- [ ] 5.1 Gates with counts. Sheaf browser suite, app suite, VST ctest, and the
      frogg3rs e2e. Fixture servers must be listening on 4173, 4174 and 4175
      before any Playwright run: the config gates readiness on 4173 alone while
      the specs use all three, so a run against a partially-started server
      produces a flood of connection-refused failures that look like defects.
- [ ] 5.2 Rebuild the wasm. The snapshot is C++ compiled into it.
- [ ] 5.3 Sheaf pushed to the fork, pin bumped as its own commit afterwards,
      both trees clean.
- [ ] 5.4 POSTFLIGHT: every SHALL in the delta has a task AND a check that runs,
      and every operator item has a named code path. Both halves, separately.
