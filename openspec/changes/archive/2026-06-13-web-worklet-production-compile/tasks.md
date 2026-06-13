## 1. Build pipeline

- [x] 1.1 Add `esbuild` (^0.25.0) to `web/package.json` devDependencies
- [x] 1.2 Add `scripts/build-worklet.mjs` — esbuild `web/src/froggers-processor.ts` → `web/public/froggers-processor.js` (`format: 'esm'`, `target: 'es2020'`, `platform: 'browser'`, `bundle: false`); mirror root-path pattern from `verify-wasm-exports.mjs`
- [x] 1.3 Add `scripts/verify-worklet-js.mjs` — read entire output file; fail on `/\binterface\s+[A-Za-z_]/`, `/:\s*WebAssembly\./`, or `/\)\s*:\s*(number|void|string|boolean)\s*[{;]/`
- [x] 1.4 Wire scripts in `web/package.json`:
  - `"build:worklet": "node ../scripts/build-worklet.mjs"`
  - `"verify:worklet": "node ../scripts/verify-worklet-js.mjs"`
  - `"predev": "npm run build:worklet && node ../scripts/check-wasm-present.mjs"`
  - `"build": "npm run sync:docs && npm run build:worklet && npm run verify:worklet && tsc && vite build"`
- [x] 1.5 Document dev edit workflow: after changing `froggers-processor.ts` during live dev, run `npm run build:worklet` or restart dev

## 2. Main thread loader

- [x] 2.1 Remove `import processorUrl from "./froggers-processor.ts?url"` in `main.ts`
- [x] 2.2 Set `const processorUrl = \`${import.meta.env.BASE_URL}froggers-processor.js\`;` (mirror `wasmUrl`)
- [x] 2.3 Revert `vite.config.ts` `assetFileNames` `.ts`→`.js` hack (delete the custom `output.assetFileNames` block)

## 3. Verify locally

- [x] 3.1 `npm run build` — confirm `dist/froggers-processor.js` exists at dist root (copied from public); file contains no `interface`
- [x] 3.2 Confirm dist has **no** `assets/froggers-processor-*` hashed asset
- [x] 3.3 `VITE_BASE=/FroggersTiga/ npm run preview` — Play grays, audio audible, no addModule parse error
- [x] 3.4 `npm run dev` — Play works after predev compile

## 4. Deploy

- [ ] 4.1 Push; Pages workflow green (inherits worklet compile via `npm run build`; no workflow edit)
- [ ] 4.2 Live: https://daguilarc.github.io/FroggersTiga/ — Play + audio; DevTools Network shows `froggers-processor.js` as `application/javascript`

## 5. Manual regression

- [ ] 5.1 Randomize updates knobs (worklet alive post-Play)
- [ ] 5.2 External input meter moves when Ext. In on + signal

## 6. Archive

- [x] 6.1 Archive change; merge `specs/web-worklet-module-load/spec.md` delta into `openspec/specs/web-worklet-module-load/spec.md` (replace stale `?url` requirement and update Purpose)
