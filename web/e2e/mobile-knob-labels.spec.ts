import { expect, test } from "@playwright/test";
import {
  KNOB_LABEL_ATT_1_2,
  KNOB_LABEL_CROSS_COUPLER,
  KNOB_LABEL_DRIVE,
  KNOB_LABEL_SELECTOR,
  KNOB_LABEL_VCO1,
  KNOBS_SELECTOR,
  MOBILE_KNOB_GRID_COLUMNS,
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

  test.beforeEach(async ({ page }) => {
    await page.goto("/");
  });

  test("three-column knob grid on load", async ({ page }) => {
    expect(await knobGridColumnCount(page)).toBe(MOBILE_KNOB_GRID_COLUMNS);
  });

  test("labels visible on load", async ({ page }) => {
    for (const label of [KNOB_LABEL_VCO1, KNOB_LABEL_CROSS_COUPLER, KNOB_LABEL_ATT_1_2]) {
      const el = labelBox(page, label);
      await expect(el).toHaveText(label);
      const box = await el.boundingBox();
      expect(box).not.toBeNull();
      expect(box!.height).toBeGreaterThan(0);
    }
  });

  test("labels update on page pill navigation", async ({ page }) => {
    await page.getByRole("button", { name: "Drive" }).click();
    await expect(labelBox(page, KNOB_LABEL_DRIVE)).toHaveText(KNOB_LABEL_DRIVE);
    await expect(labelBox(page, KNOB_LABEL_VCO1)).toHaveCount(0);
  });

  test("page pill navigation overrides stale WASM row names", async ({ page, context }) => {
    await installGetUserMediaMock(page);
    await context.grantPermissions(["microphone"]);
    await page.reload();
    await startSimAudio(page);
    await page.getByRole("button", { name: "Drive" }).click();
    await expect(labelBox(page, KNOB_LABEL_DRIVE)).toHaveText(KNOB_LABEL_DRIVE);
    await expect(labelBox(page, KNOB_LABEL_VCO1)).toHaveCount(0);
  });
});
