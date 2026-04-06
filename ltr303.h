#pragma once

#include <Wire.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// LTR-303 — Ambient Light Sensor
// I2C address: LTR303_ADDR (0x29), Bus: GPIO 47/48
// ======================================================

void showLTR303() {
  drawHeader("LTR-303");
  clearContent();
  int y = 32;
  char buf[32];

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("Ambient Light Sensor");
  y += 18;

  printRow(y, CP_ACCENT, "I2C Addr:", CP_VALUE, "0x29");
  printRow(y, CP_ACCENT, "SDA/SCL:",  CP_VALUE, "GPIO 47/48");
  y += 8;

  Wire.beginTransmission(LTR303_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    snprintf(buf, sizeof(buf), "NACK (err %d)", err);
    printRow(y, CP_ACCENT, "Status:", CP_ERR, buf);
    drawFooter(); return;
  }
  printRow(y, CP_ACCENT, "Status:", CP_OK, "ACK - FOUND");
  y += 12;

  Wire.beginTransmission(LTR303_ADDR);
  Wire.write(0x80); Wire.write(0x01);
  Wire.endTransmission();
  delay(120);

  Wire.beginTransmission(LTR303_ADDR);
  Wire.write(0x88);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)LTR303_ADDR, (uint8_t)4);
  uint16_t ch1 = Wire.read() | ((uint16_t)Wire.read() << 8);
  uint16_t ch0 = Wire.read() | ((uint16_t)Wire.read() << 8);

  float lux = 0.0f;
  if ((ch0 + ch1) > 0) {
    float ratio = (float)ch1 / (float)(ch0 + ch1);
    if      (ratio < 0.45f) lux = 1.7743f * ch0 + 1.1059f * ch1;
    else if (ratio < 0.64f) lux = 4.2785f * ch0 - 1.9548f * ch1;
    else if (ratio < 0.85f) lux = 0.5926f * ch0 + 0.1185f * ch1;
    if (lux < 0) lux = 0;
  }

  tft.setTextSize(3);
  tft.setTextColor(CP_BIG);
  tft.setCursor(5, y);
  dtostrf(lux, 7, 1, buf); tft.print(buf);
  tft.setTextSize(2); tft.print(" lux");
  y += 44;

  tft.setTextSize(1);
  tft.setTextColor(CP_VALUE);
  snprintf(buf, sizeof(buf), "CH0 Vis+IR: %5u counts", ch0);
  tft.setCursor(5, y); tft.print(buf); y += 14;
  snprintf(buf, sizeof(buf), "CH1 IR:     %5u counts", ch1);
  tft.setCursor(5, y); tft.print(buf); y += 14;

  tft.setTextColor(CP_DIM);
  float ratio = (ch0 + ch1 > 0) ? (float)ch1 / (float)(ch0 + ch1) : 0.0f;
  snprintf(buf, sizeof(buf), "IR Ratio: %.3f", ratio);
  tft.setCursor(5, y); tft.print(buf);

  drawFooter();
}
