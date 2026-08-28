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
// yields `midi:offline`. Headless grants no real devices either way, so
// `online` here means the access was granted and the manager started, not that
// a controller is attached. A physical controller against the published site
// is the only proof of that, and remains an operator check.
import { expect, test } from "@playwright/test";
import { PLAY_SELECTOR, SYNTH_ROOT_SELECTOR, waitForSurfaceReady } from "./helpers.mjs";

async function statusAfterFirstAction(page) {
  await page.goto("/");
  await waitForSurfaceReady(page);
  await page.locator(PLAY_SELECTOR).click();
  return expect
    .poll(async () => page.locator(SYNTH_ROOT_SELECTOR).getAttribute("data-synth-status"), { timeout: 20_000 });
}

test.describe("browser MIDI", () => {
  test("reports a MIDI status after the first in-app action", async ({ page }) => {
    // Without the permission the honest outcome is `offline`. What this pins is
    // that the path RUNS and reports at all -- a regression that removed the
    // MIDI half, or never reached `startFromUserActivation`, leaves no `midi:`
    // in the string and fails here.
    await (await statusAfterFirstAction(page)).toContain("midi:");
  });

  test("reaches midi:online once Web MIDI is permitted", async ({ page, context }) => {
    await context.grantPermissions(["midi", "midi-sysex"]);
    await (await statusAfterFirstAction(page)).toContain("midi:online");
  });
});
