import { expect, test } from "@playwright/test";
import { HOST_PAGE_NAMES } from "../src/hostDisplay.generated";

test.describe("host page pill order", () => {
  test("web pills follow ParamDisplayNames host page order", async ({ page }) => {
    await page.goto("/");
    const pills = page.locator(".page-pill");
    await expect(pills).toHaveCount(HOST_PAGE_NAMES.length);

    for (let i = 0; i < HOST_PAGE_NAMES.length; i++) {
      await expect(pills.nth(i)).toHaveText(HOST_PAGE_NAMES[i]!);
    }

    expect(HOST_PAGE_NAMES[2]).toBe("Reverb");
    expect(HOST_PAGE_NAMES[3]).toBe("Filter");
  });
});
