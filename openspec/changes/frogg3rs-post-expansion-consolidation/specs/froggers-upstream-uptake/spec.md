# Delta — `froggers-upstream-uptake`

**Added 2026-08-12.** A new capability. This app is an out-of-tree Sheaf app built against a PINNED
`External/Sheaf` (`77a3019e`) that is deliberately never forked — a fork was tried on 2026-07-27 and
reverted the same day, because the two commits existed only on one machine and the recorded gitlink was
unresolvable from any other checkout, breaking the browser publish and any fresh clone
(`UPSTREAM-SHEAF-ASK.md`). The pinned-upstream property is worth more than any feature behind it.

That constraint creates a recurring decision this project has now got wrong once and right once, so it is
worth a spec rather than a habit: when the library appears to lack something, is the app actually blocked?

**The withdrawn case, recorded because it is the reason this capability exists.** `EncoderDraw`'s
14-segment label truncates at 4 characters and `EncoderDrawState` exposes no field to change it. That much
is true. From it, this project concluded the app was blocked, wrote a blocking gate, and filed an upstream
issue — all wrong. `BuildFourteenSegmentCommands` is public with `numChars` as an ordinary parameter, and
`BuildEncoderDrawCommands` returns its command vector BY VALUE into the app's own draw lambda, so the app
composes its own label block with no upstream change at all. The issue was withdrawn and closed.

## ADDED Requirements

### Requirement: An upstream gap is proven app-unreachable before it is treated as blocking
Before any work is recorded as blocked on an upstream `External/Sheaf` change, the app SHALL first establish that no app-side route exists, by reading the whole relevant surface rather than inferring from one absent configuration field. A missing field on a state struct is not by itself evidence that a capability is unreachable, because the app also owns whatever the library returns to it and may call any public entry point directly.

#### Scenario: A missing configuration field is not sufficient evidence
- **WHEN** a library state struct exposes no field for some desired behaviour
- **THEN** the public functions and returned values of that library are also examined before concluding the app is blocked
- **THEN** an app-side composition, if one exists, is preferred over an upstream request

#### Scenario: A genuinely blocked item is recorded with what would unblock it
- **WHEN** no app-side route exists after that check
- **THEN** the item is recorded as upstream-blocked together with the specific change that would unblock it
- **THEN** it is re-checked against the pinned dependency whenever that pin moves

### Requirement: The pinned dependency is never forked or locally patched
The app SHALL NOT fork `External/Sheaf` or carry local patches against it. Every upstream need SHALL be expressed as an ask against the upstream project, and every workaround SHALL live in app code, so that a fresh clone of this repository resolves its dependency from upstream alone.

#### Scenario: A fresh clone resolves the dependency
- **WHEN** this repository is cloned on a machine that has never built it
- **THEN** the pinned dependency resolves from upstream with no local commits required

#### Scenario: A workaround lives app-side
- **WHEN** the app needs behaviour the pinned dependency does not offer
- **THEN** the workaround is implemented in app code
- **THEN** it is replaced by the library's own mechanism if upstream later provides one
