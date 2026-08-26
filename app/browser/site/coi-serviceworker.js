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
        // The same three headers serve-site.mjs sends, and for the same
        // reason: under require-corp a subresource without a resource policy
        // is blocked, so sending only the opener/embedder pair isolates
        // nothing and the app fails to boot with its pthread pool unable to
        // transfer SharedArrayBuffer memory.
        headers.set("Cross-Origin-Resource-Policy", "cross-origin");
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
    navigator.serviceWorker.register(scriptUrl).then(() => {
      // Already controlled: the worker's fetch handler covers this
      // navigation, isolation holds, nothing to do.
      if (navigator.serviceWorker.controller) {
        return;
      }
      // Otherwise reload once the worker is active, so its fetch handler
      // covers the navigation itself.
      //
      // Keyed on navigator.serviceWorker.ready rather than an `updatefound`
      // listener: register() resolves only AFTER registration.installing is
      // set, so on a first visit `updatefound` has usually already fired by
      // the time a listener could be attached, and a listener attached after
      // the fact never runs -- no reload, the page stays non-isolated, and
      // the SharedArrayBuffer failure surfaces as a permanent "could not
      // start" panel on what is a perfectly capable browser. That window is
      // wider the busier the machine is, which made it look intermittent.
      // `ready` resolves once there IS an active registration, covering the
      // fresh-install and already-active cases alike, and reloadOnce's
      // sessionStorage guard still bounds this to one attempt per tab.
      navigator.serviceWorker.ready.then(() => {
        if (!navigator.serviceWorker.controller) {
          reloadOnce();
        }
      });
    }).catch((error) => {
      console.error("cross-origin-isolation service worker registration failed", error);
    });
  }
}
