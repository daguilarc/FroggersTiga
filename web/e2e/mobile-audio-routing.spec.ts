import { expect, test } from "@playwright/test";
import {
  EXTERNAL_OFF_LABEL,
  EXTERNAL_ON_LABEL,
  IOS_EXTERNAL_HINT_EARPIECE,
  IOS_EXTERNAL_HINT_HEADPHONES,
  IOS_EXTERNAL_HINT_SELECTOR,
  STATUS_HINT_EXTERNAL_ON,
  STATUS_SELECTOR,
  STOP_LABEL,
  SUBTITLE_SELECTOR,
  SUBTITLE_TEXT,
} from "../test-shared/simSelectors.ts";
import {
  DESKTOP_USE,
  MOBILE_USE,
  installAudioSessionSpy,
  installGetUserMediaMock,
  readAudioSessionLog,
  startSimAudio,
} from "./helpers";

test.describe("mobile audio routing UX", () => {
  test.use(MOBILE_USE);

  test.beforeEach(async ({ context, page }) => {
    await installAudioSessionSpy(page);
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.goto("/");
  });

  test("subtitle distinguishes Play from External", async ({ page }) => {
    await expect(page.locator(SUBTITLE_SELECTOR)).toHaveText(SUBTITLE_TEXT);
  });

  test("mobile Play with External off sets playback session", async ({ page }) => {
    await startSimAudio(page);
    const log = await readAudioSessionLog(page);
    expect(log).toContain("playback");
  });

  test("mobile External on runs reset then play-and-record session sequence", async ({ page }) => {
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await expect(page.getByRole("button", { name: EXTERNAL_ON_LABEL })).toBeVisible();
    const log = await readAudioSessionLog(page);
    expect(log.indexOf("auto")).toBeLessThan(log.lastIndexOf("play-and-record"));
    expect(log).toContain("auto");
    expect(log).toContain("play-and-record");
  });

  test("mobile External off restores playback then auto session", async ({ page }) => {
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await page.getByRole("button", { name: EXTERNAL_ON_LABEL }).click();
    const log = await readAudioSessionLog(page);
    expect(log.slice(-2)).toEqual(["playback", "auto"]);
  });

  test("iOS static hint visible when External is on", async ({ page }) => {
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await expect(page.getByRole("button", { name: EXTERNAL_ON_LABEL })).toBeVisible();
    const hint = page.locator(IOS_EXTERNAL_HINT_SELECTOR);
    await expect(hint).toBeVisible();
    await expect(hint).toContainText(IOS_EXTERNAL_HINT_EARPIECE);
    await expect(hint).toContainText(IOS_EXTERNAL_HINT_HEADPHONES);
    await expect(page.locator(STATUS_SELECTOR)).not.toContainText(IOS_EXTERNAL_HINT_EARPIECE);
  });

  test("iOS static hint hidden when External is off", async ({ page }) => {
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await page.getByRole("button", { name: EXTERNAL_ON_LABEL }).click();
    await expect(page.locator(IOS_EXTERNAL_HINT_SELECTOR)).toBeVisible();
    await page.getByRole("button", { name: EXTERNAL_ON_LABEL }).click();
    await expect(page.locator(IOS_EXTERNAL_HINT_SELECTOR)).toBeHidden();
  });

  test("mobile Play with External on keeps transport status separate from iOS hint", async ({ page }) => {
    await startSimAudio(page);
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await expect(page.locator(STATUS_SELECTOR)).toContainText(STATUS_HINT_EXTERNAL_ON);
    await expect(page.locator(IOS_EXTERNAL_HINT_SELECTOR)).toBeVisible();
  });

  test("Stop after External clears session via externalOff sequence", async ({ page }) => {
    await startSimAudio(page);
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    await page.getByRole("button", { name: STOP_LABEL }).click();
    const log = await readAudioSessionLog(page);
    expect(log.filter((entry) => entry === "playback").length).toBeGreaterThanOrEqual(2);
    expect(log[log.length - 1]).toBe("auto");
  });
});

test.describe("desktop audio session guard", () => {
  test.use(DESKTOP_USE);

  test.beforeEach(async ({ context, page }) => {
    await installAudioSessionSpy(page);
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.goto("/");
  });

  test("desktop Play does not mutate audioSession", async ({ page }) => {
    await startSimAudio(page);
    expect(await readAudioSessionLog(page)).toEqual([]);
  });

  test("desktop External on does not mutate audioSession", async ({ page }) => {
    await page.getByRole("button", { name: EXTERNAL_OFF_LABEL }).click();
    expect(await readAudioSessionLog(page)).toEqual([]);
  });
});
