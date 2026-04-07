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

// Assert CAM_RST low for ≥1 ms (GC2145 datasheet), then release and wait
// 100 ms for the sensor's internal PLL to lock before SCCB access.
void cameraResetPulse() {
  camSetReset(true);  delay(10);   // hold reset (10 ms >> 1 ms min)
  camSetReset(false); delay(100);  // PLL lock time
}

bool cameraInit() {
  // esp_camera takes over I2C — release Wire first.
  // Reset pulse was performed by the caller (showCamera) while Wire was live.
  Wire.end();

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

// Draws one camera frame onto the TFT starting at y=24 (below header).
// Uses LovyanGFX pushPixelsDMA() for non-blocking transfer: after the call
// returns the DMA controller owns the pixel data and the CPU is free.
// prevFb is held across calls and returned only after waitDMA() confirms the
// previous transfer is complete, preventing a use-after-free on the PSRAM buffer.
void cameraDrawFrame() {
  static camera_fb_t* prevFb = nullptr;

  // Wait for the previous DMA transfer to finish before releasing its buffer.
  tft.waitDMA();
  if (prevFb) { esp_camera_fb_return(prevFb); prevFb = nullptr; }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return;

  // HQVGA = 240x176, RGB565 — fits exactly in portrait width.
  // Centre vertically in content area (308px tall): top margin = (284-176)/2 = 54
  int imgY = 24 + 54;
  tft.startWrite();
  tft.setAddrWindow(0, imgY, fb->width, fb->height);
  // esp_camera RGB565 is big-endian (high byte first). lgfx::swap565_t matches
  // this byte order; rgb565_t would swap bytes within each pixel → garish colors.
  // pushPixelsDMA() returns immediately; DMA runs in background.
  tft.pushPixelsDMA((lgfx::swap565_t*)fb->buf, fb->width * fb->height);
  tft.endWrite();

  prevFb = fb;  // keep alive until next call's waitDMA()
}

// FreeRTOS camera task — pinned to Core 1, priority 2.
// Holds displayMutex for the duration of each frame write;
// re-checks cameraInitialized under the mutex to avoid calling
// esp_camera_fb_get() after a concurrent cameraStop().
void cameraTaskFn(void* pvParameters) {
  for (;;) {
    if (currentTest == 5) {
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (cameraInitialized) {
          cameraDrawFrame();  // esp_camera_fb_get() yields CPU while waiting
        }
        xSemaphoreGive(displayMutex);
      }
      vTaskDelay(pdMS_TO_TICKS(1));  // block camera task so loop task (priority 1) can run
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));  // sleep when not on camera screen
    }
  }
}

void showCamera() {
  drawHeader("GC2145 Camera");
  clearContent();
  int y = 32;

  if (cameraInitialized) {
    // Just redraw static labels — frame updates happen via DMA in cameraTaskFn()
    canvas.setTextSize(1);
    canvas.setTextColor(CP_OK);
    canvas.setCursor(5, y); canvas.print("LIVE  240x176 RGB565");
    y += 14;
    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("HQVGA  20 MHz XCLK  DVP 8-bit");
    return; // canvas pushed by caller; camera frame drawn by cameraTaskFn()
  }

  canvas.setTextSize(1);
  canvas.setTextColor(CP_ACCENT);
  canvas.setCursor(5, y); canvas.print("Initialising camera..."); y += 16;
  canvas.setTextColor(CP_DIM);
  canvas.setCursor(5, y); canvas.print("RST pulse + esp_camera_init"); y += 16;
  drawFooter();
  canvas.pushSprite(0, 0); // show "Initialising..." immediately before blocking init

  // Perform reset pulse while Wire is still available
  cameraResetPulse();

  if (cameraInit()) {
    cameraInitialized = true;
    clearContent();
    y = 32;
    canvas.setTextSize(1);
    canvas.setTextColor(CP_OK);
    canvas.setCursor(5, y); canvas.print("LIVE  240x176 RGB565");
    y += 14;
    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("HQVGA  20 MHz XCLK  DVP 8-bit");
  } else {
    clearContent();
    y = 32;
    canvas.setTextSize(1);
    canvas.setTextColor(CP_ERR);
    canvas.setCursor(5, y); canvas.print("esp_camera_init FAILED"); y += 16;
    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("Check flex cable seating"); y += 12;
    canvas.setCursor(5, y); canvas.print("and power-cycle the board.");
  }

  drawFooter();
  // final state pushed by caller (canvas.pushSprite in main loop)
}
