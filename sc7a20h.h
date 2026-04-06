#pragma once

#include <Wire.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// SC7A20H — 3-Axis Accelerometer
// I2C address: SC7A20H_ADDR (0x19), Bus: GPIO 47/48
// ======================================================

void showSC7A20H() {
  drawHeader("SC7A20H");
  clearContent();
  int y = 32;
  char buf[40];

  tft.setTextColor(CP_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(5, y);
  tft.print("3-Axis Accelerometer");
  y += 18;

  printRow(y, CP_ACCENT, "I2C Addr:", CP_VALUE, "0x19");
  printRow(y, CP_ACCENT, "SDA/SCL:",  CP_VALUE, "GPIO 47/48");
  y += 8;

  Wire.beginTransmission(SC7A20H_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    snprintf(buf, sizeof(buf), "NACK (err %d)", err);
    printRow(y, CP_ACCENT, "Status:", CP_ERR, buf);
    drawFooter(); return;
  }
  printRow(y, CP_ACCENT, "Status:", CP_OK, "ACK - FOUND");

  Wire.beginTransmission(SC7A20H_ADDR);
  Wire.write(0x0F);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SC7A20H_ADDR, (uint8_t)1);
  uint8_t whoAmI = Wire.read();

  snprintf(buf, sizeof(buf), "0x%02X %s", whoAmI, whoAmI == 0x11 ? "(OK)" : "(unexpected)");
  printRow(y, CP_ACCENT, "WHO_AM_I:",
           whoAmI == 0x11 ? CP_OK : CP_ERR, buf);
  y += 10;

  Wire.beginTransmission(SC7A20H_ADDR);
  Wire.write(0x20); Wire.write(0x57); // CTRL_REG1: 100Hz, XYZ on
  Wire.endTransmission();
  Wire.beginTransmission(SC7A20H_ADDR);
  Wire.write(0x23); Wire.write(0x80); // CTRL_REG4: BDU, ±2g
  Wire.endTransmission();
  delay(20);

  Wire.beginTransmission(SC7A20H_ADDR);
  Wire.write(0x28 | 0x80);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SC7A20H_ADDR, (uint8_t)6);
  int16_t rawX = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawY = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawZ = (int16_t)(Wire.read() | (Wire.read() << 8));

  float gX = rawX / 16384.0f;
  float gY = rawY / 16384.0f;
  float gZ = rawZ / 16384.0f;

  tft.setTextSize(2);
  tft.setTextColor(CP_ERR);
  tft.setCursor(5, y);
  dtostrf(gX, 7, 3, buf); tft.print("X: "); tft.print(buf); tft.print(" g");
  y += 28;

  tft.setTextColor(CP_OK);
  tft.setCursor(5, y);
  dtostrf(gY, 7, 3, buf); tft.print("Y: "); tft.print(buf); tft.print(" g");
  y += 28;

  tft.setTextColor(CP_ACCENT);
  tft.setCursor(5, y);
  dtostrf(gZ, 7, 3, buf); tft.print("Z: "); tft.print(buf); tft.print(" g");
  y += 34;

  tft.setTextSize(1);
  tft.setTextColor(CP_DIM);
  snprintf(buf, sizeof(buf), "Raw  X:%6d  Y:%6d  Z:%6d", rawX, rawY, rawZ);
  tft.setCursor(5, y); tft.print(buf);

  drawFooter();
}
