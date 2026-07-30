## ADDED Requirements

### Requirement: Unified Application surface replaces carousel paging
Desktop v2 SHALL retire single-active-page carousel prev/next paging as the primary module navigation model. The Application surface SHALL present module sections together (unified surface) without requiring carousel arrows to reach another module’s parameters.

#### Scenario: All remaining modules visible without carousel arrows
- **WHEN** the Application surface loads at the default standalone size
- **THEN** Audio, Envelope, Filter, Drive, Reverb, and Delay module sections are reachable without carousel prev/next
- **THEN** no Random S&H module section is present

## REMOVED Requirements

### Requirement: Module carousel as sole one-module-at-a-time pager
**Reason:** Absorbed unified-parameter-layout; carousel paging replaced by unified Application surface under Runtime.  
**Migration:** Remove `PageCarouselComponent` paging model; render module sections on the Application surface; update layout tests.
