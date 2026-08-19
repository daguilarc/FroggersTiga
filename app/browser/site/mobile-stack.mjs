// Group 4c -- shell-side per-block transforms for the mobile stacked layout
// (froggers-web-host spec.md "Mobile viewport stacks around a full-width
// encoder grid": chrome block above, encoder-grid block below, spanning the
// full viewport width, with everything else -- Sheaf's own generic sidebar
// included -- also placed above or below, never beside). The grid is what
// spans the full viewport width; chrome and the sidebar share ITS scale
// (operator decision, see applyMobileStack's own comment) rather than each
// being independently stretched to full width too, so they render at
// their own smaller, design-proportional size, left-aligned under it. See
// task-g4c-report.md for the full input-mapping trace this mechanism is
// gated on (drag/press coordinate math had to be made transform-robust in
// Sheaf's own ui.ts first -- see that report's Sheaf-fix section) and for
// why this file hooks `BrowserUiBackend`'s PUBLIC `renderFrame` method
// (ui.ts:41) rather than fighting CSS at the stylesheet level the way 4b
// tried and found blocked (see index.html's own header comment, superseded
// by this file for the mobile case): ui.ts's `updateNode()`
// unconditionally clears EVERY protocol node's `transform` on every render
// frame (ui.ts:94, `element.style.transform = "";`), and only
// `fitSurface()`'s own later, root-only re-application (ui.ts:308-319)
// survives that clear. A transform this shell applies directly to the
// wire-managed block elements would be wiped within one frame (~33ms)
// unless reasserted on that exact cadence -- so this module wraps the
// public `renderFrame` method and reruns its own application synchronously,
// in the same JS turn, right after Sheaf's own per-frame writes. It only
// ever sets host-owned CSS (`transform`/`transformOrigin` on the stacked
// block elements, `height`/`overflow` on the mount) and never touches a
// protocol node's wire-managed position/size (`left`/`top`/`width`/
// `height`) -- the "per-block CONTAINER transform" mechanism: it composes
// with the wire-managed properties instead of fighting them, which is what
// makes it survive where 4b's plain-CSS approach could not.

// Legacy site's own mobile breakpoint (web/src/main.ts:300,
// `window.matchMedia("(max-width: 720px)")`; web/src/style.css:321,507),
// carried forward as this task's own threshold per the brief.
const NARROW_MAX_WIDTH = 720;
// Shell's own choice, not a wire value: vertical gap between each stacked
// block.
const STACK_GAP_PX = 12;

const MOUNT_SELECTOR = "#synth-root"; // index.html:60, this shell's own id
// FroggersNodeIds::kLeftBlock / kRightBlock -- the outer split Row's two
// Weight(2)/Weight(4) siblings (FroggersUiSurface.hpp:113-114,464-467): the
// chrome block (scope/transport/scenes/blend/bpm) and the block holding the
// bank chrome + 16-slot encoder grid.
const CHROME_BLOCK_SELECTOR = '[data-synth-node-id="froggers.layout.left"]';
const GRID_BLOCK_SELECTOR = '[data-synth-node-id="froggers.layout.right"]';
// RuntimePages.hpp:34 `NodeIds::kSidebarRoot = "runtime.sidebar.root"` --
// Sheaf's OWN generic runtime-chrome sidebar (the "Audio/Controllers/Sync/
// File" toolbar + CPU meter), a SIBLING of `froggers.root` under the
// composite `runtime.main.root` fitSurface actually scales
// (RuntimeMainComponent.hpp:197-214,212: `root.bounds = {..., appRootWidth
// + Layout::kSidebarWidth, appRootHeight}`, `root.children = {contentTree
// ...front().id, sidebarTree...front().id}`). It is "everything else" for
// the operator's "chrome above, grid full-width, everything else above or
// below" rule -- stacked here as a third block, below the grid.
const SIDEBAR_SELECTOR = '[data-synth-node-id="runtime.sidebar.root"]';

// Stacked top to bottom, in this order.
const STACK_SELECTORS = [CHROME_BLOCK_SELECTOR, GRID_BLOCK_SELECTOR, SIDEBAR_SELECTOR];

function isNarrow(mount) {
  return mount.clientWidth > 0 && mount.clientWidth <= NARROW_MAX_WIDTH;
}

