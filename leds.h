#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// WS2812 RGB LEDs — 3x NeoPixel on GPIO 46
// SuccessLed    — XL9535 P1.7 (active-high, green)
// State: ledCycleStep (0=RED, 1=GREEN, 2=BLUE, 3=WHITE, 4=OFF)
// Cycle timer managed in loop()
// ======================================================

void showLEDs() {
  static const char*    stepNames[] = { "RED", "GREEN", "BLUE", "WHITE", "OFF" };
  static const uint16_t stepTFT[]   = {
    CP_ERR, CP_OK, 0x001F, CP_VALUE, CP_FOOTER_BG
  };
  uint32_t stepColor;
  switch (ledCycleStep) {
    case 0: stepColor = leds.Color(200,   0,   0); break;
    case 1: stepColor = leds.Color(  0, 200,   0); break;
    case 2: stepColor = leds.Color(  0,   0, 200); break;
    case 3: stepColor = leds.Color(200, 200, 200); break;
    default:stepColor = leds.Color(  0,   0,   0); break;
  }

  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, stepColor);
  leds.show();
  setSuccessLed(ledCycleStep != 4);

  drawHeader("WS2812 LEDs");
  clearContent();
  int y = 32;

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("3x WS2812 + SuccessLed");
  y += 18;
  printRow(y, CP_ACCENT, "Data Pin:", CP_VALUE, "GPIO 46");
  printRow(y, CP_ACCENT, "Count:",    CP_VALUE, "3 RGB + 1 mono");
  y += 12;

  canvas.setTextSize(3);
  canvas.setTextColor(stepTFT[ledCycleStep]);
  int16_t tw = strlen(stepNames[ledCycleStep]) * 18;
  canvas.setCursor((240 - tw) / 2, y);
  canvas.print(stepNames[ledCycleStep]);
  y += 46;

  for (int i = 0; i < 4; i++) {
    int cx = 30 + i * 60;
    uint16_t color = (i < LED_COUNT) ? stepTFT[ledCycleStep]
                                      : (ledCycleStep == 4 ? CP_FOOTER_BG : CP_OK);
    canvas.fillCircle(cx, y + 20, 22, color);
    canvas.drawCircle(cx, y + 20, 22, CP_DIM);
  }
  y += 58;

  canvas.setTextSize(1);
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y); canvas.print("Changes every 1s");
  y += 14;
  canvas.setCursor(5, y); canvas.print("Verify on physical LEDs");

  drawFooter();
}
