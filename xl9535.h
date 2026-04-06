#pragma once

#include <Wire.h>
#include "config.h"

// ======================================================
// XL9535 GPIO EXPANDER — Helpers
// I2C address: XL9535_ADDR (0x20)
// P0.0 = Display backlight, P0.1 = Camera reset
// P0.2 = BTN_B (active-low), P1.4 = BTN_A (active-low)
// P1.7 = SuccessLed (active-high, green)
// ======================================================

void enableDisplayBacklight() {
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t cfg = Wire.read();
  cfg &= ~0x01;
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06);
  Wire.write(cfg);
  Wire.endTransmission();

  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x02);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t out = Wire.read();
  out |= 0x01;
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x02);
  Wire.write(out);
  Wire.endTransmission();
}

void configureButtons() {
  // BTN_A = eP5_KeyA = enum index 12 → XL9535 pin 12 → Port 1, bit 4
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x07); // Config Port 1
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t cfg1 = Wire.read();
  cfg1 |= 0x10; // P1.4 = BTN_A input
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x07);
  Wire.write(cfg1);
  Wire.endTransmission();

  // BTN_B = eP11_KeyB = enum index 2 → XL9535 pin 2 → Port 0, bit 2
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06); // Config Port 0
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t cfg0 = Wire.read();
  cfg0 |= 0x04; // P0.2 = BTN_B input
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06);
  Wire.write(cfg0);
  Wire.endTransmission();
}

void configureSuccessLed() {
  // P1.7 = SuccessLed — configure as output
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x07); // Config Port 1
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t cfg1 = Wire.read();
  cfg1 &= ~0x80; // clear bit 7 = output
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x07);
  Wire.write(cfg1);
  Wire.endTransmission();
}

void setSuccessLed(bool on) {
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x03); // Output Port 1
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t out1 = Wire.read();
  if (on) out1 |= 0x80;
  else    out1 &= ~0x80;
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x03);
  Wire.write(out1);
  Wire.endTransmission();
}

bool readBtnA() {
  // BTN_A = eP5_KeyA (index 12) → XL9535 pin 12 → Port 1, bit 4
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x01); // Input Port 1
  Wire.endTransmission(true);
  if (Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1) == 0) return false;
  return !(Wire.read() & 0x10); // P1.4 — LOW = pressed (active-low)
}

bool readBtnB() {
  // BTN_B = eP11_KeyB (index 2) → XL9535 pin 2 → Port 0, bit 2
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x00); // Input Port 0
  Wire.endTransmission(true);
  if (Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1) == 0) return false;
  return !(Wire.read() & 0x04); // P0.2 — LOW = pressed (active-low)
}
