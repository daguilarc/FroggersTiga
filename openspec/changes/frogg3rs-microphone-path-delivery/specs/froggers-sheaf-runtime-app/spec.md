# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: A rendered control is routable

Every action a runtime page can emit SHALL be routed by the host that renders
that page. A control that renders and dispatches into nothing SHALL be caught by
a check rather than by an operator finding it inert.

Where a page's actions form a fixed set, that set SHALL have one definition,
read by both the page that emits from it and the host that routes from it. Two
lists expected to agree are the defect: adding a control to one of them is not
required to touch the other, and the button ships live and dead at once.

A page whose routing rule is not a fixed set — one that also admits actions by
prefix — SHALL share the fixed half and keep the prefix rule, which membership
cannot express.

#### Scenario: A page cannot emit an unroutable action
- **WHEN** the Audio, File or Sync page's tree is built with every control shown
- **THEN** every action it emits appears in the list its host routes from

#### Scenario: The page and the host read one list
- **WHEN** the host decides whether an action belongs to the Audio, File, Sync
  or sidebar surface
- **THEN** it reads that surface's own action list rather than restating it

#### Scenario: Removing an action from the shared list is caught
- **WHEN** an action a page emits is removed from that page's action list
- **THEN** a check fails naming the page that emits it
