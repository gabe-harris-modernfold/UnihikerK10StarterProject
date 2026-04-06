#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// Microphone — 2x MSM381ACT001 MEMS mics via ES7243E codec → I2S RX
// 16 kHz 16-bit stereo. VU meter with RMS + peak hold dBFS.
// I2S helpers: i2sInstallMic, readMicBuffer (in i2s_audio.h)
// ======================================================

void showMic() {
  drawHeader("Mic Test");
  clearContent();
  int y = 32;
  char buf[32];

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("2x MSM381ACT001 -> ES7243E -> I2S");
  y += 18;

  printRow(y, CP_ACCENT, "Codec:",  CP_VALUE, "ES7243E (I2C 0x15)");
  printRow(y, CP_ACCENT, "MCLK:",   CP_VALUE, "GPIO 3");
  printRow(y, CP_ACCENT, "BCLK:",   CP_VALUE, "GPIO 0");
  printRow(y, CP_ACCENT, "LRCK:",   CP_VALUE, "GPIO 38");
  printRow(y, CP_ACCENT, "SDOUT:",  CP_VALUE, "GPIO 39  16kHz 16-bit");
  y += 10;

  // dBFS bar: maps MIC_FLOOR_DB..0 dB → 0.0..1.0
  auto dbToFill = [](float db) -> float {
    return (db - MIC_FLOOR_DB) / (-MIC_FLOOR_DB);
  };

  // MIC L
  tft.setTextColor(CP_VALUE);
  tft.setCursor(5, y); tft.print("MIC L (Left channel)");
  y += 14;
  drawBar(5, y, 220, 20, dbToFill(micDbL));
  y += 26;
  snprintf(buf, sizeof(buf), "RMS:%5.1fdB  Pk:%5.1fdB", micDbL, micPeakDbL);
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print(buf);
  y += 20;

  // MIC R
  tft.setTextColor(CP_VALUE);
  tft.setCursor(5, y); tft.print("MIC R (Right channel)");
  y += 14;
  drawBar(5, y, 220, 20, dbToFill(micDbR));
  y += 26;
  snprintf(buf, sizeof(buf), "RMS:%5.1fdB  Pk:%5.1fdB", micDbR, micPeakDbR);
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print(buf);
  y += 20;

  // Scale reference
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y);   tft.print("-50dB");
  tft.setCursor(95, y);  tft.print("-25");
  tft.setCursor(210, y); tft.print("0");
  y += 14;

  // Diagnostics
  tft.setTextColor(CP_ACCENT);
  snprintf(buf, sizeof(buf), "inst:%d pin:%d rd:%d", micInstallErr, micPinErr, micReadErr);
  tft.setCursor(5, y); tft.print(buf); y += 12;
  snprintf(buf, sizeof(buf), "bytes:%d max:%ld", (int)micLastBytes, (long)micRawMax);
  tft.setCursor(5, y); tft.print(buf); y += 12;
  snprintf(buf, sizeof(buf), "[%d %d %d %d]", micSamp[0], micSamp[1], micSamp[2], micSamp[3]);
  tft.setCursor(5, y); tft.print(buf); y += 12;
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print("Speak or clap. Peak holds until navigate.");

  drawFooter();
}
