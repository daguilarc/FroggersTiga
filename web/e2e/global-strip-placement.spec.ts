import { expect, test } from "@playwright/test";
import { DESKTOP_USE, MOBILE_USE } from "./helpers";

async function assertGlobalStripPlacement(page: import("@playwright/test").Page): Promise<void> {
  const layout = await page.evaluate(() => {
    const strip = document.querySelector(".global-strip")!.getBoundingClientRect();
    const modBay = document.getElementById("mod-bay")!.getBoundingClientRect();
    const externalMidi = document.getElementById("external-midi-btn")!.getBoundingClientRect();
    return {
      stripTop: strip.top,
      modBayTop: modBay.top,
      externalMidiBottom: externalMidi.bottom,
    };
  });
  expect(layout.stripTop).toBeLessThan(layout.modBayTop);
  expect(layout.stripTop).toBeGreaterThan(layout.externalMidiBottom);
}

test.describe("global strip placement", () => {
  test("mobile: strip under External MIDI and above mod bay", async ({ page }) => {
    await page.setViewportSize(MOBILE_USE.viewport);
    await page.goto("/");
    await assertGlobalStripPlacement(page);
    await expect(page.locator(".global-strip")).toHaveCount(1);
  });

  test("desktop: strip under External MIDI and above mod bay", async ({ page }) => {
    await page.setViewportSize(DESKTOP_USE.viewport);
    await page.goto("/");
    await assertGlobalStripPlacement(page);
    await expect(page.locator(".global-strip")).toHaveCount(1);
  });
});
