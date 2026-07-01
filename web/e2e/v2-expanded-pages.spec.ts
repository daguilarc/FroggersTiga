import { expect, test } from "@playwright/test";
import {
  EXPANDED_PAGE_KNOB_COUNT,
  GLOBAL_CRUNCHY_LABEL,
  GLOBAL_CRUNCHY_SELECTOR,
  KNOB_LABEL_BIAS,
  KNOB_LABEL_BLEND,
  KNOB_LABEL_COLOR,
  KNOB_LABEL_COMB_PEAK,
  KNOB_LABEL_HALO,
  KNOB_LABEL_PHASE,
  KNOB_LABEL_SCOOP,
  KNOB_LABEL_SPREAD,
  KNOB_LABEL_SELECTOR,
  KNOB_LABEL_VCO1,
  KNOBS_SELECTOR,
} from "../test-shared/simSelectors.ts";
import { DESKTOP_USE, MOBILE_USE } from "./helpers";

function labelBox(page: import("@playwright/test").Page, text: string) {
  return page.locator(KNOB_LABEL_SELECTOR).filter({ hasText: text }).first();
}

async function expectExpandedPageKnobCount(page: import("@playwright/test").Page): Promise<void> {
  await expect(page.locator(`${KNOBS_SELECTOR} .knob-col`)).toHaveCount(EXPANDED_PAGE_KNOB_COUNT);
}

test.describe("v2 expanded module pages", () => {
  test("desktop: global Crunchy rotary visible in global strip", async ({ page }) => {
    await page.setViewportSize(DESKTOP_USE.viewport);
    await page.goto("/");
    await expect(page.locator(GLOBAL_CRUNCHY_SELECTOR)).toBeVisible();
    await expect(page.locator(GLOBAL_CRUNCHY_SELECTOR)).toContainText(GLOBAL_CRUNCHY_LABEL);
  });

  test("mobile: global Crunchy rotary visible in global strip", async ({ page }) => {
    await page.setViewportSize(MOBILE_USE.viewport);
    await page.goto("/");
    await expect(page.locator(GLOBAL_CRUNCHY_SELECTOR)).toBeVisible();
    await expect(page.locator(GLOBAL_CRUNCHY_SELECTOR)).toContainText(GLOBAL_CRUNCHY_LABEL);
  });

  test("Random page shows ten knobs including Spread and Bias", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Random S&H" }).click();
    await expectExpandedPageKnobCount(page);
    await expect(labelBox(page, KNOB_LABEL_SPREAD)).toHaveText(KNOB_LABEL_SPREAD);
    await expect(labelBox(page, KNOB_LABEL_BIAS)).toHaveText(KNOB_LABEL_BIAS);
  });

  test("Filter page shows Comb/Peak and Scoop labels", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Filter" }).click();
    await expectExpandedPageKnobCount(page);
    await expect(labelBox(page, KNOB_LABEL_COMB_PEAK)).toHaveText(KNOB_LABEL_COMB_PEAK);
    await expect(labelBox(page, KNOB_LABEL_SCOOP)).toHaveText(KNOB_LABEL_SCOOP);
  });

  test("Drive page shows Blend and Phase labels", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Drive" }).click();
    await expectExpandedPageKnobCount(page);
    await expect(labelBox(page, KNOB_LABEL_BLEND)).toHaveText(KNOB_LABEL_BLEND);
    await expect(labelBox(page, KNOB_LABEL_PHASE)).toHaveText(KNOB_LABEL_PHASE);
  });

  test("Delay page shows Color and Halo labels", async ({ page }) => {
    await page.goto("/");
    await page.getByRole("button", { name: "Delay" }).click();
    await expectExpandedPageKnobCount(page);
    await expect(labelBox(page, KNOB_LABEL_COLOR)).toHaveText(KNOB_LABEL_COLOR);
    await expect(labelBox(page, KNOB_LABEL_HALO)).toHaveText(KNOB_LABEL_HALO);
  });

  test("Audio page keeps eight core knobs plus pair-AR", async ({ page }) => {
    await page.goto("/");
    await expect(page.locator(`${KNOBS_SELECTOR} .knob-col`)).toHaveCount(12);
    await expect(labelBox(page, KNOB_LABEL_VCO1)).toHaveText(KNOB_LABEL_VCO1);
    await expect(labelBox(page, "Attack 1+2")).toHaveText("Attack 1+2");
  });
});
