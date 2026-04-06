#pragma once

#include <LovyanGFX.hpp>
#include "config.h"

// ======================================================
// LovyanGFX hardware configuration for ILI9341 on FSPI
// ESP32-S3: FSPI = SPI2_HOST, DMA enabled
// ======================================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;

public:
  LGFX() {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;        // FSPI on ESP32-S3
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;         // 40 MHz (ILI9341 max ~42 MHz)
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;  // let IDF pick the DMA channel
      cfg.pin_sclk    = TFT_SCLK;        // GPIO 12
      cfg.pin_mosi    = TFT_MOSI;        // GPIO 21
      cfg.pin_miso    = TFT_MISO;        // -1 (no MISO)
      cfg.pin_dc      = TFT_DC;          // GPIO 13
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs          = TFT_CS;      // GPIO 14
      cfg.pin_rst         = -1;          // reset handled via XL9535 before init
      cfg.pin_busy        = -1;
      cfg.panel_width     = 240;
      cfg.panel_height    = 320;
      cfg.offset_x        = 0;
      cfg.offset_y        = 0;
      cfg.offset_rotation = 0;           // portrait, no rotation offset
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable        = false;       // no MISO
      cfg.invert          = false;
      cfg.rgb_order       = false;
      cfg.dlen_16bit      = false;
      cfg.bus_shared      = false;       // FSPI dedicated to display only
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};
