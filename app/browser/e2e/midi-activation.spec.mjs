// Browser MIDI reaches the runtime on the same first-in-app-action path audio
// does, with no activation lease involved. `SynthBrowserApp` constructs its
// `BrowserMidiManager` unconditionally in its constructor
// (External/Sheaf/projects/synth/browser/src/main.ts:178) and its own
// `BrowserUiBackend` dispatch wiring calls `startUserActivation()` after every
// dispatched UI action (main.ts:174-177), which requests audio and MIDI
// together (:267-276) and renders `audio:<...>; midi:<status>` into the root's
// `data-synth-status` (:275). The lease branch (:211-219) is an EAGER
// alternative for a launcher that already owns a gesture, not the only way in.
//
// A lease must NOT be introduced to make this pass: acquiring one asserts a
// user gesture this page has not had, which is what broke the site before.
//
// Why the permission is granted explicitly: the manager asks for
// `requestMIDIAccess({ sysex: true })` (midi.ts:114), so Chromium requires the
// `midi-sysex` permission, NOT `midi` alone -- granting only `midi` still
// yields `midi:offline`.
//
// The permission is necessary and not sufficient. Whether `requestMIDIAccess`
// resolves at all depends on the host having a MIDI backend behind it: macOS
// has CoreMIDI and resolves, a Linux CI runner has nothing and rejects, and the
// grant is accepted identically either way. So the run measures that capability
// in the page and asserts the outcome that is honest for the machine it is on,
// rather than treating a missing backend as a broken code path.
//
// Neither outcome says a controller is attached. `online` means the access was
// granted and the manager started; a physical controller against the published
// site is the only proof of the rest, and remains an operator check.
import { expect, test } from "@playwright/test";
import { PLAY_SELECTOR, SYNTH_ROOT_SELECTOR, waitForSurfaceReady } from "./helpers.mjs";

async function openSurface(page) {
  await page.goto("/");
  await waitForSurfaceReady(page);
}

// The first in-app action is what starts audio and MIDI together, so the status
// is only meaningful after it.
async function statusAfterFirstAction(page) {
  await page.locator(PLAY_SELECTOR).click();
  return expect
    .poll(async () => page.locator(SYNTH_ROOT_SELECTOR).getAttribute("data-synth-status"), { timeout: 20_000 });
}

// Does this browser have a MIDI backend behind the permission? Asked with the
// same call and the same sysex option the manager uses, so a `true` here means
// the manager's own request can resolve.
async function webMidiResolves(page) {
  return page.evaluate(async () => {
    if (typeof navigator.requestMIDIAccess !== "function") return false;
    try {
      await navigator.requestMIDIAccess({ sysex: true });
      return true;
    } catch {
      return false;
    }
  });
}

test.describe("browser MIDI", () => {
  test("reports a MIDI status after the first in-app action", async ({ page }) => {
    // Without the permission the honest outcome is `offline`. What this pins is
    // that the path RUNS and reports at all -- a regression that removed the
    // MIDI half, or never reached `startFromUserActivation`, leaves no `midi:`
    // in the string and fails here.
    await openSurface(page);
    await (await statusAfterFirstAction(page)).toContain("midi:");
  });

  test("reports the MIDI state its host can actually reach", async ({ page, context }) => {
    await context.grantPermissions(["midi", "midi-sysex"]);
    await openSurface(page);
    const resolves = await webMidiResolves(page);
    const status = await statusAfterFirstAction(page);
    // Where access resolves, a granted permission must carry through to
    // `online` -- a regression that drops the grant fails here. Where it does
    // not resolve, `offline` is the correct report and claiming otherwise would
    // be the defect.
    await (resolves ? status.toContain("midi:online") : status.toContain("midi:offline"));
  });
});
