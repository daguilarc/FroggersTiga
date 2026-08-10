# Delta — `froggers-modulation-slate`

The main spec's requirement below was already correct about what it demanded (sources marked not
connected when unavailable), and the app was already satisfying it in practice — but only because a
single hardcoded flag (`kExternalAudioOptedIn = false`) compensated for a channel-exists check that
is **permanently true** on this hardware: `FroggersApp::Config()` requested one audio input, so JUCE
opened the operator's built-in microphone on every launch, unasked (traced in
`frogg3rs-external-audio-phantom-input`'s `proposal.md` §2). Delete that flag alone — which its own
comment invited, and which a since-corrected upstream doc (`UPSTREAM-SHEAF-ASK.md` item 8) told a
future reader was safe to do — and the requirement below breaks immediately, with no other change,
because the channel-exists check goes back to reading true. This delta tightens "unavailable" so
that failure class cannot recur silently: it stops being a fact that depends on one untested boolean
staying put, and becomes a fact that follows from how many input channels the app requests at all.

## MODIFIED Requirements

### Requirement: External-audio sources stay present but inert when unavailable
When no external audio input is available, the two external-audio modulation sources SHALL be
marked **not connected**. They SHALL remain present in the slate, SHALL be inert, SHALL render as
disconnected, and SHALL NOT be randomized. They SHALL NOT be hidden, and the slate SHALL NOT change
size.

**Availability is defined narrowly, and a host-opened device is not enough.** A channel merely
existing because the host opened some device — including a platform-default device the operator
never chose — SHALL NOT by itself make a source "available". Availability requires a signal that the
operator affirmatively routed audio in. Until the host exposes that signal to the app, the app SHALL
request **zero** audio input channels, so that no device — default or otherwise — is ever opened
without an explicit operator action, and the two sources are disconnected **by construction** rather
than by a compensating flag that a future edit could silently invalidate.

#### Scenario: Disconnection is the inert state, not a removal
- **WHEN** an external-audio source is unavailable
- **THEN** it is marked not connected
- **THEN** its grid cell is still present, carrying no depth parameter
- **THEN** that cell renders in the framework's standard disconnected appearance

#### Scenario: A host-opened default device does not count as available
- **WHEN** the app requests zero audio input channels
- **THEN** no input device, default or otherwise, is opened by the host
- **THEN** both external-audio sources are disconnected as a direct consequence of zero channels
  existing, independent of any separate opt-out flag

#### Scenario: Reconnection restores it in place
- **WHEN** an external audio input becomes available — the host exposes a signal that the operator
  affirmatively routed audio in, and the app requests at least one channel again
- **THEN** the source is marked connected again
- **THEN** its depth parameter materializes on next use, in the same cell position

#### Scenario: No external input connected
- **WHEN** no external audio input is available
- **THEN** both external-audio cells are still shown, rendered as disconnected
- **THEN** they contribute no modulation
- **THEN** the slate still contains fifteen sources in the same order

#### Scenario: Slate size never changes with cabling
- **WHEN** an external input becomes available or unavailable
- **THEN** no modulation cell changes position
- **THEN** existing depth assignments keep their targets

#### Scenario: Randomization skips them
- **WHEN** randomization assigns modulation depths and no external input is available
- **THEN** neither external-audio source receives depth
- **THEN** this follows from their disconnected state, with no separate randomization rule
