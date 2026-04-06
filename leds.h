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
    ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, ILI9341_WHITE, CP_FOOTER_BG
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

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("3x WS2812 + SuccessLed");
  y += 18;
  printRow(y, CP_ACCENT, "Data Pin:", CP_VALUE, "GPIO 46");
  printRow(y, CP_ACCENT, "Count:",    CP_VALUE, "3 RGB + 1 mono");
  y += 12;

  tft.setTextSize(3);
  tft.setTextColor(stepTFT[ledCycleStep]);
  int16_t tw = strlen(stepNames[ledCycleStep]) * 18;
  tft.setCursor((240 - tw) / 2, y);
  tft.print(stepNames[ledCycleStep]);
  y += 46;

  for (int i = 0; i < 4; i++) {
    int cx = 30 + i * 60;
    uint16_t color = (i < LED_COUNT) ? stepTFT[ledCycleStep]
                                      : (ledCycleStep == 4 ? CP_FOOTER_BG : CP_OK);
    tft.fillCircle(cx, y + 20, 22, color);
    tft.drawCircle(cx, y + 20, 22, CP_DIM);
  }
  y += 58;

  tft.setTextSize(1);
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print("Changes every 1s");
  y += 14;
  tft.setCursor(5, y); tft.print("Verify on physical LEDs");

  drawFooter();
}
