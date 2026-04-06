#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"
#include "globals.h"

// ======================================================
// CYBERPUNK PALETTE
// ======================================================
#define CP_BG        0x0000   // black background
#define CP_HEADER_BG 0x2009   // deep dark purple header
#define CP_FOOTER_BG 0x2104   // dark charcoal footer
#define CP_TITLE     0xF81F   // hot magenta  — header titles, accent lines
#define CP_ACCENT    0x07FF   // electric cyan — labels, section text
#define CP_VALUE     0xFFFF   // white         — row values
#define CP_BIG       0xFFE0   // neon yellow   — large primary readings
#define CP_OK        0x07E0   // neon green    — success / active states
#define CP_ERR       0xF800   // red           — errors
#define CP_DIM       0x8410   // medium grey   — secondary / inactive text

// ======================================================
// DISPLAY HELPERS — TFT UI primitives
// Shared by all component drivers.
// Font sizes used across all screens:
//   1 = labels, metadata, footer, hints
//   2 = medium values, axis labels, button labels
//   3 = large primary readings
// ======================================================

void drawHeader(const char* title) {
  tft.fillRect(0, 0, 240, 24, CP_HEADER_BG);
  tft.drawFastHLine(0, 23, 240, CP_TITLE);
  tft.setTextColor(CP_TITLE);
  tft.setTextSize(2);
  tft.setCursor(5, 4);
  tft.print(title);
  char badge[16];
  snprintf(badge, sizeof(badge), "%d/%d", currentTest + 1, NUM_TESTS);
  tft.setTextSize(1);
  tft.setTextColor(CP_DIM);
  tft.setCursor(205, 8);
  tft.print(badge);
}

void drawFooter() {
  tft.drawFastHLine(0, 307, 240, CP_TITLE);
  tft.fillRect(0, 308, 240, 12, CP_FOOTER_BG);
  tft.setTextColor(CP_DIM);
  tft.setTextSize(1);
  tft.setCursor(5, 310);
  tft.print("A: NEXT   B: PREV");
}

void clearContent() {
  tft.fillRect(0, 24, 240, 284, CP_BG);
}

void printRow(int& y, uint16_t labelColor, const char* label,
              uint16_t valColor, const char* val) {
  tft.setTextSize(1);
  tft.setTextColor(labelColor);
  tft.setCursor(5, y);
  tft.print(label);
  tft.setTextColor(valColor);
  tft.setCursor(100, y);
  tft.print(val);
  y += 14;
}

// Draw a horizontal bar at (x,y), total width w, height h, fill 0.0-1.0
void drawBar(int x, int y, int w, int h, float fill) {
  uint16_t color;
  if      (fill < 0.6f)  color = CP_OK;
  else if (fill < 0.85f) color = CP_BIG;
  else                   color = CP_ERR;

  int fillPx = (int)(fill * w);
  tft.fillRect(x, y, fillPx, h, color);
  tft.fillRect(x + fillPx, y, w - fillPx, h, CP_FOOTER_BG);
  tft.drawRect(x, y, w, h, CP_DIM);
}
