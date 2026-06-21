import { expect, test } from "@playwright/test";
import { modLedDisplayBrightness } from "../src/hostDisplay.generated";

test.describe("marbles mod LED level", () => {
  test("generated curve golden points", () => {
    expect(modLedDisplayBrightness(-0.1, true)).toBe(0);
    expect(modLedDisplayBrightness(0, true)).toBe(0);
    expect(modLedDisplayBrightness(0.275, true)).toBeCloseTo(0.25, 5);
    expect(modLedDisplayBrightness(0.55, true)).toBe(1);
    expect(modLedDisplayBrightness(1.2, true)).toBe(1);
    expect(modLedDisplayBrightness(0.8, false)).toBe(0);
  });

  test("web mod bay exposes proportional brightness", async ({ page }) => {
    await page.goto("/");
    const leds = page.locator(".mod-led");
    await expect(leds).toHaveCount(2);

    for (let i = 0; i < 2; i++) {
      const led = leds.nth(i);
      await expect(led).toHaveAttribute("data-brightness", "0.000");
      await expect(led).not.toHaveAttribute("data-on");
    }

    await page.getByRole("button", { name: "Play" }).click();
    await expect(page.locator("#status")).toContainText("Playing", { timeout: 60_000 });

    for (let i = 0; i < 8; i++) {
      await page.getByRole("button", { name: "Rand Resample" }).click();
      await page.waitForTimeout(150);
    }

    await expect
      .poll(async () => {
        const brightnesses = await page.evaluate(() =>
          Array.from(document.querySelectorAll(".mod-led")).map((el) =>
            Number.parseFloat((el as HTMLElement).dataset.brightness ?? "0")
          )
        );
        return Math.max(...brightnesses);
      }, { timeout: 15_000 })
      .toBeGreaterThan(0);

    const parity = await page.evaluate(() =>
      Array.from(document.querySelectorAll(".mod-led")).every((el) => {
        const html = el as HTMLElement;
        const data = html.dataset.brightness ?? "0";
        const css = html.style.getPropertyValue("--mod-led-brightness").trim();
        return data === css;
      })
    );
    expect(parity).toBe(true);
  });
});
