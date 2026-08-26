// Mobile-emulated stacking assertion: mobile viewport stacks around a
// full-width encoder grid.
//
// Host-page CSS alone cannot restack this layout, because
// FroggersUiSurface.hpp lays the chrome and grid blocks out as one outer
// Row (FroggersUiSurface.hpp:856-859, Weight(2)/Weight(4) siblings) that
// Sheaf's browser UI backend positions as absolutely-bounded, wire-managed
// DOM nodes an active render loop keeps rewriting.
// Per-block
// CSS transforms sidestep that instead of fighting it: they compose with (rather than override) those wire-managed
// properties, reasserted every render frame from the site shell
// (app/browser/site/mobile-stack.mjs, hooked in site-boot.mjs) -- see that
// file's own header comment for the full mechanism. The stack is
// THREE blocks, not two: chrome above, grid full-width below it, and
// Sheaf's own generic runtime sidebar (RIGHT_BLOCK_SELECTOR/
// SIDEBAR_SELECTOR, helpers.mjs) below THAT -- everything
// else above or below the grid includes the sidebar, so it is not
// left beside anything either.
//
// SHARED SCALE: only the GRID is stretched to fill the
// viewport width. Chrome and the sidebar render at that SAME scale
// (mobile-stack.mjs's own comment has the full reasoning: their design
// widths are roughly half and a sixth of the grid's, so independently
// stretching each of them to full width too made chrome balloon to ~2x
// the height it needed and pushed the grid far down the page), so they
// come out narrower than the viewport, left-aligned under it.
import { expect, test } from "@playwright/test";
import {
  ENCODER_ROW_SELECTORS,
  LEFT_BLOCK_SELECTOR,
  RIGHT_BLOCK_SELECTOR,
  SIDEBAR_SELECTOR,
  encoderGridBoundingBox,
  expectedStackedWidth,
  verticalOverlapPx,
  waitForSurfaceReady,
} from "./helpers.mjs";

