## MODIFIED Requirements

### Requirement: Mod rack topology differs by host

Mod rack cells SHALL come from `HostPanelLayout::kModRackCatalog`. Scope capacity SHALL be `HostPanelLayout::kScopeSampleCapacity` (96 samples).

| Mod index | Source | Desktop | Web | VST/AU | VCV | Presentation (desktop/web/VST) | VCV presentation |
|-----------|--------|---------|-----|--------|-----|--------------------------------|------------------|
| 0 | MIDI CC 1 | yes | yes | no | no | Scope | — |
| 1 | MIDI CC 2 | yes | no | no | no | Scope | — |
| 4 | VCO Envelope | yes | yes | yes | yes | Scope | Scope |
| 5 | Random 1 | yes | yes | yes | yes | LED | LED |
| 6 | Random 2 | yes | yes | yes | yes | LED | LED |

**Cell counts:** desktop **5**; web **4** (0, 4, 5, 6); VST/AU **3** (4, 5, 6); VCV **3** (4, 5, 6).

Host page **indices and labels** remain `2=Reverb`, `3=Filter` in `ParamDisplayNames`. Desktop standalone MAY permute **horizontal column positions** of pages 2 and 3 so Filter appears left of Reverb to match `FroggersEngine::ApplyOutputFx` (filter stages before reverb wet/dry) without renumbering pages.

#### Scenario: Web mod bay excludes CC 2

- **WHEN** the browser sim renders the mod bay
- **THEN** exactly four entries appear in order CC 1, VCO Envelope, Random 1, Random 2
- **THEN** no CC 2 scope, enable control, assignment path, or ingestion exists

#### Scenario: VST mod rack excludes CC cells

- **WHEN** the hosted plugin editor renders the mod rack
- **THEN** only indices 4, 5, and 6 appear with scopes on 4 and LEDs on 5/6

#### Scenario: Desktop column layout may differ from host page index order

- **WHEN** desktop standalone renders submodule columns
- **THEN** Filter (page 3) MAY appear left of Reverb (page 2) while host page indices and cross-host labels stay unchanged