// Reads a block's WIRE-SET design-space extent -- the exact px string
// ui.ts's updateNode() writes from the node's resolved protocol bounds
// (ui.ts:98-99: `element.style.width/height = node.bounds.width/height +
// "px"`). Read live every call; never a hardcoded copy of the interior
// layout numbers (the 900x712 design box, the two Froggers block ids, and
// the sidebar's own id are the only structural facts this file bakes in,
// all cited above).
function wireExtent(element) {
  const width = parseFloat(element.style.width);
  const height = parseFloat(element.style.height);
  return width > 0 && height > 0 ? { width, height } : null;
}

// Measures ONE block's current (ancestor-scaled, own-transform-cleared)
// rect and wire extent, WITHOUT deciding anything about its final
// transform yet. Clearing `transform` here IS a DOM write (a block's own
// surface -- e.g. Sheaf's sidebar, rendered by its own, separate
// SidebarSurface -- can transiently report a degenerate extent mid-resize,
// so this write can turn out to have been for nothing); applyMobileStack
// is responsible for making the OVERALL operation atomic across all three
// blocks despite that -- see its own comment for how and why.
//
// IMPORTANT: this does NOT assume any particular ancestor is the element
// fitSurface() scales, and does not try to neutralize any ancestor's
// transform. Sheaf's generic runtime shell wraps the app's own tree in a
// further composite root (`runtime.main.root`, see SIDEBAR_SELECTOR's own
// comment above) that fitSurface actually scales -- confirmed empirically
// (task-g4c-report.md's mechanism section has the trace). Rather than
// special-case that ancestor (fragile if Sheaf's composite structure ever
// changes further), every measurement here is taken with the block's OWN
// transform cleared but every ancestor's transform left exactly as Sheaf
// set it, and the needed scale/translate (computed later, once every
// block has measured) is derived from the RATIO between the current
// (ancestor-scaled) rendered rect and the wire (design-space) extent --
// self-correcting for whatever scale any ancestor already contributes, at
// any depth, without needing to know or touch it.
function measureBlock(element) {
  const extent = wireExtent(element);
  if (!extent) return null;
  element.style.transform = "none";
  const rect = element.getBoundingClientRect();
  return rect.width > 0 ? { element, extent, rect } : null;
}

// Computes and applies ONE already-measured block's transform, using a
// SHARED `scale` (the same number for every block in the stack -- see
// applyMobileStack's own comment for why), left edge at `targetLeft`, top
// edge at `targetTop` (all real viewport px). Returns the block's own
// final on-screen height, for stacking the next block under it.
function applyStackedTransform(measurement, scale, targetLeft, targetTop) {
  const { element, extent, rect } = measurement;
  // A translate() inside an ancestor scale gets multiplied by that same
  // ancestor scale when painted (transforms compose down the tree), so a
  // raw "target minus current" delta would land short/long by exactly the
  // ancestor factor. `rect.width / extent.width` IS that ancestor factor:
  // rect.width already equals (ancestor scale x wire width), so dividing
  // it out recovers the ancestor's own contribution, self-correcting for
  // whatever scale any ancestor already applies, at any depth, without
  // needing to know or touch it.
  const ancestorScale = rect.width / extent.width;
  // The block's OWN needed scale, on top of the ancestor's contribution
  // above, so the TOTAL effective on-screen scale (ancestorScale x
  // ownScale) equals the shared `scale` passed in -- not each block's own
  // independent "fill the viewport" scale.
  const ownScale = scale / ancestorScale;
  const scaledHeight = scale * extent.height;

  element.style.transformOrigin = "0 0";
  element.style.transform =
    `translate(${(targetLeft - rect.left) / ancestorScale}px, ${(targetTop - rect.top) / ancestorScale}px) scale(${ownScale})`;
  return scaledHeight;
}

