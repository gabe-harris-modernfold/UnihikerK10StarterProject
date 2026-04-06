#pragma once

#include <Wire.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// XL9535 — GPIO Expander Register Probe & Button Display
// I2C address: XL9535_ADDR (0x20), Bus: GPIO 47/48
// Button driver functions are in xl9535.h
// ======================================================

void showXL9535() {
  drawHeader("XL9535");
  clearContent();
  int y = 32;
  char buf[32];

  printRow(y, CP_ACCENT, "I2C Addr:", CP_VALUE, "0x20");
  printRow(y, CP_ACCENT, "SDA/SCL:",  CP_VALUE, "GPIO 47/48");
  y += 8;

  Wire.beginTransmission(XL9535_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    snprintf(buf, sizeof(buf), "NACK (err %d)", err);
    printRow(y, CP_ACCENT, "Status:", CP_ERR, buf);
    drawFooter(); return;
  }
  printRow(y, CP_ACCENT, "Status:", CP_OK, "ACK - FOUND");
  y += 12;

  // Read input registers
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)2);
  uint8_t in0 = Wire.read();
  uint8_t in1 = Wire.read();

  // BTN_A: Port1 bit4 (P1.4), BTN_B: Port0 bit2 (P0.2) — active-low
  bool btnApressed = (in1 & 0x10) == 0;
  bool btnBpressed = (in0 & 0x04) == 0;

  tft.setTextSize(2);
  tft.setTextColor(CP_ACCENT);
  tft.setCursor(5, y); tft.print("BTN A:");
  y += 4;
  tft.setTextSize(3);
  tft.setTextColor(btnApressed ? CP_OK : CP_DIM);
  tft.setCursor(20, y + 4); tft.print(btnApressed ? "PRESSED" : "idle");
  y += 40;

  tft.setTextSize(2);
  tft.setTextColor(CP_ACCENT);
  tft.setCursor(5, y); tft.print("BTN B:");
  y += 4;
  tft.setTextSize(3);
  tft.setTextColor(btnBpressed ? CP_OK : CP_DIM);
  tft.setCursor(20, y + 4); tft.print(btnBpressed ? "PRESSED" : "idle");
  y += 44;

  // Register detail
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)2);
  uint8_t cfg0 = Wire.read();
  uint8_t cfg1 = Wire.read();

  tft.drawFastHLine(0, y, 240, CP_ACCENT);
  y += 8;

  snprintf(buf, sizeof(buf), "0x%02X  dir:0x%02X", in0, cfg0);
  printRow(y, CP_ACCENT, "Port 0:", CP_VALUE, buf);
  snprintf(buf, sizeof(buf), "0x%02X  dir:0x%02X", in1, cfg1);
  printRow(y, CP_ACCENT, "Port 1:", CP_VALUE, buf);
  y += 4;

  tft.setTextSize(1);
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print("P0.0=BLK P0.1=CAM P0.2=B P1.4=A P1.7=OK");

  drawFooter();
}
