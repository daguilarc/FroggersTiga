import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { defineConfig, devices } from "@playwright/test";

const previewPort = 4173;
const previewHost = "127.0.0.1";

if (!process.env.CI) {
  const cacheCandidates = [
    path.join(os.homedir(), "Library/Caches/ms-playwright"),
    path.join(os.homedir(), ".cache/ms-playwright"),
  ];
  for (const cacheDir of cacheCandidates) {
    if (!fs.existsSync(cacheDir)) {
      continue;
    }
    const hasChromium = fs.readdirSync(cacheDir).some((name) => name.startsWith("chromium"));
    if (!hasChromium) {
      continue;
    }
    process.env.PLAYWRIGHT_BROWSERS_PATH = cacheDir;
    if (
      process.platform === "darwin" &&
      process.arch === "arm64" &&
      !process.env.PLAYWRIGHT_HOST_PLATFORM_OVERRIDE
    ) {
      const shellRoot = fs
        .readdirSync(cacheDir)
        .find((name) => name.startsWith("chromium_headless_shell-"));
      if (shellRoot) {
        const arm64Bin = path.join(cacheDir, shellRoot, "chrome-headless-shell-mac-arm64");
        const x64Bin = path.join(cacheDir, shellRoot, "chrome-headless-shell-mac-x64");
        if (fs.existsSync(arm64Bin) && !fs.existsSync(x64Bin)) {
          const darwinMajor = Number(os.release().split(".")[0]);
          const macTag = darwinMajor >= 25 ? "mac15" : darwinMajor >= 24 ? "mac14" : "mac13";
          process.env.PLAYWRIGHT_HOST_PLATFORM_OVERRIDE = `${macTag}-arm64`;
        }
      }
    }
    break;
  }
}

export default defineConfig({
  testDir: "./e2e",
  fullyParallel: false,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  workers: 1,
  timeout: 60_000,
  use: {
    baseURL: `http://${previewHost}:${previewPort}`,
    trace: "on-first-retry",
    launchOptions: {
      args: ["--autoplay-policy=no-user-gesture-required"],
    },
  },
  webServer: {
    command: `npm run build && npm run preview -- --host ${previewHost} --port ${previewPort}`,
    url: `http://${previewHost}:${previewPort}`,
    reuseExistingServer: !process.env.CI,
    timeout: 180_000,
  },
  projects: [
    {
      name: "chromium",
      use: { ...devices["Desktop Chrome"] },
    },
  ],
});
