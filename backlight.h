#pragma once

#include "display.h"
#include "globals.h"
#include "xl9535.h"

// ======================================================
// Backlight — XL9535 P0.0 → LCD_BLK → NPN Q4 → LED+
// Demonstrates digital on/off control via I2C expander.
// Auto-cycles: ON 3 s → OFF 2 s → repeat.
// ======================================================

void showBacklight() {
  drawHeader("Backlight");
  clearContent();
  int y = 32;

  canvas.setTextSize(1);
  canvas.setTextColor(CP_ACCENT);
  canvas.setCursor(5, y); canvas.print("XL9535 P0.0  (active-high)");
  y += 14;
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y); canvas.print("NPN Q4 -> LED+ string");
  y += 20;

  canvas.setTextSize(3);
  canvas.setTextColor(backlightOn ? CP_OK : CP_ERR);
  canvas.setCursor(5, y);
  canvas.print(backlightOn ? "ON " : "OFF");
  y += 32;

  canvas.setTextSize(1);
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y);
  canvas.print(backlightOn ? "-> OFF in 3s" : "-> ON  in 2s");
  y += 20;

  drawFooter();
}
