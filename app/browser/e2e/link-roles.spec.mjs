// Link-role regression: site
// links carry the legacy roles under the new repository references. Runs under BOTH the
// mobile and desktop projects (playwright.config.mjs) since link presence
// and target correctness do not depend on viewport. Never starts audio --
// no control is clicked here at all.
import { expect, test } from "@playwright/test";

const ROLES = [
  { role: "download", hrefPrefix: "https://github.com/daguilarc/frogg3rs/releases/latest" },
  { role: "license", hrefPrefix: "https://github.com/daguilarc/frogg3rs/blob/main/LICENSE" },
  { role: "manual", hrefPrefix: "https://github.com/daguilarc/frogg3rs/blob/main/MANUAL.md" },
];

test.describe("site link roles", () => {
  test.beforeEach(async ({ page }) => {
    await page.goto("/");
  });

  for (const { role, hrefPrefix } of ROLES) {
    test(`${role} link is present and resolves to the new origin`, async ({ page }) => {
      const link = page.locator(`[data-site-link="${role}"]`);
      await expect(link).toHaveCount(1);
      await expect(link).toHaveAttribute("href", hrefPrefix);
    });
  }

  test("no link targets the old repository name", async ({ page }) => {
    const hrefs = await page.locator("a[href]").evaluateAll((anchors) => anchors.map((a) => a.getAttribute("href")));
    expect(hrefs.length).toBeGreaterThan(0);
    for (const href of hrefs) {
      expect(href).not.toContain("FroggersTiga");
    }
  });

  test("every legacy role (download/license/manual) is present exactly once", async ({ page }) => {
    for (const { role } of ROLES) {
      await expect(page.locator(`[data-site-link="${role}"]`)).toHaveCount(1);
    }
  });
});
