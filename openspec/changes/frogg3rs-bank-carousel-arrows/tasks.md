# Tasks — frogg3rs-bank-carousel-arrows

Gate after every task group: `cd app && nice make -j2 test` green (baseline
274/274 at a67b3fbf plus this change's tests; NEVER above -j2 on this
machine). One commit per numbered group, repo commit style (plain imperative,
no attribution lines). Subagent dispatch per omni-rule: lightest capable
model, sequential code changes, per-task review with §14 postflight.

## 1. Arrow row structure and emission

- [ ] 1.1 Constants: `FroggersActions::kBankPrevious/kBankNext` and
      `FroggersNodeIds::kBankPrevArrow/kBankNextArrow/kModulationHeaderTitle`,
      matching the existing `inline constexpr const char*` conventions
      (`app/FroggersUiSurface.hpp:107-220`).
- [ ] 1.2 Restructure `AppendModulationHeaderRow` (`:1288-1307`) per design:
      outer `Row` keeps id `kModulationHeader` and `Px(26)` main extent in
      BOTH drill states; level 0 emits
      `[spacer W1][prev Px][next Px][spacer W1]` with the pair centered and
      arrow glyphs drawn as triangle fills (transport-plate idiom,
      `:484`, `:940-941`); level > 0 emits the single full-width title child
      (`kModulationHeaderTitle`) with today's exact fill+text commands and NO
      arrow nodes.
- [ ] 1.3 Tests (TDD: write first): geometry — pair midpoint == band midpoint
      within tolerance, arrows inside the band, band bounds byte-identical to
      pre-change (imitate `:878-955` using `AbsoluteBounds`); drilled — no
      arrow action nodes, title commands unchanged, band bounds unchanged.

## 2. Arrow action handling

- [x] 2.1 `HandleAction` branches for `kBankPrevious`/`kBankNext` beside the
      `kBankSelect` branch (`:1858-1861`), GATED on
      `app_->DrillLevel() == 0` (MANDATORY — preflight finding, design
      §behavior: HandleAction matches action names with no node-presence
      check, so an ungated branch would accept synthetic dispatches while
      drilled and silently exit the drill via the bank-switch drain):
      `(CurrentBankIndex() ± 1 + kFroggersBankCount) % kFroggersBankCount` →
      `app_->RequestBankSelect(ix)`. No new state; single authority
      preserved.
- [x] 2.2 Tests (TDD): dispatch `kBankNext` steps +1 with highlight following
      (exactly one `selected==true` after every step, imitate `:565-612`);
      wrap 5→0 on next and 0→5 on previous; while drilled, the tree carries
      no arrow action node AND a synthetic dispatch of `kBankNext` changes
      neither the bank nor the drill level (pins the 2.1 gate).

## 3. Full gate and operator acceptance (user-gated finish)

- [ ] 3.1 Full suite green (`cd app && nice make -j2 test`), counts reported;
      rebuild the launcher (`./app/build-launcher.sh`).
- [ ] 3.2 OPERATOR GATE: the operator confirms in the built app that the
      arrows render centered between the bank row and the encoder grid at
      the top level, step and wrap the carousel with the highlight following,
      and disappear inside a modulation drill-in.
