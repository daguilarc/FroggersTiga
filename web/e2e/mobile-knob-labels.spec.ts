import { expect, test } from "@playwright/test";
import {
  KNOB_LABEL_ATT_1_2,
  KNOB_LABEL_CROSS_COUPLER,
  KNOB_LABEL_SELECTOR,
  KNOB_LABEL_VCO1,
  KNOBS_SELECTOR,
} from "../test-shared/simSelectors.ts";
import { MOBILE_USE, installGetUserMediaMock, startSimAudio } from "./helpers";

function knobGridColumnCount(page: import("@playwright/test").Page): Promise<number> {
  return page.evaluate((selector) => {
    const cols = getComputedStyle(document.querySelector(selector)!).gridTemplateColumns;
    return cols.split(/\s+/).filter((track) => track.length > 0).length;
  }, KNOBS_SELECTOR);
}

function labelBox(page: import("@playwright/test").Page, text: string) {
  return page.locator(KNOB_LABEL_SELECTOR).filter({ hasText: text }).first();
}

test.describe("mobile knob labels", () => {
  test.use(MOBILE_USE);

  test.beforeEach(async ({ context, page }) => {
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.goto("/");
  });

  test("mobile knob grid uses two columns after Play", async ({ page }) => {
    await startSimAudio(page);
    expect(await knobGridColumnCount(page)).toBe(2);
  });

  test("mobile knob labels are visible on audio page", async ({ page }) => {
    await startSimAudio(page);
    expect(await knobGridColumnCount(page)).toBe(2);

    for (const label of [KNOB_LABEL_VCO1, KNOB_LABEL_CROSS_COUPLER]) {
      const el = labelBox(page, label);
      await expect(el).not.toHaveText("…");
      const box = await el.boundingBox();
      expect(box).not.toBeNull();
      expect(box!.height).toBeGreaterThan(0);
    }

    const pairAr = labelBox(page, KNOB_LABEL_ATT_1_2);
    await expect(pairAr).not.toHaveText("…");
    const pairArBox = await pairAr.boundingBox();
    expect(pairArBox).not.toBeNull();
    expect(pairArBox!.height).toBeGreaterThan(0);
  });
});
