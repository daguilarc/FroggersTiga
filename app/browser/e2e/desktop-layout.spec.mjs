// Desktop-emulated layout sanity. Runs only under the
// "desktop" project (playwright.config.mjs testMatch). Proves the app
// actually boots and renders its real UI frame over the direct catalog
// load path (site-boot.mjs) with no picker/launcher UI ever shown, and
// that the surface renders at a sane, non-degenerate size on a wide
// viewport. Never starts audio -- no control is clicked here.
import { expect, test } from "@playwright/test";
import {
  ENCODER_ROW_SELECTORS,
  LEFT_BLOCK_SELECTOR,
  RIGHT_BLOCK_SELECTOR,
  SIDEBAR_SELECTOR,
  SURFACE_ROOT_SELECTOR,
  encoderGridBoundingBox,
  expectedStackedWidth,
  verticalOverlapPx,
  waitForSurfaceReady,
  wireWidth,
} from "./helpers.mjs";

test.describe("desktop layout sanity", () => {
  test.beforeEach(async ({ page }) => {
    await page.goto("/");
    await waitForSurfaceReady(page);
  });

  test("no catalog-browser picker is ever shown", async ({ page }) => {
    // SheafPatchLauncher (the picker UI) always renders a
    // `.synth-launcher` shell with a `.synth-launcher__apps` list --
    // site-boot.mjs never imports or constructs that class at all, so
    // this must stay completely absent, at every point in the page
    // lifecycle, not just after boot.
    await expect(page.locator(".synth-launcher")).toHaveCount(0);
  });

  test("the surface renders at a sane, non-degenerate size", async ({ page }) => {
    const root = page.locator(SURFACE_ROOT_SELECTOR);
    await expect(root).toBeVisible();
    const box = await root.boundingBox();
    expect(box).not.toBeNull();
    expect(box.width).toBeGreaterThan(200);
    expect(box.height).toBeGreaterThan(200);
  });

  test("the encoder grid and the left chrome block both render", async ({ page }) => {
    const grid = await encoderGridBoundingBox(page);
    expect(grid).not.toBeNull();
    expect(grid.width).toBeGreaterThan(0);
    expect(grid.height).toBeGreaterThan(0);
    await expect(page.locator(LEFT_BLOCK_SELECTOR)).toBeVisible();
  });

  test("all four encoder-grid rows render at equal width (one coherent grid)", async ({ page }) => {
    const boxes = await Promise.all(ENCODER_ROW_SELECTORS.map((selector) => page.locator(selector).boundingBox()));
    for (const box of boxes) expect(box).not.toBeNull();
    const widths = boxes.map((box) => Math.round(box.width));
    expect(new Set(widths).size).toBe(1);
  });

  test("no audio starts on load", async ({ page }) => {
    // SynthBrowserApp only starts audio from a user gesture dispatched
    // through the app's own UI (main.ts's BrowserUiBackend dispatch
    // wiring); this suite never clicks anything, so the root's own status
    // dataset must never reach a "running"/"audio:online" state.
    const status = await page.locator("#synth-root").getAttribute("data-synth-status");
    expect(status ?? "").not.toContain("audio:online");
  });

  // mobile-stack.mjs re-applies from a ResizeObserver on
  // the same host element Sheaf's own fitSurface observes
  // (mobile-stack.mjs's own header comment has the full reasoning), not
  // window "resize" alone -- this drives an actual live resize (starting
  // wide, matching this describe block's own project) rather than a fresh
  // page load at a narrow viewport, so it exercises that observer path
  // specifically, in both directions.
  test("a live resize from wide to narrow engages the stack, and back restores wide layout", async ({ page }) => {
    const gridBefore = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const leftBefore = await page.locator(LEFT_BLOCK_SELECTOR).boundingBox();

    await page.setViewportSize({ width: 390, height: 844 });
    // Polls on BOTH the grid's X POSITION (crossed back toward 0) AND its
    // WIDTH (reached the actual 390 target), not either alone -- both
    // turned out to have their own false-positive window, found via this
    // exact test flaking (~40-45% of runs at each single-signal attempt)
    // and traced with a MutationObserver-based instrumentation pass (kept
    // out of the committed test):
    //  - width alone: at WIDE, the grid's own NATIVE (un-stacked) rendered
    //    width is already comfortably above the "0.9x narrow viewport"
    //    threshold (it is a large block regardless of layout), so
    //    `page.setViewportSize()` resolving before the page's own resize
    //    reaction has even run once could read the STILL-WIDE state and
    //    mistake it for "already stacked".
    //  - x alone: `mount.clientWidth` itself was observed passing through
    //    transient intermediate values while an active CDP-driven resize
    //    settles (a real browser/CDP characteristic during live resize,
    //    not a mobile-stack.mjs defect -- production self-corrects every
    //    ~33ms frame regardless), and this shell's per-block math makes
    //    the grid's rendered x converge to ~0 for ANY narrow-mode
    //    application, even one computed from a not-yet-final width --
    //    so x alone can go true before the width has actually settled.
    // Requiring both together has no known false-positive window: WIDE
    // fails the x check, and an unsettled-width narrow frame fails the
    // width check (per this file's own comment on `applyStackedTransform`
    // in mobile-stack.mjs, the grid's rendered width always exactly
    // equals whatever `mount.clientWidth` was at that specific frame, so
    // it cannot read >=351 without the width truly having reached ~390).
    // A THIRD signal, and the one that is specific to a live resize: the
    // shell reacts to the new width immediately, but the SURFACE only
    // switches to its narrow topology once the browser-narrow action the
    // shell dispatches has reached the wasm app and it has emitted another
    // frame. Between those two the blocks are stacked while still carrying
    // WIDE design widths, and anything derived from a wire width -- the
    // shared scale below is -- reads a value that is about to change.
    // Equal chrome and grid wire widths is the surface-side half of
    // "settled": it is true only of the narrow topology
    // (FroggersCellMap::kLeftBlockWeightNarrow).
    await expect
      .poll(
        async () => {
          const box = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
          const gridWire = await wireWidth(page, RIGHT_BLOCK_SELECTOR);
          const chromeWire = await wireWidth(page, LEFT_BLOCK_SELECTOR);
          return box.x < 50 && box.width >= 390 * 0.9 && Math.abs(gridWire - chromeWire) < 1;
        },
        { timeout: 5_000 },
      )
      .toBe(true);
    const gridNarrow = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const sidebarNarrow = await page.locator(SIDEBAR_SELECTOR).boundingBox();
    expect(verticalOverlapPx(sidebarNarrow, gridNarrow)).toBe(0);
    // The sidebar shares the grid's scale rather than being independently
    // stretched to full width too (see mobile-stack.mjs's
    // own comment) -- expect it at the grid's shared scale (viewport width
    // over the grid's own live wire width, read live via
    // page.viewportSize() -- already 390 here, set above) times the
    // sidebar's own live wire width, not at the viewport width.
    const expectedSidebarWidth = await expectedStackedWidth(page, SIDEBAR_SELECTOR);
    expect(Math.abs(sidebarNarrow.width - expectedSidebarWidth)).toBeLessThan(1.5);

    await page.setViewportSize({ width: 1280, height: 800 });
    // Wide-mode settle signal: the mount's `height` is Sheaf's alone
    // (browser/src/ui.ts's fitSurface) at every width, and the shell never
    // writes it. What the shell writes when narrow, and must drop when
    // wide, is a `min-height` reserving the stacked total; a stale
    // reservation left behind would hold every wide viewport open to a
    // phone-sized page. So "settled wide" is: a non-empty px height
    // authored by Sheaf, no reservation from the shell, AND the stacked
    // blocks' own transforms released.
    await expect
      .poll(
        async () =>
          page.locator(RIGHT_BLOCK_SELECTOR).evaluate((el) => el.style.transform),
        { timeout: 5_000 },
      )
      .toBe("");
    await expect
      .poll(async () => page.locator("#synth-root").evaluate((el) => el.style.height), { timeout: 5_000 })
      .not.toBe("");
    await expect
      .poll(async () => page.locator("#synth-root").evaluate((el) => el.style.minHeight), { timeout: 5_000 })
      .toBe("");
    // mobile-stack.mjs's own follow-up burst (scheduleApply, FOLLOW_UP_TICKS)
    // keeps re-asserting for a short span after the resize to out-last a
    // possible race with ui.ts's own internal resize-triggered fitSurface()
    // call (its own comment has the full reasoning); give that burst room to
    // fully settle before reading final state.
    await page.waitForTimeout(300);
    const gridAfter = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const leftAfter = await page.locator(LEFT_BLOCK_SELECTOR).boundingBox();
    const mountHeight = await page.locator("#synth-root").evaluate((el) => el.style.height);
    const leftTransform = await page.locator(LEFT_BLOCK_SELECTOR).evaluate((el) => el.style.transform);
    // Restored wide layout matches the original wide layout exactly (no
    // leftover transform from the narrow pass), and the mount is sized by
    // Sheaf again rather than by this shell or by nobody.
    expect(gridAfter).toEqual(gridBefore);
    expect(leftAfter).toEqual(leftBefore);
    expect(leftTransform).toBe("");
    expect(mountHeight).toMatch(/^\d+(\.\d+)?px$/);
  });
});
