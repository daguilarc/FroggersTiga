// Task 4.3 -- shared selectors/helpers for the site e2e suite. Node ids
// mirror FroggersUiSurface.hpp's `FroggersNodeIds` (app/FroggersUiSurface.hpp
// :109-206) exactly, which is what Sheaf's browser UI backend publishes as
// each element's `data-synth-node-id` (External/Sheaf/projects/synth/
// browser/src/ui.ts, `updateNode()`).

// FroggersNodeIds::kLeftBlock -- scope, transport, scenes, scene-blend, bpm.
export const LEFT_BLOCK_SELECTOR = '[data-synth-node-id="froggers.layout.left"]';
// FroggersNodeIds::kRightBlock -- the bank chrome + 16-slot encoder grid
// (FroggersUiSurface.hpp:114).
export const RIGHT_BLOCK_SELECTOR = '[data-synth-node-id="froggers.layout.right"]';
// FroggersNodeIds::EncoderRow(0..3) -- the four 4-wide slices that make up
// the 16-slot (4x4) encoder grid (FroggersUiSurface.hpp:201-206).
export const ENCODER_ROW_SELECTORS = [0, 1, 2, 3].map((row) => `[data-synth-node-id="froggers.layout.right.row.${row}"]`);
export const SYNTH_ROOT_SELECTOR = "#synth-root";
export const SURFACE_ROOT_SELECTOR = '[data-synth-node-id="froggers.root"]';
// RuntimePages.hpp:34 `NodeIds::kSidebarRoot` -- Sheaf's own generic
// runtime-chrome sidebar (Audio/Controllers/Sync/File + CPU meter), a
// sibling of `froggers.root` under the composite `runtime.main.root`
// Sheaf's fitSurface actually scales (RuntimeMainComponent.hpp:197-214).
// Not a FroggersUiSurface node, but Group 4c's mobile stack includes it as
// a third stacked block per the operator's "everything else above or
// below the grid" rule.
export const SIDEBAR_SELECTOR = '[data-synth-node-id="runtime.sidebar.root"]';

/**
 * Waits for the app to have rendered at least one real UI frame (proof the
 * catalog loaded, the package materialized, and the wasm module started --
 * NOT proof audio started, which never happens in this suite: nothing here
 * ever clicks Play).
 */
export async function waitForSurfaceReady(page) {
  await page.locator(ENCODER_ROW_SELECTORS[0]).waitFor({ state: "attached", timeout: 45_000 });
}

/**
 * The union bounding box of the four encoder-row elements, i.e. the
 * 16-slot grid's own rendered extent.
 */
export async function encoderGridBoundingBox(page) {
  const boxes = await Promise.all(ENCODER_ROW_SELECTORS.map((selector) => page.locator(selector).boundingBox()));
  const present = boxes.filter((box) => box !== null);
  if (present.length === 0) return null;
  const left = Math.min(...present.map((box) => box.x));
  const top = Math.min(...present.map((box) => box.y));
  const right = Math.max(...present.map((box) => box.x + box.width));
  const bottom = Math.max(...present.map((box) => box.y + box.height));
  return { x: left, y: top, width: right - left, height: bottom - top };
}

/**
 * A block's own wire-set (design-space) width, read live from the DOM the
 * same way mobile-stack.mjs itself derives it (mobile-stack.mjs's own
 * `wireExtent`) -- never a hardcoded design width number.
 */
export async function wireWidth(page, selector) {
  return page.locator(selector).evaluate((el) => parseFloat(el.style.width));
}

/**
 * The ONE shared scale mobile-stack.mjs applies to every stacked block
 * when narrow (that file's own "ONE shared scale for the WHOLE stack"
 * comment): viewport width over the grid block's own live wire width.
 */
export async function gridSharedScale(page) {
  const viewport = page.viewportSize();
  const gridWire = await wireWidth(page, RIGHT_BLOCK_SELECTOR);
  return viewport.width / gridWire;
}

/**
 * A stacked block's expected on-screen width at the grid's shared scale
 * (gridSharedScale(page) * that block's own live wire width) -- the
 * formula both the chrome and sidebar width assertions need, since
 * mobile-stack.mjs applies the same shared scale to every stacked block.
 */
export async function expectedStackedWidth(page, selector) {
  const [scale, wire] = await Promise.all([gridSharedScale(page), wireWidth(page, selector)]);
  return scale * wire;
}

/**
 * Vertical overlap in px between two bounding boxes (0 when one renders
 * entirely above/below the other) -- the "stacked, not beside" predicate
 * the mobile-stacking and desktop-layout suites both need. Direction-
 * agnostic (only reports overlap, not which box is on top); callers that
 * know the stacking order from construction (as every current caller
 * does) get "A is above B" for free when this returns 0.
 */
export function verticalOverlapPx(boxA, boxB) {
  return Math.max(0, Math.min(boxA.y + boxA.height, boxB.y + boxB.height) - Math.max(boxA.y, boxB.y));
}
