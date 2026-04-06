#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// Speaker — NS4168 Amplifier via I2S TX
// 440 Hz sine wave, auto-cycling volume 20–100%
// I2S helpers: i2sInstallSpeaker, fillSpeakerBuffer (in i2s_audio.h)
// ======================================================

void showSpeaker() {
  static const float volLevels[]  = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
  static const int   volPct[]     = { 20, 40, 60, 80, 100 };

  drawHeader("Speaker");
  clearContent();
  int y = 32;
  char buf[32];

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("NS4168 Amp (I2S TX)");
  y += 18;

  printRow(y, CP_ACCENT, "BCLK:",  CP_VALUE, "GPIO 0");
  printRow(y, CP_ACCENT, "LRCK:",  CP_VALUE, "GPIO 38 (=FONT_CS)");
  printRow(y, CP_ACCENT, "MCLK:",  CP_VALUE, "GPIO 3");
  printRow(y, CP_ACCENT, "DOUT:",  CP_VALUE, "GPIO 45");
  y += 4;

  printRow(y, CP_ACCENT, "Tone:",        CP_VALUE, "440 Hz (A4)");
  printRow(y, CP_ACCENT, "Sample rate:", CP_VALUE, "44100 Hz, 16-bit");
  y += 12;

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("Volume:");
  y += 14;

  tft.setTextSize(3);
  tft.setTextColor(CP_BIG);
  snprintf(buf, sizeof(buf), " %3d%%", volPct[spkrVolStep]);
  tft.setCursor(5, y);
  tft.print(buf);
  y += 40;

  drawBar(5, y, 220, 24, volLevels[spkrVolStep]);
  y += 34;

  tft.setTextSize(1);
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y);
  tft.print("Auto-cycles 20->100% (2s each)");
  y += 14;
  tft.setCursor(5, y);
  tft.print("Listen for tone on speaker");

  drawFooter();
}
