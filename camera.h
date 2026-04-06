#pragma once

#include <Wire.h>
#include "esp_camera.h"
#include "driver/i2c.h"
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// GC2145 Camera — DVP parallel interface, live preview
// HQVGA (240x176) RGB565 via esp_camera driver
// Reset via XL9535 P0.1 (active-low)
// ======================================================

// Assert/release CAM_RST via XL9535 P0.1
void camSetReset(bool asserted) {
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06); Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t cfg = Wire.read();
  cfg &= ~0x02;
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x06); Wire.write(cfg); Wire.endTransmission();

  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x02); Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)XL9535_ADDR, (uint8_t)1);
  uint8_t out = Wire.read();
  // CAM_RST is active-low: asserted = LOW, released = HIGH
  if (asserted) out &= ~0x02; else out |= 0x02;
  Wire.beginTransmission(XL9535_ADDR);
  Wire.write(0x02); Wire.write(out); Wire.endTransmission();
}

bool cameraInit() {
  // esp_camera takes over I2C — release Wire first
  Wire.end();

  // Reset pulse: assert → start XCLK → release → wait
  // (Wire is ended so XL9535 calls won't work here; do reset before Wire.end)
  delay(10);

  camera_config_t config;
  config.ledc_channel   = LEDC_CHANNEL_0;
  config.ledc_timer     = LEDC_TIMER_0;
  config.pin_d0         = CAM_D0;
  config.pin_d1         = CAM_D1;
  config.pin_d2         = CAM_D2;
  config.pin_d3         = CAM_D3;
  config.pin_d4         = CAM_D4;
  config.pin_d5         = CAM_D5;
  config.pin_d6         = CAM_D6;
  config.pin_d7         = CAM_D7;
  config.pin_xclk       = CAM_XCLK;
  config.pin_pclk       = CAM_PCLK;
  config.pin_vsync      = CAM_VSYNC;
  config.pin_href       = CAM_HREF;
  config.pin_sscb_sda   = I2C_SDA;      // GPIO 47
  config.pin_sscb_scl   = I2C_SCL;      // GPIO 48
  config.pin_pwdn       = -1;           // no PWDN pin
  config.pin_reset      = -1;           // reset handled via XL9535 before init
  config.xclk_freq_hz   = 20000000;
  config.pixel_format   = PIXFORMAT_RGB565;
  config.frame_size     = FRAMESIZE_HQVGA; // 240x176 — fits TFT width
  config.jpeg_quality   = 10;
  config.fb_count       = 1;
  config.fb_location    = CAMERA_FB_IN_PSRAM;
  config.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Wire.begin(I2C_SDA, I2C_SCL);
    return false;
  }

  // Configure camera orientation before deleting the I2C driver
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_vflip(s, 1);    // Flip image vertically
    s->set_hmirror(s, 1);  // Mirror horizontally
  }

  // Streaming is DVP/DMA — SCCB not needed after init.
  // Delete the camera's I2C driver so Wire can reclaim the bus for XL9535 buttons.
  i2c_driver_delete(I2C_NUM_0);
  Wire.begin(I2C_SDA, I2C_SCL);
  return true;
}

void cameraStop() {
  Wire.end();                    // release bus before deinit reinstalls I2C driver
  esp_camera_deinit();
  i2c_driver_delete(I2C_NUM_0); // clean up camera's I2C driver
  cameraInitialized = false;
  Wire.begin(I2C_SDA, I2C_SCL);
}

// Draws one camera frame onto the TFT starting at y=24 (below header)
void cameraDrawFrame() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return;

  // HQVGA = 240x176, RGB565 — fits exactly in portrait width
  // Centre vertically in content area (308px tall): top margin = (284-176)/2 = 54
  int imgY = 24 + 54;
  tft.startWrite();
  tft.setAddrWindow(0, imgY, fb->width, fb->height);
  // esp_camera RGB565 is big-endian (MSB first) — pass bigEndian=true to skip swap
  tft.writePixels((uint16_t*)fb->buf, fb->width * fb->height, true, true);
  tft.endWrite();

  esp_camera_fb_return(fb);
}

void showCamera() {
  drawHeader("GC2145 Camera");
  clearContent();
  int y = 32;

  if (cameraInitialized) {
    // Just redraw static labels — frame updates happen in loop()
    tft.setTextSize(1);
    tft.setTextColor(CP_OK);
    tft.setCursor(5, y); tft.print("LIVE  240x176 RGB565");
    y += 14;
    tft.setTextColor(CP_DIM);
    tft.setCursor(5, y); tft.print("HQVGA  20 MHz XCLK  DVP 8-bit");
    return; // frame drawn by loop()
  }

  tft.setTextSize(1);
  tft.setTextColor(CP_ACCENT);
  tft.setCursor(5, y); tft.print("Initialising camera..."); y += 16;
  tft.setTextColor(CP_DIM);
  tft.setCursor(5, y); tft.print("RST pulse + esp_camera_init"); y += 16;
  drawFooter();

  // Perform reset pulse while Wire is still available
  camSetReset(true);  delay(10);
  camSetReset(false); delay(100);

  if (cameraInit()) {
    cameraInitialized = true;
    // Redraw with live status
    clearContent();
    y = 32;
    tft.setTextSize(1);
    tft.setTextColor(CP_OK);
    tft.setCursor(5, y); tft.print("LIVE  240x176 RGB565");
    y += 14;
    tft.setTextColor(CP_DIM);
    tft.setCursor(5, y); tft.print("HQVGA  20 MHz XCLK  DVP 8-bit");
  } else {
    clearContent();
    y = 32;
    tft.setTextSize(1);
    tft.setTextColor(CP_ERR);
    tft.setCursor(5, y); tft.print("esp_camera_init FAILED"); y += 16;
    tft.setTextColor(CP_DIM);
    tft.setCursor(5, y); tft.print("Check flex cable seating"); y += 12;
    tft.setCursor(5, y); tft.print("and power-cycle the board.");
  }

  drawFooter();
}
