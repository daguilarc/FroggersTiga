# Tasks — `frogg3rs-encoder-label-plate-color`

## 0. Hygiene sweep

Sweep every directory the Impact section names — `app/`,
`External/Sheaf/projects/synth/include/synth/`,
`External/Sheaf/projects/synth/juce/`,
`External/Sheaf/projects/synth/tests/` — and name each one swept. Also verify
the tree is clean under `app/` and `External/Sheaf` before any evidence run.

## 1. Operator mock, BEFORE building

The governing requirement ("Encoder labels are legible, natural, and never
obscure the encoder") makes the operator's on-screen confirmation the
acceptance criterion, exercised on a mock BEFORE the change is built. Produce
a mock of one encoder row with the plate at `Rgb(18,20,22)` and ghost
segments at `Rgb(22,26,28)` — a doctored screenshot or a small rendered image
is fine — and get the operator's yes. Include one cell over a visualizer
underlay so the opacity purpose stays visible.

## 2. The Sheaf constant

On a NEW standalone Sheaf branch off the pinned commit:

- Add `inline constexpr Color kSurfaceBackground` (18, 20, 22) where
  `EncoderDraw.hpp`, `PortableJuceBackend.hpp`, and the three tests can all
  reach it. Read the include graph first and put it in the narrowest header
  all five sites already reach; do not invent a new palette layer.
- Reference it at `PortableJuceBackend.hpp:266` and `:275`,
  `EncoderDraw.hpp:680` (both alpha variants share its r/g/b), and in the
  three tests that pin the value
  (`MiniAppJuceBackendParityTests.cpp:312`,
  `PortableJuceBackendTests.cpp:1183`,
  `browser_command_buffer_tests.cpp:524`).
- The badge chip at `EncoderDraw.hpp:595` is deliberately unchanged.
- Gates: Sheaf synth gate AND the miniapp target
  (`make -C projects/synth/apps/miniapp test`), counts reported. The two
  braid4 96 kHz deadline failures are known on this machine.
- Push the branch, open the upstream PR (expected #11).

## 3. The app plate

On frogg3rs `main`, after the pin bump (task 4 can precede or follow this
edit, but the build that produces evidence must use the bumped pin):

- `app/FroggersUiSurface.hpp:2039`: plate colour becomes
  `synth::kSurfaceBackground`.
- `app/FroggersUiSurface.hpp:2035`: off colour becomes the plate colour
  plus (4, 6, 6), computed from the constant, not a literal.
- `app/FroggersSurfaceTests.cpp:1681`: the assertion pins the plate command
  to `synth::kSurfaceBackground`, and the off-segment assertion to the
  derived value.

## 4. Pin bump

Point `External/Sheaf` at the new branch head, as its own commit.

## 5. Gates and visual confirmation

- `make -C app test` — full count, zero FAIL.
- `make -C app/browser build` and `test` — fresh wasm, artifact deleted
  first.
- `./app/build-launcher.sh` — fresh bundle.
- OPERATOR: on the built result (browser or launcher), the label strips are
  indistinguishable from the app background on plain cells, the glyphs stay
  legible over a visualizer underlay, and the badge chips still read as
  chips on the knobs.

## 6. Constraints

No bespoke shell harnesses; every check is a `TEST_CASE` under `make` or a
direct build of a shipping target. Comments say what the colour is for, not
which change touched it.