// Last successfully computed transform for each stacked block, keyed by
// its own SELECTOR (not the DOM element -- selectors are stable identity
// across frames even if Sheaf ever recreated a node's element; elements
// are not assumed to be). This is the fallback re-applied when a frame's
// measurement fails for ANY block -- and it has to be a cache like this,
// not a live read of `element.style.transform`: ui.ts's own `updateNode()`
// (part of the ORIGINAL `renderFrame()` this module wraps, which always
// runs BEFORE this module's own code, every frame) unconditionally clears
// EVERY node's `transform` to `""` before this module ever sees it -- so
// reading `element.style.transform` at the top of applyMobileStack always
// finds that wiped value, never the previous frame's real stacked
// transform. (An earlier version of this fix tried exactly that "read the
// live style, restore it on failure" approach; it looked correct but
// verifiably was not -- a forced-failure e2e test caught it reverting
// chrome/grid to something close to their NATIVE, un-stacked size even
// though the restore code ran, because what it captured and restored was
// already the cleared value, not the real one. See task-g4c-report.md's
// FIX 3 follow-up section for the full trace and the test that caught it.)
// `lastGoodMountHeight` is the same idea for the mount's own height:
// ui.ts's `fitSurface()` (also inside the original renderFrame, also
// always running before this module) unconditionally rewrites
// `mount.style.height` to its own (wide-appropriate) value every frame
// too, so "leave it untouched on failure" would silently let that
// wide-style value show through the mount for that one frame instead of
// this module's own last-good stacked total.
const lastGoodTransform = new Map();
let lastGoodMountHeight = null;

/**
 * Applies (while narrow) or clears (while wide) the per-block stack
 * transform. Idempotent and safe to call every render frame and on every
 * resize. At wide viewports this is a true no-op: it touches nothing, so
 * Sheaf's own unmodified fitSurface/updateNode output is exactly what
 * reaches the screen (matches "wide viewports: hands off").
 *
 * Applies ATOMICALLY across the three stacked blocks: measures all three
 * first (measureBlock), and only commits a NEW transform to any of them if
 * EVERY block measured successfully. If even one is transiently
 * unmeasurable (e.g. Sheaf's sidebar surface mid-rebuild reporting a
 * momentary zero-extent bounds during a resize -- observed in practice,
 * and forced deterministically in the e2e regression test for this),
 * every element that measureBlock already cleared to `transform: none`
 * earlier in this same pass is put back to its own last KNOWN-GOOD
 * stacked transform (lastGoodTransform, see its own comment), the mount's
 * height is likewise put back to its own last-good total
 * (lastGoodMountHeight), and this frame is otherwise a no-op -- the
 * PREVIOUS frame's fully-correct stacked state is what's left standing,
 * not one block reverted to its native position while the other two stay
 * stacked. Everything in this function runs synchronously (no `await`/rAF
 * inside it), so the browser cannot paint an intermediate state between
 * "some elements cleared" and "all restored" (or, on success, "all
 * re-stacked") -- only the state present when this function RETURNS is
 * ever visible. The next render frame (~33ms later) retries from scratch
 * either way.
 */
