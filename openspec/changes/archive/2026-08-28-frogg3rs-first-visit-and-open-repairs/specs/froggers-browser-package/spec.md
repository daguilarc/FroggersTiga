# Delta — `froggers-browser-package`

GitHub Pages cannot send COOP/COEP headers, so the published site's
cross-origin isolation comes from a service-worker shim that reloads the page
once to gain a controller. The boot path does not know that reload is coming,
so on a first visit it boots without isolation, fails, and paints "frogg3rs
could not start in this browser." on a browser that is about to work fine.

Nothing said the boot path owed the shim anything, so nothing caught it. The
site is right about being broken for the second or so before the reload lands,
and wrong that it is broken at all.

## ADDED Requirements

### Requirement: A first visit does not report a failure the reload will fix

A first visit SHALL reach a working instrument on any browser capable of
running it. Where the site's isolation depends on a service worker that has not
yet taken control, the boot path SHALL wait for that attempt to settle, and
SHALL NOT report a startup failure while an isolation attempt is still owed to
the page.

An attempt is owed from the moment the shim decides to make one until that
attempt has settled — either by reloading, or by establishing that no reload is
coming. It is NOT owed once the shim's one-shot guard has already fired: the
page has had its reload, and any failure after that is real and SHALL surface
exactly as it does today.

The state that says whether an attempt is owed SHALL be published by the shim,
which owns the reload guard, and read by the boot path. The boot path SHALL NOT
re-derive the guard's storage key.

#### Scenario: A first visit under load
- **WHEN** the site is served without isolation headers and the service worker
  has not yet taken control
- **THEN** no boot-failure panel is painted before the shim's reload

#### Scenario: A failure after the reload still reports
- **WHEN** the shim's one-shot guard has already fired and isolation still does
  not hold
- **THEN** the boot-failure panel appears with the underlying error

### Requirement: Suppression is bounded by an attempt that can settle

Suppressing the failure report SHALL be conditional on an attempt that can
finish. Where the shim establishes that no reload will happen — its registration
fails, the page is already controlled, or its one-shot guard declines a second
attempt — it SHALL say so, and the boot path SHALL resume reporting.

A page that suppresses its own failure report indefinitely is worse than the
failure it hides: the operator is left with a blank frame and no reason for it,
which is the exact condition the boot-failure panel was added to end.

#### Scenario: The service worker cannot register
- **WHEN** service-worker registration fails outright
- **THEN** the boot-failure panel appears rather than the page waiting silently

### Requirement: Every path that paints the failure honours the same condition

Every code path able to paint the boot-failure panel SHALL honour the same
suppression condition.

The site paints that panel from more than one place: the boot module's own
handler, and an inline handler in the page shell that exists to catch a failure
of the boot module's own imports and therefore cannot import anything. A suppression applied to one of them
leaves the other painting the panel the suppression exists to prevent.

#### Scenario: The inline handler respects the pending attempt
- **WHEN** an isolation attempt is still owed and an error reaches the page
  shell's own handler
- **THEN** that handler paints nothing either
