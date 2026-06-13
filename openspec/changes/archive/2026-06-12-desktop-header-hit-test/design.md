## Context

**Bug introduced in `desktop-chrome-cohesion` `MainComponent::resized`:**

```cpp
m_recordCluster.setBounds(header);  // 104px × full width — swallows Play clicks
```

**Component z-order (constructor add order, later = on top):**

```text
panels → modRack → strip → cableOverlay → play → stop → external → inputEnv
         → recordCluster (FULL HEADER) → audio → midi
resized: cableOverlay.toFront(false)  // topmost; hitTest false off jacks
```

Click on Play: cable (miss) → midi (miss) → audio (miss) → **recordCluster (HIT, empty area)** → Play never reached.

## Goals / Non-Goals

**Goals:**

- One layout pass computes `recordGlobal`, `formatGlobal`, transport rects, then applies bounds once.
- Cluster container is click-transparent except on children.
- Transport buttons reliably receive clicks at startup.

**Non-Goals:**

- Revert two-row header or chrome constants.
- Change `PatchCableOverlay` jack hit-testing.
- Split `RecordExportCluster` into separate MainComponent children (union bounds is sufficient).

## Decisions

### 1. Bounds = union of child areas (OMNI: accumulate then apply)

In `MainComponent::resized`, after computing global rects:

```cpp
const auto clusterGlobal = recordGlobal.getUnion(formatGlobal);
m_recordCluster.setBounds(clusterGlobal);
const auto recordLocal = recordGlobal.translated(-clusterGlobal.getX(), -clusterGlobal.getY());
const auto formatLocal = formatGlobal.translated(-clusterGlobal.getX(), -clusterGlobal.getY());
m_recordCluster.layoutChrome(recordLocal, formatLocal);
```

Remove `m_recordCluster.setBounds(header)`.

### 2. Click pass-through on cluster shell

`RecordExportCluster` constructor:

```cpp
setInterceptsMouseClicks(false, true);
```

Parent shell never steals clicks; children (RECORD, format toggles) still receive them.

### 3. Transport z-order after cable overlay

End of `MainComponent::resized`:

```cpp
juce::Component* transport[] = {
    &m_play, &m_stop, &m_externalInput, &m_inputEnvelope,
    &m_audioSettings, &m_midiSettings, &m_recordCluster};
for (juce::Component* c : transport)
    c->toFront(false);
m_cableOverlay.toFront(false);
```

Cable remains topmost for jack drags; `hitTest` returns false off ports so transport receives those clicks.

### 4. Rejected: make RecordExportCluster a non-Component helper

Union bounds + pass-through is smaller diff and preserves existing child layout API.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Union rect gap between RECORD and formats | Rects share right edge; union is contiguous |
| toFront order fragile on new header widgets | Loop explicit transport + cluster list |

## Migration Plan

1. `RecordExportCluster` intercept policy.
2. `MainComponent::resized` union bounds + local layout.
3. Transport `toFront` + rebuild + manual click test.

## Open Questions

- None blocking.