export function applyMobileStack() {
  const mount = document.querySelector(MOUNT_SELECTOR);
  if (!mount) return;

  if (!isNarrow(mount)) {
    // Wide viewport: clear any of OUR OWN prior overrides (a no-op unless
    // the viewport just crossed narrow->wide) and touch nothing else --
    // Sheaf's own renderFrame call this runs after already left the
    // correct wide-mode state on every node it owns.
    //
    // Deliberately does NOT clear lastGoodTransform/lastGoodMountHeight
    // here. `mount.clientWidth` can transiently read a stale "still wide"
    // value for a frame or two while an active resize is settling (layout
    // catching up with a just-issued viewport change), so `isNarrow()`
    // reaching `false` for one frame mid-transition does not reliably mean
    // "the user is actually at a wide viewport now" -- an e2e regression
    // test caught this: clearing the caches on that stale read left a
    // SUBSEQUENT transient measurement failure (see applyStackedTransform's
    // own atomicity comment) with nothing to restore from, reverting one
    // block to its native size for a frame even though the OTHER two
    // blocks (whose own measurements didn't fail that frame) looked fine.
    // Stale cache entries are harmless while genuinely wide (nothing here
    // reads them at all in that state); the only cost of leaving them is
    // that the FIRST transient failure after a later re-entry into narrow
    // mode could fall back to a slightly-stale (previous viewport width's)
    // transform for one frame instead of "no transform" -- strictly better,
    // not worse, and self-corrects on the very next successful frame.
    // `overflow` is ours to clear; the mount's `height` is NOT OURS TO WRITE
    // AT ALL out here. Sheaf's fitSurface owns that inline property
    // (browser/src/ui.ts:346, `root.style.height = surfaceHeight *
    // surfaceScale`) and this hook runs immediately AFTER each renderFrame,
    // so the `mount.style.height = ""` that used to live here erased Sheaf's
    // sizing every frame: the mount's only child is absolutely positioned,
    // so the mount computed to height 0 and clipped the whole surface away
    // -- a blank page at every wide viewport, which narrow mode masked by
    // setting a height of its own. Leaving the property untouched HERE (the
    // wide branch) keeps Sheaf the sole writer of the WIDE-mode value
    // specifically -- not a project-wide single-writer guarantee for this
    // property: the narrow branch below (applyMobileStack's own
    // `mount.style.height = ...` lines) writes it too, on purpose, with its
    // own separately-computed stacked total, which is a distinct value for
    // a distinct state rather than a re-derivation of Sheaf's number.
    // Entering wide mode is by definition a width
    // change, so Sheaf's own ResizeObserver -> fitSurface re-asserts the
    // correct height on the same resize that clears the narrow layout; the
    // narrow branch's height therefore never outlives the transition.
    mount.style.overflow = "";
    return;
  }

  const elements = STACK_SELECTORS.map((selector) => document.querySelector(selector));
  if (elements.some((element) => !element)) return; // not all rendered yet

  const measurements = elements.map(measureBlock);
  if (measurements.some((measurement) => measurement === null)) {
    // Restore every block to its own cached last-good transform (leaving
    // it cleared if this stack has never successfully applied yet --
    // nothing better to fall back to), and the mount to its own
    // last-good total height.
    STACK_SELECTORS.forEach((selector, index) => {
      const cached = lastGoodTransform.get(selector);
      if (!cached) return;
      elements[index].style.transformOrigin = cached.transformOrigin;
      elements[index].style.transform = cached.transform;
    });
    if (lastGoodMountHeight !== null) mount.style.height = lastGoodMountHeight;
    return;
  }

  const viewportWidth = mount.clientWidth;
  const mountRect = mount.getBoundingClientRect();

  // ONE shared scale for the WHOLE stack (operator decision, superseding
  // this file's earlier per-block-independent scaling): each block used
  // to be stretched to its OWN full viewport width, but the grid's design
  // width (~600 of the 900 design box) is roughly double the chrome
  // block's (~300, FroggersUiSurface.hpp:464-467's 4:2 weighting), so
  // that made chrome render at ~2x the scale the grid needed -- ballooning
  // chrome's height and pushing the grid far down the page. Deriving ONE
  // scale from the grid block's own live wire width and applying it to
  // every stacked block keeps the grid spanning the full viewport width
  // (unchanged requirement) while chrome and the sidebar render at their
  // own, smaller, design-proportional size instead of being stretched.
  const gridIndex = STACK_SELECTORS.indexOf(GRID_BLOCK_SELECTOR);
  const sharedScale = viewportWidth / measurements[gridIndex].extent.width;

  let top = mountRect.top;
  let totalHeight = 0;
  measurements.forEach((measurement, index) => {
    // Left-aligned (not centered): every block starts at the same left
    // edge as the mount/grid, including chrome and the sidebar even
    // though they now render narrower than the viewport -- simpler,
    // predictable, and keeps everything flush against one shared edge
    // rather than each block needing its own centering offset.
    const scaledHeight = applyStackedTransform(measurement, sharedScale, mountRect.left, top);
    lastGoodTransform.set(STACK_SELECTORS[index], {
      transform: measurement.element.style.transform,
      transformOrigin: measurement.element.style.transformOrigin,
    });
    top += scaledHeight + STACK_GAP_PX;
    totalHeight += scaledHeight + STACK_GAP_PX;
  });
  totalHeight -= STACK_GAP_PX; // no trailing gap after the last block

  // fitSurface's own mount height (sized for its own, un-stacked scale) no
  // longer applies; reserve exactly the stacked content's own extent
  // (chrome + grid + sidebar, all three now stacked -- "everything else"
  // per the operator's rule, not just the two Froggers blocks) so the
  // shell's own footer chrome sits right below the sidebar, and clip
  // whatever of Sheaf's own composite tree (e.g. its own outer margin)
  // would otherwise peek out under `overflow: visible`.
  lastGoodMountHeight = `${totalHeight}px`;
  mount.style.height = lastGoodMountHeight;
  mount.style.overflow = "hidden";
}

