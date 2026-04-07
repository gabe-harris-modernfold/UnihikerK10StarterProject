#pragma once

#include "driver/i2s.h"
#include <math.h>
#include "config.h"
#include "globals.h"

// ======================================================
// I2S AUDIO HELPERS — Speaker (TX) and Microphone (RX)
// Shared by speaker.h and mic.h
// ======================================================

// Speaker fade-in parameters
#define SPKR_CHUNK      256       // samples per fillSpeakerBuffer() call (~5.8 ms at 44.1 kHz)
#define SPKR_RAMP_S     30.0f    // fade-in duration in seconds
#define SPKR_DB_START  -60.0f    // initial level (dBFS)
#define SPKR_DB_RANGE   20.0f    // ramp spans this many dB (ends at -40 dBFS)

void i2sInstallSpeaker() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 256,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0,
    .mclk_multiple        = I2S_MCLK_MULTIPLE_DEFAULT,
    .bits_per_chan         = I2S_BITS_PER_CHAN_DEFAULT
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_zero_dma_buffer(I2S_PORT);  // zero DMA buffers BEFORE pins go live — prevents startup screech

  i2s_pin_config_t pins = {
    .mck_io_num   = IIS_MCLK,            // GPIO 3
    .bck_io_num   = IIS_BCLK,            // GPIO 0
    .ws_io_num    = IIS_LRCK,            // GPIO 38 (also FONT_CS — not simultaneous)
    .data_out_num = IIS_DOUT,            // GPIO 45
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_PORT, &pins);
  i2sInstalled = true;
}

void i2sInstallMic() {
  // Exact match to official unihiker_k10 initI2S():
  // full-duplex 16-bit stereo, 16 kHz. ES7243E codec (init'd by init_board())
  // outputs on DSIN (GPIO39). TX side outputs silence to keep clocks active.
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 3,
    .dma_buf_len          = 300,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0,
    .mclk_multiple        = I2S_MCLK_MULTIPLE_DEFAULT,
    .bits_per_chan         = I2S_BITS_PER_CHAN_DEFAULT
  };
  micInstallErr = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

  i2s_pin_config_t pins = {
    .mck_io_num   = IIS_MCLK,   // GPIO 3
    .bck_io_num   = IIS_BCLK,   // GPIO 0
    .ws_io_num    = IIS_LRCK,   // GPIO 38
    .data_out_num = IIS_DOUT,   // GPIO 45
    .data_in_num  = IIS_DSIN    // GPIO 39 (from ES7243E SDOUT)
  };
  micPinErr = i2s_set_pin(I2S_PORT, &pins);
  i2sInstalled = true;
}

void i2sUninstall() {
  i2s_driver_uninstall(I2S_PORT);
  i2sInstalled = false;
}

// Called every loop() when on the speaker screen — fills I2S TX buffer
void fillSpeakerBuffer() {
  // Advance ramp fraction 0..1 once per chunk (SPKR_CHUNK samples ≈ 5.8 ms — inaudible granularity).
  const float fracStep = (float)SPKR_CHUNK / (SAMPLE_RATE * SPKR_RAMP_S);
  if (spkrCurrentAmp < 1.0f) {
    spkrCurrentAmp += fracStep;
    if (spkrCurrentAmp > 1.0f) spkrCurrentAmp = 1.0f;
  }

  // dB-linear scaling: fraction 0→1 maps SPKR_DB_START → (SPKR_DB_START + SPKR_DB_RANGE) dBFS.
  // The NS4168 amp saturates at roughly -40dBFS (~328 PCM) on this hardware,
  // so this stretches the entire audible fade across the full ramp duration.
  // powf is called once per chunk, not per sample — negligible cost.
  float dbFS     = SPKR_DB_START + spkrCurrentAmp * SPKR_DB_RANGE;
  float chunkAmp = 32767.0f * powf(10.0f, dbFS / 20.0f);

  int16_t buf[SPKR_CHUNK * 2]; // stereo interleaved
  const float phaseInc = 2.0f * (float)M_PI * TONE_HZ / SAMPLE_RATE;

  for (int i = 0; i < SPKR_CHUNK; i++) {
    int16_t s = (int16_t)(chunkAmp * sinf(spkrPhase));
    spkrPhase += phaseInc;
    if (spkrPhase >= 2.0f * (float)M_PI) spkrPhase -= 2.0f * (float)M_PI;
    buf[i * 2]     = s;
    buf[i * 2 + 1] = s;
  }

  size_t bytesWritten;
  i2s_write(I2S_PORT, buf, sizeof(buf), &bytesWritten, pdMS_TO_TICKS(10));
}

// Called every ~20ms when on the mic screen — reads and computes RMS dBFS
void readMicBuffer() {
  // 16-bit stereo: each frame = 2 bytes L + 2 bytes R = 4 bytes
  const int FRAMES = 300;
  int16_t buf[FRAMES * 2];
  size_t bytesRead = 0;
  micReadErr = i2s_read(I2S_PORT, buf, sizeof(buf), &bytesRead, pdMS_TO_TICKS(10));
  micLastBytes = bytesRead;

  int n = bytesRead / 4; // stereo frames
  if (n == 0) { needsRedraw = true; return; }

  // Capture first 4 samples for display
  for (int i = 0; i < 4 && i < n * 2; i++) micSamp[i] = buf[i];

  int32_t rawMax = 0;
  int64_t sumSqL = 0, sumSqR = 0;
  for (int i = 0; i < n; i++) {
    int16_t sL = buf[i * 2];
    int16_t sR = buf[i * 2 + 1];
    int32_t aL = sL < 0 ? -sL : sL;
    int32_t aR = sR < 0 ? -sR : sR;
    if (aL > rawMax) rawMax = aL;
    if (aR > rawMax) rawMax = aR;
    sumSqL += (int64_t)sL * sL;
    sumSqR += (int64_t)sR * sR;
  }
  micRawMax = rawMax;

  // Normalise to 0.0–1.0, apply software gain, clamp at 1.0
  float rmsL = fminf(sqrtf((float)sumSqL / n) / 32768.0f * MIC_SW_GAIN, 1.0f);
  float rmsR = fminf(sqrtf((float)sumSqR / n) / 32768.0f * MIC_SW_GAIN, 1.0f);

  micDbL = (rmsL > 0.0f) ? fmaxf(20.0f * log10f(rmsL), MIC_FLOOR_DB) : MIC_FLOOR_DB;
  micDbR = (rmsR > 0.0f) ? fmaxf(20.0f * log10f(rmsR), MIC_FLOOR_DB) : MIC_FLOOR_DB;
  if (micDbL > micPeakDbL) micPeakDbL = micDbL;
  if (micDbR > micPeakDbR) micPeakDbR = micDbR;
  needsRedraw = true;
}
