/*
  UNIHIKER K10 (ESP32-S3 WROOM 1) — Starter Project
  BTN_A (XL9535 P1.4) = Next component
  BTN_B (XL9535 P0.2) = Previous component
  Live sensor readings refresh every 1s.
  Mic VU meter refreshes every 200ms.

  GPIO 38 NOTE: shared by IIS_LRCK (speaker/mic) and FONT_CS (font chip).
  Cannot be used simultaneously — sequential screens, no conflict.
  GPIO 45 = speaker DOUT. GPIO 46 = WS2812 LEDs only (no I2S conflict).

  References
  https://www.unihiker.com/wiki/K10/HardwareReference/img/hardwarereference_onboard/UnihikerK10Schematic.pdf
  https://www.unihiker.com/wiki/K10/HardwareReference/hardwarereference_specs/#system-framwork
  https://www.makerbrains.com/projects/learn-edge-ai-on-unihiker-k10
  https://github.com/MukeshSankhla/Learn-Edge-AI-on-Unihiker-K10-Edge-Impulse-Beginner-Tutorial-
  https://hackaday.io/project/203864-light-meter-unihiker-k10-with-arduino-libraries/details
  Thank you, Mukesh Sankhla!
*/

// ---- System / library includes ----
#include <Wire.h>
#include <SPI.h>
#include "lgfx_config.h"
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include "driver/i2s.h"
#include "driver/i2c.h"
#include "esp_camera.h"
#include "initBoard.h"   // init_board() configures XL9535 + ES7243E codec
#include <math.h>

// ---- Project headers (config and globals first, then drivers) ----
#include "config.h"
#include "globals.h"
#include "xl9535.h"
#include "display.h"
#include "i2s_audio.h"
#include "aht20.h"
#include "ltr303.h"
#include "sc7a20h.h"
#include "sd_card.h"
#include "leds.h"
#include "camera.h"
#include "speaker.h"
#include "mic.h"
#include "fontchip.h"
#include "xl9535_test.h"
#include "backlight.h"
#include "bootscreen.h"

// ======================================================
// OBJECTS
// ======================================================
SPIClass   spiSD(HSPI);
LGFX       tft;
LGFX_Sprite canvas(&tft);
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ======================================================
// STATE
// ======================================================
int  currentTest    = 0;
const int NUM_TESTS = 11;
bool prevBtnA       = false;
bool prevBtnB       = false;
bool needsRedraw    = true;
unsigned long lastRefresh = 0;

// LED component state
int  ledCycleStep   = 0;
unsigned long lastLedStep = 0;

// Speaker component state
bool  i2sInstalled  = false;
float spkrPhase     = 0.0f;
float spkrCurrentAmp = 0.0f;  // linear ramp fraction 0..1; PCM amp = frac² × 32767

// Mic component state — stored as dBFS (0 = full scale, floor = silence)
float micDbL        = MIC_FLOOR_DB;
float micDbR        = MIC_FLOOR_DB;
float micPeakDbL    = MIC_FLOOR_DB;
float micPeakDbR    = MIC_FLOOR_DB;
size_t  micLastBytes   = 0;
int32_t micRawMax      = 0;
esp_err_t micInstallErr = ESP_OK;  // i2s_driver_install return
esp_err_t micPinErr     = ESP_OK;  // i2s_set_pin return
esp_err_t micReadErr    = ESP_OK;  // i2s_read return
int16_t micSamp[4]     = {0,0,0,0}; // first 4 raw samples
unsigned long lastMicRead = 0;

// Camera component state
bool              cameraInitialized = false;
TaskHandle_t      cameraTaskHandle  = NULL;
SemaphoreHandle_t displayMutex      = NULL;

// Backlight demo state
bool          backlightOn       = true;
int           backlightDemoStep = 0;
unsigned long lastBacklightStep = 0;

