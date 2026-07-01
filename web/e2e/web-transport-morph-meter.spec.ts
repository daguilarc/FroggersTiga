import { expect, test } from "@playwright/test";
import {
  KNOB_LABEL_SELECTOR,
  KNOB_LABEL_VCO1,
  KNOBS_SELECTOR,
  STATUS_SELECTOR,
} from "../test-shared/simSelectors.ts";
import { installGetUserMediaMock, startSimAudio } from "./helpers";

test.describe("web transport morph and external meter", () => {
  test("Rand waveforms updates VCO morph SVG after Play", async ({ page }) => {
    await page.goto("/");
    await startSimAudio(page);
    await expect(page.locator(STATUS_SELECTOR)).toContainText("Playing");

    const morphBefore = await page.locator(".vco-morph-btn").first().innerHTML();
    await page.getByRole("button", { name: "Rand waveforms" }).click();
    await expect
      .poll(async () => page.locator(".vco-morph-btn").first().innerHTML())
      .not.toBe(morphBefore);
  });

  test("external meter waiting before Play", async ({ page, context }) => {
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.goto("/");
    await page.getByRole("button", { name: /External Audio: Off/i }).click();
    await expect(page.locator("#external-meter-label")).toHaveText("Waiting for Play");
  });

  test("external meter active after Play and External", async ({ page, context }) => {
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.goto("/");
    await page.getByRole("button", { name: /External Audio: Off/i }).click();
    await startSimAudio(page);
    await expect(page.locator("#external-meter-label")).not.toHaveText("Waiting for Play");
    await expect(page.locator("#external-meter-label")).not.toHaveText("Off");
  });

  test("Audio page shows full Attack pair-AR label", async ({ page }) => {
    await page.goto("/");
    await expect(page.locator(KNOB_LABEL_SELECTOR).filter({ hasText: "Attack 1+2" })).toHaveCount(1);
    await expect(page.locator(`${KNOBS_SELECTOR} .knob-col`)).toHaveCount(12);
    await expect(page.locator(KNOB_LABEL_SELECTOR).filter({ hasText: KNOB_LABEL_VCO1 })).toHaveCount(1);
  });
});
