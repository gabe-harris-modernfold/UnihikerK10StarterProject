#pragma once

#include <Wire.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// AHT20 — Temperature / Humidity Sensor
// I2C address: AHT20_ADDR (0x38), Bus: GPIO 47/48
// ======================================================

void showAHT20() {
  drawHeader("AHT20");
  clearContent();
  int y = 32;
  char buf[32];

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("Temp / Humidity Sensor");
  y += 18;

  printRow(y, CP_ACCENT, "I2C Addr:", CP_VALUE, "0x38");
  printRow(y, CP_ACCENT, "SDA/SCL:",  CP_VALUE, "GPIO 47/48");
  y += 8;

  Wire.beginTransmission(AHT20_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    snprintf(buf, sizeof(buf), "NACK (err %d)", err);
    printRow(y, CP_ACCENT, "Status:", CP_ERR, buf);
    drawFooter(); return;
  }
  printRow(y, CP_ACCENT, "Status:", CP_OK, "ACK - FOUND");
  y += 12;

  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
  Wire.endTransmission();
  // AHT20 datasheet §8.4: measurement completes in ≤75 ms; 80 ms adds margin
  delay(80);

  uint8_t got = Wire.requestFrom((uint8_t)AHT20_ADDR, (uint8_t)6);
  if (got < 6) {
    printRow(y, CP_ACCENT, "Read:", CP_ERR, "short read");
    drawFooter(); return;
  }
  uint8_t data[6];
  for (int i = 0; i < 6; i++) data[i] = Wire.read();

  uint32_t rawHum  = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
  uint32_t rawTemp = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
  float humidity = (float)rawHum  / 1048576.0f * 100.0f;
  float tempC    = (float)rawTemp / 1048576.0f * 200.0f - 50.0f;
  float tempF    = tempC * 9.0f / 5.0f + 32.0f;

  canvas.setTextSize(3);
  canvas.setTextColor(CP_BIG);
  canvas.setCursor(5, y);
  dtostrf(tempC, 5, 1, buf); canvas.print(buf);
  canvas.setTextSize(2); canvas.print("\xF7""C");
  y += 40;

  canvas.setTextSize(2);
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y);
  dtostrf(tempF, 5, 1, buf); canvas.print(buf); canvas.print("\xF7""F");
  y += 30;

  canvas.setTextSize(3);
  canvas.setTextColor(CP_ACCENT);
  canvas.setCursor(5, y);
  dtostrf(humidity, 5, 1, buf); canvas.print(buf);
  canvas.setTextSize(2); canvas.print(" %RH");
  y += 42;

  canvas.setTextSize(1);
  canvas.setTextColor(CP_DIM);
  snprintf(buf, sizeof(buf), "Raw hum:  0x%05lX", (unsigned long)rawHum);
  canvas.setCursor(5, y); canvas.print(buf); y += 12;
  snprintf(buf, sizeof(buf), "Raw temp: 0x%05lX", (unsigned long)rawTemp);
  canvas.setCursor(5, y); canvas.print(buf);

  drawFooter();
}
