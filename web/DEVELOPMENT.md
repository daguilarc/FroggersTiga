# Web sim development

## AudioWorklet edits

`froggers-processor.ts` is compiled to `public/froggers-processor.js` before dev and build (`npm run build:worklet`).

After editing the worklet during a live `npm run dev` session, run:

```bash
npm run build:worklet
```

Or restart the dev server (`predev` recompiles on start).