// ======================================================
// COMPONENT DISPATCHER
// ======================================================
void renderCurrentTest() {
  switch (currentTest) {
    case 0: showAHT20();     break;
    case 1: showLTR303();    break;
    case 2: showSC7A20H();   break;
    case 3: showSD();        break;
    case 4: showLEDs();      break;
    case 5: showCamera();    break;
    case 6: showSpeaker();   break;
    case 7: showMic();       break;
    case 8: showFontChip();  break;
    case 9: showXL9535();    break;
    case 10: showBacklight(); break;
  }
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);

  // init_board() configures XL9535 (backlight, camera RST, button pins) and
  // initializes the ES7243E audio codec via I2C so the mic I2S stream works.
  // Must run before Wire.begin() as it uses ESP-IDF I2C directly.
  Wire.begin(I2C_SDA, I2C_SCL); // init_board needs I2C already up on same bus
  init_board();

  // Re-apply our own XL9535 settings on top of init_board's defaults
  enableDisplayBacklight();
  configureButtons();
  configureSuccessLed();
  setSuccessLed(false);  // XL9535 output register defaults HIGH at power-on — force off

  tft.init();
  tft.fillScreen(0x0000);

  canvas.setPsram(true);
  canvas.setColorDepth(16);
  canvas.createSprite(240, 320);
  canvas.fillScreen(0x0000);

  leds.begin();
  leds.clear();
  leds.show();

  // FreeRTOS display mutex — must exist before cameraTaskFn starts
  displayMutex = xSemaphoreCreateMutex();
  configASSERT(displayMutex != NULL);

  // Camera task pinned to Core 1, priority 2 (above loopTask's priority 1).
  // Stack 4096 bytes — cameraDrawFrame() locals are trivial; frame data is in PSRAM.
  xTaskCreatePinnedToCore(
    cameraTaskFn,      // task function (defined in camera.h)
    "cameraTask",      // name for vTaskList debugging
    4096,              // stack bytes (ESP32 Arduino uses bytes, not words)
    NULL,              // pvParameters — task reads globals directly
    2,                 // priority
    &cameraTaskHandle, // handle stored for future suspend/delete
    1                  // Core 1
  );
  configASSERT(cameraTaskHandle != NULL);

  // Keep font chip deselected at boot (GPIO 40 LOW = NPN off = CS# HIGH = font idle)
  // Same pin as SD_CS — LOW also means SD is selectable normally
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, LOW);

  // Silence NS4168 amp at boot — floating I2S pins cause faint noise until
  // speaker screen is visited. Brief install+uninstall drives pins to a
  // defined state (i2s_zero_dma_buffer runs before pins go live).
  i2sInstallSpeaker();
  i2sUninstall();

  // Boot splash — fades "UNIHIKER K10" in over 5 s, then waits for a button press
  runBootScreen();

  needsRedraw = true;
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  unsigned long now = millis();

  // --- Button edge detection ---
  bool btnA = readBtnA();
  bool btnB = readBtnB();

  if (btnA && !prevBtnA) {
    leds.clear(); leds.show(); setSuccessLed(false);
    if (i2sInstalled) i2sUninstall();
    if (cameraInitialized) {
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      cameraStop();
      xSemaphoreGive(displayMutex);
    }
    if (currentTest == 10) { setBacklight(true); backlightOn = true; }
    currentTest = (currentTest + 1) % NUM_TESTS;
    if (currentTest == 4)  { ledCycleStep = 0; lastLedStep = now; }
    if (currentTest == 6)  { spkrPhase = 0.0f; spkrCurrentAmp = 0.0f; }
    if (currentTest == 7)  { micPeakDbL = MIC_FLOOR_DB; micPeakDbR = MIC_FLOOR_DB; }
    if (currentTest == 10) { backlightDemoStep = 0; lastBacklightStep = now; backlightOn = true; }
    needsRedraw = true;
    lastRefresh = now;
  }
  if (btnB && !prevBtnB) {
    leds.clear(); leds.show(); setSuccessLed(false);
    if (i2sInstalled) i2sUninstall();
    if (cameraInitialized) {
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      cameraStop();
      xSemaphoreGive(displayMutex);
    }
    if (currentTest == 10) { setBacklight(true); backlightOn = true; }
    currentTest = (currentTest - 1 + NUM_TESTS) % NUM_TESTS;
    if (currentTest == 4)  { ledCycleStep = 0; lastLedStep = now; }
    if (currentTest == 6)  { spkrPhase = 0.0f; spkrCurrentAmp = 0.0f; }
    if (currentTest == 7)  { micPeakDbL = MIC_FLOOR_DB; micPeakDbR = MIC_FLOOR_DB; }
    if (currentTest == 10) { backlightDemoStep = 0; lastBacklightStep = now; backlightOn = true; }
    needsRedraw = true;
    lastRefresh = now;
  }
  prevBtnA = btnA;
  prevBtnB = btnB;

  // --- Install / uninstall I2S as needed ---
  if (currentTest == 6 && !i2sInstalled) i2sInstallSpeaker();
  if (currentTest == 7 && !i2sInstalled) i2sInstallMic();

  // --- LED cycle (1s) ---
  if (currentTest == 4 && now - lastLedStep >= 1000) {
    ledCycleStep = (ledCycleStep + 1) % 5;
    lastLedStep  = now;
    needsRedraw  = true;
  }

  // --- Speaker: fill I2S buffer every loop ---
  if (currentTest == 6 && i2sInstalled) {
    fillSpeakerBuffer();
  }

  // --- Mic: read buffer every 200ms ---
  if (currentTest == 7 && i2sInstalled && now - lastMicRead >= 20) {
    readMicBuffer();
    lastMicRead = now;
  }

  // --- Backlight demo cycle: ON 3s → OFF 2s → repeat ---
  if (currentTest == 10) {
    unsigned long stepDuration = (backlightDemoStep == 0) ? 3000UL : 2000UL;
    if (now - lastBacklightStep >= stepDuration) {
      backlightDemoStep = 1 - backlightDemoStep;
      backlightOn = (backlightDemoStep == 0);
      setBacklight(backlightOn);
      lastBacklightStep = now;
      needsRedraw = true;
    }
  }

  // --- Screen refresh ---
  bool liveTest  = (currentTest <= 2 || currentTest == 9 || currentTest == 10); // sensors + XL9535 + backlight: 1s
  bool spkrActive= (currentTest == 6);                     // speaker: 1s (vol label)

  if (needsRedraw ||
      (liveTest   && now - lastRefresh >= 1000) ||
      (spkrActive && now - lastRefresh >= 1000)) {
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    renderCurrentTest();
    canvas.pushSprite(0, 0); // camera screen: pushes header; DMA task overwrites content area
    needsRedraw = false;
    lastRefresh = now;
    xSemaphoreGive(displayMutex);
  }
}
