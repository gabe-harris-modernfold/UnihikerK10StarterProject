#pragma once

// ======================================================
// PIN & DEVICE MAP — Unihiker K10 (ESP32-S3 WROOM 1)
// ======================================================

// I2C Bus
#define I2C_SDA       47
#define I2C_SCL       48

// I2C Device Addresses
#define XL9535_ADDR   0x20
#define AHT20_ADDR    0x38
#define LTR303_ADDR   0x29
#define SC7A20H_ADDR  0x19
#define GC2145_ADDR   0x3C

// TFT Display (ILI9341 on FSPI)
#define TFT_CS        14
#define TFT_DC        13
#define TFT_MOSI      21
#define TFT_MISO      -1
#define TFT_SCLK      12

// SD Card (HSPI)
#define SD_CS         40
#define SD_MOSI       42
#define SD_MISO       41
#define SD_SCK        44

// WS2812 RGB LEDs
#define LED_PIN       46
#define LED_COUNT     3

// Camera GC2145 (DVP, D2-D9 = GPIO 8,10,11,9,18,16,15,6)
#define CAM_XCLK      7
#define CAM_PCLK      17
#define CAM_VSYNC     4
#define CAM_HREF      5
#define CAM_D0        8    // D2 (LSB)
#define CAM_D1        10   // D3
#define CAM_D2        11   // D4
#define CAM_D3        9    // D5
#define CAM_D4        18   // D6
#define CAM_D5        16   // D7
#define CAM_D6        15   // D8
#define CAM_D7        6    // D9 (MSB)

// GT30L24A3W Font chip — shares HSPI bus AND CS pin with SD card (GPIO 40)
// NPN inverter on CS line makes selection mutually exclusive:
//   GPIO 40 HIGH → NPN on → font CS# LOW → font selected, SD deselected
//   GPIO 40 LOW  → NPN off → font CS# HIGH → font deselected, SD selected
// Source: GT30L24A3W.h in official library: #define FONTCS 40
#define FONT_CS       40   // same GPIO as SD_CS — shared pin, opposite polarity

// I2S — pin names match official unihiker_k10.h (IIS_* defines)
// NOTE: GPIO 38 (IIS_LRCK) is shared with FONT_CS — not simultaneous
#define IIS_BCLK      0    // Bit Clock
#define IIS_LRCK      38   // Word Select — NOTE: also FONT_CS (hardware shared, not simultaneous)
#define IIS_DOUT      45   // Speaker out → NS4168 amp
#define IIS_DSIN      39   // Mic in ← MEMS mics (ES7243E SDOUT)
#define IIS_MCLK      3    // Master Clock

#define I2S_PORT      I2S_NUM_0
#define SAMPLE_RATE   44100
#define TONE_HZ       440  // A4

// Microphone signal processing
#define MIC_FLOOR_DB   -50.0f   // floor: below this = bar empty
#define MIC_SW_GAIN     8.0f    // software preamp to compensate ES7243E default gain
