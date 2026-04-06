#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// Speaker — NS4168 Amplifier via I2S TX
// 440 Hz sine wave, volume fades from silent to full over ~8 seconds
// I2S helpers: i2sInstallSpeaker, fillSpeakerBuffer (in i2s_audio.h)
// ======================================================

void showSpeaker() {
  const float volFrac = spkrCurrentAmp;                   // already 0..1 (linear ramp fraction)
  const int   volPct  = (int)(volFrac * 100.0f + 0.5f);

  drawHeader("Speaker");
  clearContent();
  int y = 32;
  char buf[32];

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("NS4168 Amp (I2S TX)");
  y += 18;

  printRow(y, CP_ACCENT, "BCLK:",  CP_VALUE, "GPIO 0");
  printRow(y, CP_ACCENT, "LRCK:",  CP_VALUE, "GPIO 38 (=FONT_CS)");
  printRow(y, CP_ACCENT, "MCLK:",  CP_VALUE, "GPIO 3");
  printRow(y, CP_ACCENT, "DOUT:",  CP_VALUE, "GPIO 45");
  y += 4;

  printRow(y, CP_ACCENT, "Tone:",        CP_VALUE, "440 Hz (A4)");
  printRow(y, CP_ACCENT, "Sample rate:", CP_VALUE, "44100 Hz, 16-bit");
  y += 12;

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("Volume:");
  y += 14;

  canvas.setTextSize(3);
  canvas.setTextColor(CP_BIG);
  snprintf(buf, sizeof(buf), " %3d%%", volPct);
  canvas.setCursor(5, y);
  canvas.print(buf);
  y += 40;

  drawBar(5, y, 220, 24, volFrac);
  y += 34;

  canvas.setTextSize(1);
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y);
  canvas.print("Fades in over 30s (dB-linear scale)");
  y += 14;
  canvas.setCursor(5, y);
  canvas.print("Listen for tone on speaker");

  drawFooter();
}
