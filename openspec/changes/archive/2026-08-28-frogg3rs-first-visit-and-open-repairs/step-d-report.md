# Section 2 report — measuring the load readout

**Instrument: raw published samples, from the real frogg3rs DSP.** Not a proxy.
The frogg3rs package was staged into Sheaf's browser harness so the worker
handle was reachable, and `deadlineMicrounits` was polled off the
`audio-worklet-stats` message every ~100ms — the meter's own publish window
(`kPublishWindowMicros`, `BrowserRuntime.hpp:329`). 645-648 samples per rate
over 65s each, four rates. The conversion was read, not assumed:
`DeadlineMicrounits` (`BrowserRuntime.hpp:338-349`) computes
`elapsed * 100 / block` and stores it times 1e6, so microunits / 1e6 IS the
percentage the sidebar shows.

**VERDICT: the run is VOID as a model of a phone. Neither (a) nor (b).**

Not "inconclusive" as a hedge — void in §8.1's specific sense: the controlling
quantity did not move the way the instrument was supposed to move it. It moved
the opposite way.

## Why it is void

CPU throttling never reached the audio thread. Two independent proofs from the
run's own data:

**1. Audio ran at exactly real time at every rate.** At 48 kHz with 128-frame
blocks, real time is 375.0 blocks/s:

| rate | blocks | span | blocks/s | vs real time |
|---|---|---|---|---|
| 1 | 24402 | 64.9s | 375.9 | 100.2% |
| 4 | 24402 | 64.9s | 375.7 | 100.2% |
| 6 | 24418 | 65.0s | 375.8 | 100.2% |
| 20 | 24434 | 64.9s | 376.2 | 100.3% |

An AudioWorklet genuinely throttled 20x could not hold real time; it would
underrun continuously. It held it to within 0.3%, identically at 1x and 20x.

**2. The load fell as throttling rose.** Median deadline: 27.6% unthrottled,
18.7% at 4x, 12.8% at 6x, 13.8% at 20x. Throttling a renderer's main thread
makes it compete LESS with the audio thread, so the DSP's share improves. If
the worklet were being throttled the figure would climb, not drop.

`Emulation.setCPUThrottlingRate` throttles the renderer's main thread. The
AudioWorklet runs on its own real-time audio thread and is not subject to it.
The plan called CDP throttling a FIRST ATTEMPT with no working invocation to
diff against; this is the discovery that first attempt produced, and it is a
finding about the instrument, not about the instrument's operator.

## What the run does establish

On this machine (M-series Mac, 8 cores), with the real frogg3rs DSP running and
confirmed live (`contextState: "running"`, `progressed: true`, 48 kHz, blocks
advancing):

| rate | n | min | median | p95 | max | samples >100% | max in first 5s |
|---|---|---|---|---|---|---|---|
| 1 | 645 | 12.8 | 27.6 | 33.5 | 36.5 | **0** | 36.5 |
| 4 | 648 | 11.8 | 18.7 | 18.7 | 18.7 | **0** | 18.7 |
| 6 | 647 | 11.8 | 12.8 | 12.8 | 12.8 | **0** | 12.8 |
| 20 | 639 | 11.8 | 13.8 | 18.7 | 18.7 | **0** | 13.8 |

Zero samples above 100% anywhere, at any rate, including startup. Steady state
on this hardware is 12-28%.

The published value is coarsely quantised — 25 distinct values unthrottled,
only 2 at rate 6 — because a 100ms window at 375 blocks/s aggregates ~37
callbacks and the ratio lands on a small set of levels.

## What it does NOT establish, and this is the point

The prediction on record was a 50-80% steady state with brief excursions
crossing 100%. This hardware shows 12-28% and no excursions at all. So the run
cannot distinguish (a) startup transient from (b) recurring overrun — because
it never produced the phenomenon either explanation is FOR.

Therefore:
- The shipped one-second window is NOT confirmed as the fix. Its positive
  control still has not been run.
- It is also NOT refuted. Nothing here says the readout is dishonest or that
  DSP cost is the cause.
- The operator's ">100% on a phone" remains unexplained by measurement.

Recording "steady state is 12-28%, no windows over 100%" as if it settled the
question would be the same error the predecessor made — presenting a
measurement of the wrong thing as a passed control.

## What would actually settle it

A real phone. Operator item 6.3 is now the ONLY instrument that can answer
this, not a nice-to-have confirmation of a headless result:

- CDP cannot throttle the audio thread, so no desktop-headless configuration
  reproduces a phone's DSP pressure.
- A phone differs in ways throttling does not model anyway: a different
  audio callback size and sample rate, a mobile scheduler that deprioritises
  background threads, and thermal throttling that arrives over minutes.

When that happens, the number to capture is the sidebar readout over time —
whether it crosses 100% once at startup or keeps returning there.

## Task 2.4 — where the numbers live

These figures belong with the requirement they bear on. "The runtime chrome
reports its load honestly" is now in
`openspec/specs/froggers-sheaf-runtime-app/spec.md`, synced from the
predecessor during section 0. It is NOT amended here, because nothing measured
contradicts it — the requirement asks for an honest readout, not for a
one-second window specifically.

## Reproducing this

The harness is deleted (below). To redo it: stage the frogg3rs package into
Sheaf's browser harness, poll `{ type: "audio-worklet-stats" }` through the
worker's `request()`, and divide `deadlineMicrounits` by 1e6. Confirm audio is
live first — `blocks` must advance at ~375/s — or the samples are meaningless.
Do not bother with `Emulation.setCPUThrottlingRate`; it cannot reach the thread
that matters.

Raw samples: `scratchpad/step-d-results.json` (294 KB, 2579 samples).
