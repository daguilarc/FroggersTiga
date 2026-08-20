// Cross-Origin-Isolation shim, needed because GitHub Pages has
// no mechanism to set custom HTTP response headers (no per-path header
// config of any kind), yet frogg3rs.js is compiled with Emscripten
// pthreads and unconditionally starts its pthread pool on module load --
// which requires a crossOriginIsolated context (COOP: same-origin + COEP:
// require-corp response headers) to transfer SharedArrayBuffer-backed wasm
// memory to its workers. Without those headers this repo's own
// serve-site.mjs now sends them (see its header comment), so local/CI
// serving already works; GitHub Pages cannot, so this shim is the only
// remaining path to a working deployed site. Confirmed necessary
// empirically: `installSynthBrowserApp` throws
// `DataCloneError: ... SharedArrayBuffer transfer requires
// self.crossOriginIsolated` immediately on load when served without either
// real headers or this shim.
//
// Standard technique for this exact problem (same approach as the
// widely-used https://github.com/gzuidhof/coi-serviceworker, MIT --
// written narrowly for this file rather than vendored verbatim, since this
// single-origin site only ever needs to isolate its own same-origin
// resources, not that project's general cross-origin case): a service
// worker adds the two response headers to every same-origin fetch, then
// the page reloads once so the browser re-evaluates isolation against the
// new headers. Loaded as a plain classic script (not a module) so
// `document.currentScript` is available for the registration path below,
// and dual-purposed as the worker script itself -- the same file runs in
// both contexts, distinguished by `typeof window`.
if (typeof window === "undefined") {
  // Service worker context: add the two isolation headers to every
  // same-origin response this page fetches (including the navigation
  // request itself on the next load/reload).
  self.addEventListener("install", () => {
    self.skipWaiting();
  });
  self.addEventListener("activate", (event) => {
    event.waitUntil(self.clients.claim());
  });
  self.addEventListener("fetch", (event) => {
    if (event.request.cache === "only-if-cached" && event.request.mode !== "same-origin") return;
    event.respondWith(
      fetch(event.request).then((response) => {
        // Opaque/opaqueredirect responses (cross-origin, no-cors) carry no
        // readable headers to rebuild from -- pass them through untouched
        // rather than throw. This site never issues such a request itself,
        // but a future one should degrade instead of breaking every fetch.
        if (response.status === 0 || response.type === "opaque" || response.type === "opaqueredirect") return response;
        const headers = new Headers(response.headers);
        headers.set("Cross-Origin-Embedder-Policy", "require-corp");
        headers.set("Cross-Origin-Opener-Policy", "same-origin");
        return new Response(response.body, {
          status: response.status,
          statusText: response.statusText,
          headers,
        });
      }).catch(() => fetch(event.request)),
    );
  });
} else {
  // Page context: only bother if the server didn't already isolate us
  // (serve-site.mjs does, so this is a no-op there) and the browser
  // supports service workers at all.
  if (window.crossOriginIsolated === false && "serviceWorker" in navigator) {
    const scriptUrl = document.currentScript.src;
    // One-shot reload guard (same idiom as the upstream
    // https://github.com/gzuidhof/coi-serviceworker this shim is modeled
    // on): registration resolving does not guarantee the NEXT load is
    // actually controlled by the worker -- blocked storage, private
    // browsing, or a scope mismatch can all leave a browser stuck
    // re-registering successfully but never gaining a controller, which
    // without this guard would reload() on every single load, forever.
    // sessionStorage survives a same-tab reload() but not a fresh
    // tab/window, so this allows at most one reload attempt per tab
    // session; a second attempt fails visibly (logged here, and
    // site-boot.mjs's own boot-error panel surfaces the resulting
    // SharedArrayBuffer failure) instead of looping.
    const RELOAD_GUARD_KEY = "frogg3rs-coi-reloaded";
    const reloadOnce = () => {
      if (sessionStorage.getItem(RELOAD_GUARD_KEY)) {
        console.error(
          "cross-origin-isolation service worker is registered but this tab is still " +
          "not crossOriginIsolated after one reload attempt -- giving up instead of " +
          "reloading forever (blocked storage, private browsing, and a scope mismatch " +
          "can all cause this).",
        );
        return;
      }
      sessionStorage.setItem(RELOAD_GUARD_KEY, "1");
      window.location.reload();
    };
    navigator.serviceWorker.register(scriptUrl).then((registration) => {
      // Already active from an earlier visit but not yet controlling THIS
      // load (the very first navigation after registration never is) --
      // reload so the worker's fetch handler covers the navigation itself.
      if (registration.active && !navigator.serviceWorker.controller) {
        reloadOnce();
        return;
      }
      registration.addEventListener("updatefound", () => {
        reloadOnce();
      });
    }).catch((error) => {
      console.error("cross-origin-isolation service worker registration failed", error);
    });
  }
}
