import { expect, test } from "@playwright/test";
import {
  KNOB_LABEL_ATT_1_2,
  KNOB_LABEL_CROSS_COUPLER,
  KNOB_LABEL_DRIVE,
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

  test.describe("static labels", () => {
    test.beforeEach(async ({ page }) => {
      await page.goto("/");
    });

    test("labels visible on load without Play", async ({ page }) => {
      expect(await knobGridColumnCount(page)).toBe(2);
      for (const label of [KNOB_LABEL_VCO1, KNOB_LABEL_CROSS_COUPLER, KNOB_LABEL_ATT_1_2]) {
        const el = labelBox(page, label);
        await expect(el).toHaveText(label);
        const box = await el.boundingBox();
        expect(box).not.toBeNull();
        expect(box!.height).toBeGreaterThan(0);
      }
    });

    test("labels update when changing page", async ({ page }) => {
      await page.getByRole("button", { name: "Drive" }).click();
      const driveLabel = page.locator(KNOB_LABEL_SELECTOR).filter({ hasText: "Drive" }).first();
      await expect(driveLabel).toHaveText("Drive");
      await expect(labelBox(page, KNOB_LABEL_VCO1)).toHaveCount(0);
    });
  });

  test.describe("after Play", () => {
    test.beforeEach(async ({ context, page }) => {
      await installGetUserMediaMock(page);
      await context.grantPermissions(["microphone"]);
      await page.goto("/");
    });

    test("knob grid uses two columns", async ({ page }) => {
      await startSimAudio(page);
      expect(await knobGridColumnCount(page)).toBe(2);
    });

    test("labels remain visible on audio page", async ({ page }) => {
      await startSimAudio(page);
      expect(await knobGridColumnCount(page)).toBe(2);

      for (const label of [KNOB_LABEL_VCO1, KNOB_LABEL_CROSS_COUPLER, KNOB_LABEL_ATT_1_2]) {
        const el = labelBox(page, label);
        await expect(el).toHaveText(label);
        const box = await el.boundingBox();
        expect(box).not.toBeNull();
        expect(box!.height).toBeGreaterThan(0);
      }
    });

    test("labels switch to static page table when page changes while playing", async ({ page }) => {
      await startSimAudio(page);
      await page.getByRole("button", { name: "Drive" }).click();
      await expect(labelBox(page, KNOB_LABEL_DRIVE)).toHaveText(KNOB_LABEL_DRIVE);
      await expect(labelBox(page, KNOB_LABEL_VCO1)).toHaveCount(0);
    });
  });
});
