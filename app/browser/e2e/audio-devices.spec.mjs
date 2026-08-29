// The Audio page's Input dropdown is built at runtime from whatever
// `navigator.mediaDevices.enumerateDevices()` reports (audio.ts's own
// `submitAudioDevices`, filtered and turned into options by
// `BuildBrowserAudioSnapshot`, BrowserAudioDevices.hpp), not from a
// compile-time list. This spec runs under its own Playwright project
// ("audio-devices", playwright.config.mjs), the only one launched with
// `--use-fake-device-for-media-stream`, so enumeration reports real,
// labelled capture devices instead of the three blank placeholder entries a
// permission-less page otherwise gets. See that project's own comment for
// the measured numbers this suite is designed against.
//
// Capture (`getUserMedia`) is armed only by the operator selecting a device
// from the Input combo (audio.ts's `AudioBridge.acquireInputDeviceAtIndex`,
// reached only through `consumePendingAudioRequest` in main.ts);
// enumerating devices and opening the Audio page never calls it. Nothing
// here ever selects a device, so nothing here proves a real microphone's
// audio arrives -- that stays an operator check against the published site.
import { expect, test } from "@playwright/test";
import {
  AUDIO_INPUT_SELECT_SELECTOR,
  AUDIO_STATUS_LINE_SELECTOR,
  PLAY_SELECTOR,
  SIDEBAR_BUTTON_SELECTORS,
  waitForAudioOnline,
  waitForSurfaceReady,
} from "./helpers.mjs";

// RuntimePages.hpp's own `kNoInputOptionId`/`kNoInputOptionLabel` -- the
// option `BuildBrowserAudioSnapshot` always puts first, before any
// enumerated device, and falls back to whenever no other selection holds.
const NO_INPUT_OPTION_ID = "no_input";
const NO_INPUT_OPTION_LABEL = "No Input";

// `BrowserAudioInputStatusText(BrowserAudioInputStatus::NotRequested)`
// (BrowserAudioDevices.hpp) -- the exact text the Audio page's status line
// renders while the native runtime has never been asked to acquire an input
// device. Every other capture status renders different text.
const CAPTURE_NOT_STARTED_TEXT = "microphone capture not started";

// The Input combo and its status line only render once the app's own
// first-in-app action has run (`startFromUserActivation`, which is what
// discovers the app's requested input-channel count) -- the same path
// audio-activation.spec.mjs and midi-activation.spec.mjs both open with.
async function openAudioPage(page) {
  await page.goto("/");
  await waitForSurfaceReady(page);
  await page.locator(PLAY_SELECTOR).click();
  await waitForAudioOnline(page);
  await page.locator(SIDEBAR_BUTTON_SELECTORS[0]).click();
}

test.describe("audio device lists", () => {
  test("the Input list grows past No Input when a capture device is enumerated", async ({ page, context }) => {
    // Belt and braces alongside the project's own launch flag: the measured
    // facts attribute the populated labels to that flag, but granting the
    // permission too can only help reveal them on this machine, never hurt.
    await context.grantPermissions(["microphone"]);
    await openAudioPage(page);
    const inputSelect = page.locator(AUDIO_INPUT_SELECT_SELECTOR);
    await expect(inputSelect).toBeVisible();
    // Device submission, and the option-list rebuild it drives, both land on
    // a later rendered UI frame than the clicks above, so this polls rather
    // than reading the option count once.
    await expect.poll(async () => inputSelect.locator("option").count()).toBeGreaterThan(1);
    const labels = await inputSelect.locator("option").allTextContents();
    expect(labels[0]).toBe(NO_INPUT_OPTION_LABEL);
    // Growth in option count alone would also be produced by a regression
    // that stopped filtering empty-labelled entries out, which is not the
    // property this test is for -- so at least one of the grown options must
    // itself carry a real (non-empty) label.
    expect(labels.slice(1).some((label) => label.length > 0)).toBe(true);
  });

  test("an unpermitted page presents no named entry", async ({ page }) => {
    // This project's own point is a browser that already exposes labelled
    // fake devices (see its comment in playwright.config.mjs), so the
    // permission-less page's three blank entries cannot be produced by this
    // project's ambient environment. The shape is supplied directly instead:
    // `enumerateDevices` is wrapped, before any app script runs, to report
    // entries whose `deviceId` and `label` are both empty -- the exact shape
    // measured on an unpermitted page -- so what is under test is the
    // filtering `BuildBrowserAudioSnapshot` applies (an empty-label entry
    // contributes no option) rather than this project's own permission
    // state.
    await page.addInitScript(() => {
      const mediaDevices = navigator.mediaDevices;
      const original = mediaDevices.enumerateDevices.bind(mediaDevices);
      mediaDevices.enumerateDevices = async () => {
        const real = await original();
        return real.map((device) => ({ deviceId: "", label: "", kind: device.kind }));
      };
    });
    await openAudioPage(page);
    const inputSelect = page.locator(AUDIO_INPUT_SELECT_SELECTOR);
    await expect(inputSelect).toBeVisible();
    // Deterministic given the wrapper above: however many blank entries the
    // real enumeration reports, all of them are filtered out, so the option
    // list never grows past the one default entry regardless of timing.
    await expect(inputSelect.locator("option")).toHaveCount(1);
    const options = await inputSelect.locator("option").allTextContents();
    expect(options).toEqual([NO_INPUT_OPTION_LABEL]);
  });

  test("listing does not start capture", async ({ page, context }) => {
    await context.grantPermissions(["microphone"]);
    await openAudioPage(page);
    const inputSelect = page.locator(AUDIO_INPUT_SELECT_SELECTOR);
    await expect.poll(async () => inputSelect.locator("option").count()).toBeGreaterThan(1);
    // Nothing above has touched the Input combo, so the selection the page
    // shows is still whatever `BuildBrowserAudioSnapshot` defaults to.
    await expect(inputSelect).toHaveValue(NO_INPUT_OPTION_ID);
    // The Audio page's own status line is the runtime's rendering of its own
    // `BrowserAudioInputStatus` (`BrowserAudioInputStatusText`,
    // BrowserAudioDevices.hpp). `NotRequested` is the only status that
    // renders this exact text, and every path that would move off it
    // (`AudioBridge.acquireInput`) is reached only from a device selection or
    // Retry Input -- neither of which happened here. This is the strongest
    // observation available from the DOM: it reads the native runtime's own
    // record of whether it ever tried to acquire a device, not just what the
    // combo currently shows selected. It does NOT intercept `getUserMedia`
    // itself, so it would not catch a hypothetical code path that called it
    // without also updating this status code first -- no such path exists in
    // the reviewed source, and the DOM exposes no more direct signal than
    // this line does.
    await expect(page.locator(AUDIO_STATUS_LINE_SELECTOR)).toContainText(CAPTURE_NOT_STARTED_TEXT);
  });
});
