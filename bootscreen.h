#pragma once

#include "lgfx_config.h"
#include "globals.h"
#include "xl9535.h"

// ======================================================
// BOOT SCREEN — fade "UNIHIKER K10" from black to white
// over 5 seconds, then wait for a button press.
// Button press during the fade skips straight to the end.
// ======================================================

void runBootScreen() {
  const unsigned long FADE_MS = 5000UL;
  unsigned long start = millis();
  bool fadeComplete = false;

  while (true) {
    unsigned long elapsed = millis() - start;

    // Fraction 0.0 → 1.0 over FADE_MS
    float frac = (float)elapsed / (float)FADE_MS;
    if (frac >= 1.0f) { frac = 1.0f; fadeComplete = true; }

    // Interpolate RGB565: black (0x0000) → white (0xFFFF)
    uint8_t r = (uint8_t)(31.0f * frac);   // 5-bit red
    uint8_t g = (uint8_t)(63.0f * frac);   // 6-bit green
    uint8_t b = (uint8_t)(31.0f * frac);   // 5-bit blue
    uint16_t textColor = ((uint16_t)r << 11) | ((uint16_t)g << 5) | b;

    // Draw frame
    canvas.fillScreen(0x0000);
    canvas.setTextSize(2);
    canvas.setTextColor(textColor);

    // Centre the string at runtime so it survives any font change
    int tw = canvas.textWidth("UNIHIKER K10");
    int th = canvas.fontHeight();
    canvas.setCursor((240 - tw) / 2, (320 - th) / 2 - 10);
    canvas.print("UNIHIKER K10");
    canvas.pushSprite(0, 0);

    // Allow button press to skip the fade; require it once fade is done
    bool btnA = readBtnA();
    bool btnB = readBtnB();
    if (btnA || btnB) {
      // Wait for full release so loop()'s edge detector starts clean
      while (readBtnA() || readBtnB()) delay(10);
      break;
    }

    if (fadeComplete) {
      // Fade done — just wait for button, no need to keep redrawing
      delay(20);
    } else {
      delay(16);  // ~60 fps during fade
    }
  }

  // Prime edge-detector state so loop() doesn't see a ghost press
  prevBtnA = readBtnA();
  prevBtnB = readBtnB();
}
