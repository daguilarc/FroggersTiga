import type { Page } from "@playwright/test";
import { PLAY_LABEL, PLAYING_STATUS_TEXT, STATUS_SELECTOR } from "../test-shared/simSelectors.ts";

export const MOBILE_USE = {
  viewport: { width: 390, height: 844 },
  userAgent:
    "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Mobile/15E148 Safari/604.1",
  isMobile: true,
  hasTouch: true,
};

export const DESKTOP_USE = {
  viewport: { width: 1280, height: 720 },
  userAgent:
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
};

export async function installAudioSessionSpy(page: Page): Promise<void> {
  await page.addInitScript(() => {
    const log: string[] = [];
    (window as Window & { __audioSessionLog?: string[] }).__audioSessionLog = log;
    let current = "auto";
    Object.defineProperty(navigator, "audioSession", {
      configurable: true,
      value: {
        get type() {
          return current;
        },
        set type(next: string) {
          current = next;
          log.push(next);
        },
      },
    });
  });
}

export async function readAudioSessionLog(page: Page): Promise<string[]> {
  return page.evaluate(() => (window as Window & { __audioSessionLog?: string[] }).__audioSessionLog ?? []);
}

export async function installGetUserMediaMock(page: Page): Promise<void> {
  await page.addInitScript(() => {
    navigator.mediaDevices.getUserMedia = async () => {
      const ctx = new AudioContext();
      const osc = ctx.createOscillator();
      const dest = ctx.createMediaStreamDestination();
      osc.connect(dest);
      osc.start();
      return dest.stream;
    };
  });
}

export async function startSimAudio(page: Page): Promise<void> {
  await page.getByRole("button", { name: PLAY_LABEL }).click();
  await page.locator(STATUS_SELECTOR).filter({ hasText: PLAYING_STATUS_TEXT }).waitFor({ timeout: 45_000 });
}