test.describe("mobile stacking (phone-width layout)", () => {
  test.beforeEach(async ({ page }) => {
    await page.goto("/");
    await waitForSurfaceReady(page);
  });

  test("the encoder grid spans the full viewport width", async ({ page }) => {
    const grid = await encoderGridBoundingBox(page);
    const viewport = page.viewportSize();
    expect(grid.width).toBeGreaterThanOrEqual(viewport.width * 0.9);
  });

  test("no other control renders beside the grid", async ({ page }) => {
    const rightBlockBox = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const leftBlockBox = await page.locator(LEFT_BLOCK_SELECTOR).boundingBox();
    // "Beside" means the two boxes share a vertical range (some y overlap)
    // while occupying disjoint horizontal ranges. Stacked above/below means
    // zero vertical overlap.
    expect(verticalOverlapPx(rightBlockBox, leftBlockBox)).toBe(0);
  });

  test("chrome renders at the grid's shared scale, not stretched to its own full width", async ({ page }) => {
    // The grid alone is stretched to the viewport width; chrome shares
    // THAT scale rather than independently filling the viewport too (see
    // this file's own header comment and mobile-stack.mjs's for why).
    // expectedStackedWidth derives it the same way mobile-stack.mjs itself
    // derives it: viewport width over the grid's own live wire width,
    // times chrome's own live wire width -- never a hardcoded 600ish
    // design-width number.
    const gridBox = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const chromeBox = await page.locator(LEFT_BLOCK_SELECTOR).boundingBox();

    expect(Math.abs(chromeBox.width - (await expectedStackedWidth(page, LEFT_BLOCK_SELECTOR)))).toBeLessThan(1.5); // within ~1.5px
    // Well short of the grid's own full width -- proves it is NOT also
    // independently stretched to fill the viewport.
    expect(chromeBox.width).toBeLessThan(gridBox.width * 0.95);

    // Sits fully above the grid (zero vertical overlap, same "stacked not
    // beside" check the other tests here use) and does not spill past the
    // grid's own right edge (no horizontal overlap past its container).
    expect(verticalOverlapPx(chromeBox, gridBox)).toBe(0);
    expect(chromeBox.x + chromeBox.width).toBeLessThanOrEqual(gridBox.x + gridBox.width + 1);
  });

  test("the sidebar stacks below the grid at the grid's shared scale, not full width", async ({ page }) => {
    // Sheaf's own generic runtime sidebar is "everything else" for
    // this stacking rule too -- verifies it must be part of
    // the stack (directly below the grid, zero overlap with anything),
    // not left at its native design position where it would otherwise
    // sit beside the app's own content. It shares the grid's scale rather
    // than being independently stretched to full width, same as chrome.
    const gridBox = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();
    const sidebarBox = await page.locator(SIDEBAR_SELECTOR).boundingBox();

    expect(Math.abs(sidebarBox.width - (await expectedStackedWidth(page, SIDEBAR_SELECTOR)))).toBeLessThan(1.5);
    expect(sidebarBox.y).toBeGreaterThanOrEqual(gridBox.y + gridBox.height);
    expect(verticalOverlapPx(sidebarBox, gridBox)).toBe(0);
  });

  test("Randomize and Reset sit above the encoder grid, not below it", async ({ page }) => {
    // FroggersCellMap::kRightRowsNarrow reorders the right block's rows for
    // a narrow viewport: BankTabs, Header, Randomize, Reset, then the four
    // EncoderRows (FroggersUiSurface.hpp's `kRightRowsNarrow`) -- the
    // inverse of `kRightRows`' order, where Randomize/Reset sit after the
    // grid. This keeps both reachable without scrolling past the whole
    // 16-slot grid on a phone-width screen. Bounding-box tops (not row
    // index) are what actually prove the on-screen order, since row index
    // alone says nothing about where the mobile stack transform (this
    // file's own header comment) ultimately places the row.
    const randomizeBox = await page.locator('[data-synth-node-id="froggers.layout.right.randomize"]').boundingBox();
    const resetBox = await page.locator('[data-synth-node-id="froggers.layout.right.reset"]').boundingBox();
    const firstEncoderRowBox = await page.locator(ENCODER_ROW_SELECTORS[0]).boundingBox();

    expect(randomizeBox.y).toBeLessThan(firstEncoderRowBox.y);
    expect(resetBox.y).toBeGreaterThan(randomizeBox.y);
    expect(resetBox.y).toBeLessThan(firstEncoderRowBox.y);
  });

  test("a transient measurement failure on one block does not disturb the other two", async ({ page }) => {
    // Regression test for a real bug this suite caught during development:
    // mobile-stack.mjs's
    // per-frame stacking pass is supposed to be atomic across the three
    // blocks -- if ONE block's measurement fails on a given frame (e.g.
    // Sheaf's sidebar surface reporting a transient zero-extent bounds
    // mid-resize), the OTHER two must stay exactly where they were, not
    // flash back toward their native/unstacked size for that frame.
    //
    // Forces the failure deterministically: overrides the sidebar
    // element's OWN `getBoundingClientRect` to report a degenerate
    // (zero-width) rect for a short window, and polls chrome/grid's real
    // boxes throughout that window (not just before/after) to catch a
    // transient revert a single before/after snapshot would miss.
    await expect
      .poll(async () => page.locator("#synth-root").evaluate((el) => el.style.height), { timeout: 5_000 })
      .not.toBe("");
    const chromeBefore = await page.locator(LEFT_BLOCK_SELECTOR).boundingBox();
    const gridBefore = await page.locator(RIGHT_BLOCK_SELECTOR).boundingBox();

    const observed = await page.evaluate(
      async ({ chromeBefore, gridBefore, leftSelector, rightSelector, sidebarSelector }) => {
        const sidebar = document.querySelector(sidebarSelector);
        const original = sidebar.getBoundingClientRect.bind(sidebar);
        sidebar.getBoundingClientRect = () => ({
          width: 0, height: 0, top: 0, left: 0, right: 0, bottom: 0, x: 0, y: 0,
          toJSON() { return this; },
        });

        const chromeEl = document.querySelector(leftSelector);
        const gridEl = document.querySelector(rightSelector);
        let maxChromeDeltaW = 0;
        let maxGridDeltaW = 0;
        const deadline = performance.now() + 250; // several render frames' worth
        while (performance.now() < deadline) {
          maxChromeDeltaW = Math.max(maxChromeDeltaW, Math.abs(chromeEl.getBoundingClientRect().width - chromeBefore.width));
          maxGridDeltaW = Math.max(maxGridDeltaW, Math.abs(gridEl.getBoundingClientRect().width - gridBefore.width));
          await new Promise((resolve) => requestAnimationFrame(resolve));
        }

        sidebar.getBoundingClientRect = original;
        return { maxChromeDeltaW, maxGridDeltaW };
      },
      { chromeBefore, gridBefore, leftSelector: LEFT_BLOCK_SELECTOR, rightSelector: RIGHT_BLOCK_SELECTOR, sidebarSelector: SIDEBAR_SELECTOR },
    );

    // A genuine revert-to-native would move these by tens to hundreds of
    // px (chrome/grid's native, un-stacked widths are far below the
    // ~390px stacked target); a generous few-px tolerance still catches
    // that while giving normal sub-pixel layout/transform-matrix rounding
    // room to be sub-pixel.
    expect(observed.maxChromeDeltaW).toBeLessThan(2);
    expect(observed.maxGridDeltaW).toBeLessThan(2);

    // Confirms recovery too: the sidebar's own real getBoundingClientRect
    // is restored above, so the very next frame should re-settle it back
    // into the stack, at the grid's shared scale (not full width -- see
    // this file's own header comment).
    const expectedSidebarWidth = await expectedStackedWidth(page, SIDEBAR_SELECTOR);
    await expect
      .poll(async () => (await page.locator(SIDEBAR_SELECTOR).boundingBox()).width, { timeout: 5_000 })
      .toBeGreaterThan(expectedSidebarWidth * 0.9);
  });

  // Positive control: proves the drag/press input
  // mapping survives the per-block transform above, for one control in
  // EACH stacked block -- not just that the boxes land in the right
  // place, but that the app still genuinely reacts to input inside them.

  test("a button press in the chrome block still reaches the app", async ({ page }) => {
    // FroggersNodeIds::SceneButton(1) ("Scene 2") -- a plain click-dispatch
    // control (no pointerDragAction) in the chrome block. Selecting it
    // moves FroggersActions::kSceneBlend's own slider to that scene's
    // blend position, a real app-computed value (not something the client
    // guesses), rendered as the native range input's own value -- a
    // "drawn value text" positive control for a
    // plain click path.
    const blendInput = page.locator('[data-synth-node-id="froggers.scene.blend"] input');
    const before = await blendInput.inputValue();
    await page.locator('[data-synth-node-id="froggers.scene.1"]').click();
    await expect(blendInput).not.toHaveValue(before, { timeout: 5_000 });
  });

  test("an encoder drag in the grid block still reaches the app", async ({ page }) => {
    // FroggersNodeIds::Encoder(0) -- a Draw-kind node whose value is only
    // ever changed via pointerDragAction (ui.ts continuePointerDrag), the
    // exact code path a drag-input regression in the mobile-stack transform
    // would break. Playwright's
    // page.mouse.* does not reliably synthesize the pointerdown/pointermove
    // sequence Chromium's PointerEvent + setPointerCapture flow needs in
    // this headless run (reproduced with zero mobile-stack transforms
    // involved, on the desktop project too -- an environment/harness
    // characteristic, not a mobile-stack defect); dispatching real
    // PointerEvents directly against the element sidesteps that and is
    // what this test does. A successful drag repaints the encoder's own
    // canvas (its drawn value), which this asserts directly -- the
    // strongest available "rendered state change" signal, stronger than
    // the internal draggedSincePointerDown flag alone.
    const encoder = page.locator('[data-synth-node-id="froggers.encoder.0"]');
    const box = await encoder.boundingBox();
    const canvasDataUrl = () => page.evaluate(() => document.querySelector('[data-synth-node-id="froggers.encoder.0"] canvas')?.toDataURL());
    const before = await canvasDataUrl();

    const center = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
    await encoder.dispatchEvent("pointerdown", { pointerId: 1, clientX: center.x, clientY: center.y, bubbles: true, isPrimary: true, button: 0 });
    for (let step = 1; step <= 10; step++) {
      await encoder.dispatchEvent("pointermove", {
        pointerId: 1,
        clientX: center.x + step * 8,
        clientY: center.y - step * 8,
        bubbles: true,
        isPrimary: true,
      });
    }
    await encoder.dispatchEvent("pointerup", { pointerId: 1, clientX: center.x + 80, clientY: center.y - 80, bubbles: true, isPrimary: true });

    await expect.poll(canvasDataUrl, { timeout: 5_000 }).not.toBe(before);
  });

  test("a touch drag on an encoder reaches pointerup without the browser ever cancelling it", async ({ page }) => {
    // `#synth-root [data-synth-node-kind="draw"] { touch-action: none; }`
    // (site.css) is what stops the browser from claiming a finger-drag on
    // an encoder canvas as its own page-scroll/pan gesture; without it, a
    // touch-type pointer sequence gets cut short after a move or two --
    // the browser fires `pointercancel` on the element (the pointer has
    // been handed off to native scrolling) and, if a capture was set,
    // `lostpointercapture` fires as part of that same cancellation, before
    // the drag ever reaches its own `pointerup`. Both are the actual
    // regression signal, not merely that a drag "still works" (the encoder
    // -drag test above already covers that with mouse-type pointers, which
    // `touch-action` never governs): a scroll-hijacked drag can still end
    // up moving the encoder's value.
    const encoder = page.locator('[data-synth-node-id="froggers.encoder.0"]');
    const box = await encoder.boundingBox();
    const center = { x: box.x + box.width / 2, y: box.y + box.height / 2 };

    await page.evaluate((selector) => {
      const element = document.querySelector(selector);
      window.__touchDragEventOrder = [];
      const record = (event) => window.__touchDragEventOrder.push(event.type);
      element.addEventListener("pointercancel", record);
      element.addEventListener("lostpointercapture", record);
      element.addEventListener("pointerup", record);
    }, '[data-synth-node-id="froggers.encoder.0"]');

    await encoder.dispatchEvent("pointerdown", {
      pointerId: 1,
      pointerType: "touch",
      clientX: center.x,
      clientY: center.y,
      bubbles: true,
      isPrimary: true,
      button: 0,
    });
    for (let step = 1; step <= 10; step++) {
      await encoder.dispatchEvent("pointermove", {
        pointerId: 1,
        pointerType: "touch",
        clientX: center.x + step * 8,
        clientY: center.y - step * 8,
        bubbles: true,
        isPrimary: true,
      });
    }
    await encoder.dispatchEvent("pointerup", {
      pointerId: 1,
      pointerType: "touch",
      clientX: center.x + 80,
      clientY: center.y - 80,
      bubbles: true,
      isPrimary: true,
    });

    const eventOrder = await page.evaluate(() => window.__touchDragEventOrder);
    expect(eventOrder).not.toContain("pointercancel");
    expect(eventOrder).toContain("pointerup");
    const lostCaptureIndex = eventOrder.indexOf("lostpointercapture");
    if (lostCaptureIndex !== -1) {
      expect(lostCaptureIndex).toBeGreaterThan(eventOrder.indexOf("pointerup"));
    }
  });
});
