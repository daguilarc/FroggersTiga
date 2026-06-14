import { defineConfig, devices } from "@playwright/test";

const previewPort = 4173;
const previewHost = "127.0.0.1";

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
