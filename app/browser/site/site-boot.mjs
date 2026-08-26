// Direct-load boot for the dedicated frogg3rs site. Mirrors
// Sheaf's own `launchCatalogApplication`
// (External/Sheaf/projects/synth/browser/src/main.ts, `launchCatalogApplication`) MINUS
// `SheafPatchLauncher`'s picker UI (`installSheafPatchLauncher`, never imported here):
// this page is dedicated to frogg3rs, so it loads the catalog, finds the
// one app, and boots it directly -- no catalog-browser list is ever
// rendered. Every imported symbol is Sheaf's own, unmodified, copied
// verbatim into ./sheaf-runtime/ by package-catalog.mjs's assembleSite();
// nothing here reimplements catalog loading, package verification (SHA-256
// digests -- package-loader.js's own `materializePackage`), or app
// bootstrapping (main.js's own `installSynthBrowserApp`).
//
// Deliberately does NOT pass an `activationLease`: `SheafPatchLauncher`
// only needs one because its "Launch" button click is the user gesture
// audio activation must be anchored to (`installSheafPatchLauncher`). This page has no
// such click -- `SynthBrowserApp` defers audio activation to the first
// in-app UI action instead (main.ts's own `BrowserUiBackend` dispatch
// wiring calls `startUserActivation()` after every
// dispatched action), which is exactly the app's own Play control, same as
// every other Sheaf host. No audio starts on load (sbw-4 covers input;
// this covers output the same way): matches the e2e suite's "no audio
// start" requirement.
import { CatalogClient } from "./sheaf-runtime/catalog-client.js";
import { runtimeIdentityForCatalogApp } from "./sheaf-runtime/catalog.js";
import { installSynthBrowserApp } from "./sheaf-runtime/main.js";
import { materializePackage } from "./sheaf-runtime/package-loader.js";
import { BrowserUiBackend } from "./sheaf-runtime/ui.js";
import { installMobileStack } from "./mobile-stack.mjs";

const APP_ID = "frogg3rs";

// Installs before the app boots so the very first render frame
// already carries the mobile stack when narrow. See mobile-stack.mjs's own
// header comment for the full mechanism and its input-mapping trace.
installMobileStack(BrowserUiBackend);

// A failed boot must SAY so on the page. The data attribute alone left a
// visitor (and the operator, debugging remotely) staring at a blank frame
// between the header and footer with the reason readable only from the
// console or a DOM inspector.
function fail(root, error) {
  const message = error instanceof Error ? error.message : "frogg3rs failed to start";
  root.dataset.synthStatus = message;
  root.style.height = "";
  root.style.overflow = "";
  const notice = document.createElement("div");
  notice.className = "boot-error";
  notice.setAttribute("role", "alert");
  const heading = document.createElement("p");
  heading.className = "boot-error-heading";
  heading.textContent = "frogg3rs could not start in this browser.";
  const detail = document.createElement("p");
  detail.className = "boot-error-detail";
  detail.textContent = message;
  notice.append(heading, detail);
  root.replaceChildren(notice);
  console.error("frogg3rs boot failed", error);
}

async function boot(root) {
  const sourcesUrl = root.dataset.synthCatalogSources ?? "./catalog-sources.json";
  const client = new CatalogClient({ sourcesUrl });
  const result = await client.loadSources({ cacheMode: "default" });
  const app = result.apps.find((candidate) => candidate.appId === APP_ID);
  if (!app) {
    const detail = result.diagnostics
      .map((d) => `${d.catalogUrl}: ${d.status}${d.message ? ` (${d.message})` : ""}`)
      .join("; ");
    throw new Error(detail || `${APP_ID} is not present in the catalog`);
  }

  let materialized;
  try {
    materialized = await materializePackage(app);
    await installSynthBrowserApp(root, {
      module: materialized,
      runtimeVersions: {
        abiVersion: app.browser.abiVersion,
        uiProtocolVersion: app.browser.uiProtocolVersion,
        runtimeConfigVersion: app.browser.runtimeConfigVersion,
      },
      runtimeIdentity: runtimeIdentityForCatalogApp(app),
      disposeModule: () => materialized.dispose(),
    });
  } catch (error) {
    materialized?.dispose();
    throw error;
  }
}

// Chromium (and other browsers) dispatch a genuine `window` `error` event
// for "ResizeObserver loop completed with undelivered notifications." /
// "ResizeObserver loop limit exceeded" -- a well-known, benign, spec-
// sanctioned notice (not a real thrown Error: `event.error` is null,
// `event.message` carries the text) that fires whenever a ResizeObserver
// callback itself triggers layout work that would need yet another
// observation in the same frame. This app has TWO independent
// ResizeObservers on the same `#synth-root` element -- Sheaf's own
// `fitSurface`-triggering one (ui.ts) and mobile-stack.mjs's own
// (installMobileStack's own comment on why) -- which is exactly the
// reactive-layout pattern that provokes this notice, especially at narrow
// viewports where mobile-stack.mjs's per-frame stacking pass is actively
// engaged. Empirically confirmed harmless and frequent here (caught by
// this file's own e2e suite going red the moment the backstop below
// started treating it as fatal): DO NOT treat it as a boot failure.
const BENIGN_WINDOW_ERROR_MESSAGES = new Set([
  "ResizeObserver loop completed with undelivered notifications.",
  "ResizeObserver loop limit exceeded",
]);

const root = document.querySelector("#synth-root");
if (root) {
  void boot(root).catch((error) => fail(root, error));

  // Backstop for failures OUTSIDE boot()'s own promise chain (e.g. an
  // uncaught error from code that runs later, on its own timer/rAF/event
  // callback, never awaited by boot() itself). Does NOT double-fire for
  // boot()'s own errors: a promise with an attached .catch() (above) is
  // "handled" and never also raises `unhandledrejection`.
  //
  // Does NOT, and structurally cannot, catch a static import failure of
  // this module's OWN `./sheaf-runtime/*.js` imports at the top of this
  // file: per ES module semantics, a module's top-level code (including
  // these two addEventListener calls) never runs at all if any of its own
  // static imports fails to resolve, no matter where in the file a
  // listener is registered -- so a handler living in this file cannot be
  // the backstop for THIS file's own import failing. That specific case
  // (the exact scenario that produces a silent blank page) needs a
  // handler that does not depend on this module having loaded in the
  // first place; see index.html's own early inline <script>, registered
  // before this module's <script type="module"> tag, for that layer.
  window.addEventListener("error", (event) => {
    if (BENIGN_WINDOW_ERROR_MESSAGES.has(event.message)) return;
    fail(root, event.error instanceof Error ? event.error : new Error(event.message || "frogg3rs failed to start"));
  });
  window.addEventListener("unhandledrejection", (event) => {
    fail(root, event.reason);
  });
}
