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

    // Published synchronously, before register() is even called, as the
    // ONE object both painters read: index.html's inline backstop runs
    // synchronously (it cannot await anything) and needs `pending`
    // readable on the spot; site-boot.mjs is async and awaits `settled`
    // instead. Both fields are driven by the single settle() function
    // below so they can never disagree about whether an isolation attempt
    // is still owed. Absent entirely when this branch does not run at all
    // (the server already sent real isolation headers) -- both consumers
    // treat "global absent" as "no attempt owed".
    let resolveSettled;
    const settled = new Promise((resolve) => {
      resolveSettled = resolve;
    });
    const attempt = { pending: true, settled };
    function settle() {
      attempt.pending = false;
      resolveSettled();
    }
    window.frogg3rsCoiAttempt = attempt;

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
    // SharedArrayBuffer failure) instead of looping. Returns whether a
    // reload was actually initiated, so the caller can settle() the
    // attempt above when it was declined instead (no reload means no
    // event is ever coming that would otherwise settle it).
    const RELOAD_GUARD_KEY = "frogg3rs-coi-reloaded";
    const reloadOnce = () => {
      if (sessionStorage.getItem(RELOAD_GUARD_KEY)) {
        console.error(
          "cross-origin-isolation service worker is registered but this tab is still " +
          "not crossOriginIsolated after one reload attempt -- giving up instead of " +
          "reloading forever (blocked storage, private browsing, and a scope mismatch " +
          "can all cause this).",
        );
        return false;
      }
      sessionStorage.setItem(RELOAD_GUARD_KEY, "1");
      window.location.reload();
      return true;
    };
    navigator.serviceWorker.register(scriptUrl).then(() => {
      // NO `controller` CHECK HERE, DELIBERATELY. Being controlled is not
      // the same as being isolated, and conflating them is what made a
      // first visit fail. This worker calls skipWaiting() on install and
      // clients.claim() on activate (see the service-worker branch at the
      // top of this file), so on a first visit it frequently claims THIS
      // document before register() even resolves -- but this document's
      // own navigation response was already served without the isolation
      // headers, so it is controlled and still not isolated. A
      // controller-keyed check reads that as "isolation holds, nothing to
      // do" and never reloads, leaving the page permanently un-isolated
      // until someone reloads it by hand. That is the actual first-visit
      // failure, not merely a slow reload: the busier the machine, the
      // more often claim wins that race.
      //
      // `crossOriginIsolated` is fixed for a document's lifetime, and this
      // whole branch only runs when it is already false, so this document
      // can NEVER become isolated in place. Once a worker is active to
      // serve the headers, the only remedy is a fresh navigation.
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
        // reloadOnce() returning false means the guard had already fired
        // -- no reload is coming this time, so settle instead of leaving
        // the attempt owed forever, and let the boot path surface
        // whatever real failure follows. When it returns true the page is
        // being replaced; do not settle, there is no one left to observe
        // it.
        if (!reloadOnce()) settle();
      });
    }).catch((error) => {
      console.error("cross-origin-isolation service worker registration failed", error);
      // Registration itself failed -- no reload will ever be scheduled.
      settle();
    });
  }
}
