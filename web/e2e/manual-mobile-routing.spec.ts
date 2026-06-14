import { expect, test } from "@playwright/test";
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import path from "node:path";

const webRoot = path.dirname(path.dirname(fileURLToPath(import.meta.url)));

test("sim-manual documents mobile External routing", async () => {
  const manualPath = path.join(webRoot, "public", "sim-manual.md");
  const manual = await readFile(manualPath, "utf8");
  expect(manual).toContain("**Mobile browsers**");
  expect(manual).toContain("earpiece");
});
