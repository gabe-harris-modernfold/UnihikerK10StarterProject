#pragma once

// Library includes needed for complex extern types
#include <SPI.h>
#include "lgfx_config.h"
#include <Adafruit_NeoPixel.h>
#include "driver/i2s.h"   // for esp_err_t

// ======================================================
// OBJECTS (defined in K10ComponentTest.ino)
// ======================================================
extern SPIClass  spiSD;
extern LGFX      tft;
extern Adafruit_NeoPixel leds;

// ======================================================
// NAVIGATION STATE
// ======================================================
extern int           currentTest;
extern const int     NUM_TESTS;
extern bool          prevBtnA;
extern bool          prevBtnB;
extern bool          needsRedraw;
extern unsigned long lastRefresh;

// ======================================================
// LED COMPONENT STATE
// ======================================================
extern int           ledCycleStep;
extern unsigned long lastLedStep;

// ======================================================
// SPEAKER COMPONENT STATE
// ======================================================
extern bool          i2sInstalled;
extern int           spkrVolStep;
extern float         spkrPhase;
extern unsigned long lastVolStep;

// ======================================================
// MIC COMPONENT STATE
// ======================================================
extern float     micDbL;
extern float     micDbR;
extern float     micPeakDbL;
extern float     micPeakDbR;
extern size_t    micLastBytes;
extern int32_t   micRawMax;
extern esp_err_t micInstallErr;
extern esp_err_t micPinErr;
extern esp_err_t micReadErr;
extern int16_t   micSamp[4];
extern unsigned long lastMicRead;

// ======================================================
// CAMERA COMPONENT STATE
// ======================================================
extern bool              cameraInitialized;
extern TaskHandle_t      cameraTaskHandle;
extern SemaphoreHandle_t displayMutex;