// rAF-debounced apply, plus a short burst of follow-up ticks -- catches
// both a secondary layout shift the resize itself can trigger (e.g. a
// scrollbar appearing/disappearing changing `mount.clientWidth` again
// after the first pass already ran) AND ui.ts's OWN internal
// `ResizeObserver` on this same element (see installMobileStack's own
// comment): that observer calls `fitSurface()` directly, independent of
// `renderFrame()`, so it is NOT something this module's `renderFrame`
// patch alone can order against -- it can fire in the same
// resize-observation batch as this callback, in EITHER order (browsers do
// not guarantee inter-observer callback order), so a single follow-up
// tick is not reliably enough margin. `renderFrame()`'s own ~33ms cadence
// self-heals any single miss on its own, but that isn't sufficient during
// this active window (a resize burst can legitimately outlast one frame,
// and this shell would rather actively re-assert than rely on the app's
// own frame cadence alone). FOLLOW_UP_TICKS spans several rAF ticks
// (~16ms each, well over two full ui.ts render-frame intervals) so any
// interleaving with ui.ts's own resize-triggered fitSurface() call is
// re-corrected well within the same user-perceived resize gesture.
const FOLLOW_UP_TICKS = 8;
let applyScheduled = false;
function scheduleApply() {
  if (applyScheduled) return;
  applyScheduled = true;
  let remaining = FOLLOW_UP_TICKS;
  const tick = () => {
    applyMobileStack();
    remaining -= 1;
    if (remaining > 0) requestAnimationFrame(tick);
    else applyScheduled = false;
  };
  requestAnimationFrame(tick);
}

/**
 * Wires applyMobileStack() to run after every render frame -- fighting
 * ui.ts's own per-frame style resets, see this file's header comment --
 * and on resize, so a narrow<->wide crossing doesn't have to wait for the
 * next wasm-driven frame tick. Call once, before booting the app. Patches
 * the SHARED prototype method (not a specific instance): site-boot.mjs
 * never gets a direct handle to the `BrowserUiBackend`
 * `installSynthBrowserApp` constructs internally, but every instance looks
 * up `renderFrame` through the same prototype at call time, so this covers
 * it regardless.
 *
 * Resize detection uses a `ResizeObserver` on the SAME host element Sheaf
 * itself observes for `fitSurface` (ui.ts:37-38, `this.resizeObserver =
 * new ResizeObserver(() => this.fitSurface()); this.resizeObserver.
 * observe(root);` where `root` is the mount passed into
 * `BrowserUiBackend`'s constructor -- `#synth-root`, same as
 * MOUNT_SELECTOR here) rather than `window`'s `resize` event alone: a
 * `window.resize` doesn't fire for every layout change that resizes the
 * mount (e.g. this shell's own responsive chrome reflowing without the
 * outer window changing size at all), so observing the exact element
 * Sheaf's own fitSurface reacts to is what makes this react to every
 * resize fitSurface itself reacts to, not just window-level ones. It does
 * NOT, on its own, guarantee running strictly after that same resize's
 * `fitSurface()` call -- two independent `ResizeObserver` instances on one
 * element have no browser-guaranteed callback ordering -- which is exactly
 * why `scheduleApply`'s multi-tick follow-up burst exists (see its own
 * comment): order-safety here comes from repeatedly re-asserting shortly
 * after the resize, not from winning a single race.
 */
export function installMobileStack(BrowserUiBackend) {
  const originalRenderFrame = BrowserUiBackend.prototype.renderFrame;
  BrowserUiBackend.prototype.renderFrame = function patchedRenderFrame(...args) {
    const result = originalRenderFrame.apply(this, args);
    applyMobileStack();
    return result;
  };

  const mount = document.querySelector(MOUNT_SELECTOR);
  if (mount) {
    const resizeObserver = new ResizeObserver(scheduleApply);
    resizeObserver.observe(mount);
  }
  // Kept as a belt-and-suspenders trigger (orientation change on some
  // engines resizes the layout viewport without necessarily resizing the
  // mount element on the very same tick the OS event fires) -- cheap,
  // idempotent, and scheduleApply() itself is debounced.
  window.addEventListener("orientationchange", scheduleApply);
}
