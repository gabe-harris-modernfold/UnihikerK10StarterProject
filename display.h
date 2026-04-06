#pragma once

#include "lgfx_config.h"
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
  canvas.fillRect(0, 0, 240, 24, CP_HEADER_BG);
  canvas.drawFastHLine(0, 23, 240, CP_TITLE);
  canvas.setTextColor(CP_TITLE);
  canvas.setTextSize(2);
  canvas.setCursor(5, 4);
  canvas.print(title);
  char badge[16];
  snprintf(badge, sizeof(badge), "%d/%d", currentTest + 1, NUM_TESTS);
  canvas.setTextSize(1);
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(205, 8);
  canvas.print(badge);
}

void drawFooter() {
  canvas.drawFastHLine(0, 307, 240, CP_TITLE);
  canvas.fillRect(0, 308, 240, 12, CP_FOOTER_BG);
  canvas.setTextColor(CP_DIM);
  canvas.setTextSize(1);
  canvas.setCursor(5, 310);
  canvas.print("A: NEXT   B: PREV");
}

void clearContent() {
  canvas.fillRect(0, 24, 240, 284, CP_BG);
}

void printRow(int& y, uint16_t labelColor, const char* label,
              uint16_t valColor, const char* val) {
  canvas.setTextSize(1);
  canvas.setTextColor(labelColor);
  canvas.setCursor(5, y);
  canvas.print(label);
  canvas.setTextColor(valColor);
  canvas.setCursor(100, y);
  canvas.print(val);
  y += 14;
}

// Draw a horizontal bar at (x,y), total width w, height h, fill 0.0-1.0
void drawBar(int x, int y, int w, int h, float fill) {
  uint16_t color;
  if      (fill < 0.6f)  color = CP_OK;
  else if (fill < 0.85f) color = CP_BIG;
  else                   color = CP_ERR;

  int fillPx = (int)(fill * w);
  canvas.fillRect(x, y, fillPx, h, color);
  canvas.fillRect(x + fillPx, y, w - fillPx, h, CP_FOOTER_BG);
  canvas.drawRect(x, y, w, h, CP_DIM);
}
