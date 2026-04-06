#pragma once

#include <SD.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// GT30L24A3W Font Chip — 24Mb GB2312/GBK font ROM via HSPI
// CS: GPIO 40 (shared with SD via NPN inverter — HIGH = font selected)
// SCK: 44, MOSI: 42, MISO: 41
// ======================================================

void showFontChip() {
  drawHeader("Font Chip");
  clearContent();
  int y = 32;

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("GT30L24A3W Font ROM (SPI)");
  y += 18;

  printRow(y, CP_ACCENT, "Bus/CS:", CP_VALUE, "HSPI, GPIO40 NPN HIGH=sel");
  printRow(y, CP_ACCENT, "Pins:",   CP_VALUE, "SCK=44 MOSI=42 MISO=41");
  y += 6;

  // GPIO 40 is shared with SD_CS. NPN inverter makes CS mutually exclusive:
  //   HIGH = font selected + SD deselected
  //   LOW  = font deselected + SD selectable
  // Official library: GT30L24A3W.h #define FONTCS 40, SPI1.begin(44,41,42,-1)
  SD.end();
  spiSD.end();
  delay(2);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, LOW); // ensure deselected before init
  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, -1); // manual CS only
  spiSD.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // Read len bytes from 24-bit address; GPIO40 HIGH = font selected
  auto fontRead = [&](uint32_t addr, uint8_t* out, int len) {
    digitalWrite(FONT_CS, HIGH);
    spiSD.transfer(0x03);                      // READ command
    spiSD.transfer((addr >> 16) & 0xFF);
    spiSD.transfer((addr >>  8) & 0xFF);
    spiSD.transfer( addr        & 0xFF);
    for (int i = 0; i < len; i++) out[i] = spiSD.transfer(0x00);
    digitalWrite(FONT_CS, LOW);
  };

  // Read ASCII 'A'(0x41) and 'B'(0x42) using 8x16 font
  // Formula from ASCII_GetData: address = 16 * (asc + 130174), read 16 bytes
  uint8_t glyphA[16], glyphB[16];
  fontRead(16UL * (0x41 + 130174), glyphA, 16);
  fontRead(16UL * (0x42 + 130174), glyphB, 16);

  // Verify by checking two known-different addresses differ
  bool allSame = true;
  for (int i = 0; i < 16; i++) if (glyphA[i] != glyphB[i]) { allSame = false; break; }
  bool allFF = true, all00 = true;
  for (int i = 0; i < 16; i++) {
    if (glyphA[i] != 0xFF) allFF = false;
    if (glyphA[i] != 0x00) all00 = false;
  }
  bool pass = !allFF && !all00 && !allSame;

  // Keep transaction open — more reads follow in the pass block below
  printRow(y, CP_ACCENT, "Status:",
           pass ? CP_OK : CP_ERR,
           allFF ? "NO RESPONSE (0xFF)" : all00 ? "MISO stuck (0x00)" :
           allSame ? "A==B (stuck?)" : "DATA OK");
  y += 10;

  if (pass) {
    // --- Helper: draw a 1-bit-per-pixel glyph at (gx,gy), bytesPerRow wide ---
    auto drawGlyph = [&](int gx, int gy, uint8_t* bmp,
                         int w, int h, int bpr, int scale, uint16_t color) {
      for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
          if ((bmp[row * bpr + col / 8] >> (7 - (col % 8))) & 1)
            canvas.fillRect(gx + col * scale, gy + row * scale, scale, scale, color);
        }
      }
    };

    // ---- Row 1: Letter 'A' in all 5 font sizes side by side ----
    canvas.setTextColor(CP_ACCENT);
    canvas.setCursor(5, y); canvas.print("'A' all sizes (5x7  6x12  8x16  12x24  16x32)");
    y += 11;

    uint8_t g5x7[8], g6x12[12], g8x16[16], g12x24[48], g16x32[64];

    // Formulas from official DFRobot_GT30L24A3W.cpp ASCII_GetData()
    fontRead(8UL  * (0x41 + 259932),           g5x7,  8);   // ASCII_5X7
    fontRead(12UL *  0x41 + 2080864UL,         g6x12, 12);  // ASCII_6X12
    fontRead(16UL * (0x41 + 130174),           g8x16, 16);  // ASCII_8X16
    fontRead(48UL *  0x41 + 1543800UL,         g12x24,48);  // ASCII_12X24_A
    fontRead(((0x41UL + 67108832UL) << 6) + 2084832UL, g16x32, 64); // ASCII_16X32

    int rowBase = y;
    drawGlyph( 5,  rowBase, g5x7,   5,  7, 1, 2, CP_VALUE);   // 10x14
    drawGlyph(25,  rowBase, g6x12,  6, 12, 1, 2, CP_VALUE);   // 12x24
    drawGlyph(50,  rowBase, g8x16,  8, 16, 1, 2, CP_VALUE);   // 16x32
    drawGlyph(80,  rowBase, g12x24,12, 24, 2, 2, CP_VALUE);   // 24x48 — scale 2x
    drawGlyph(135, rowBase, g16x32,16, 32, 2, 2, CP_VALUE);   // 32x64 — scale 2x
    y += 70; // tallest glyph at 2x = 64px, +6 gap

    // Size labels
    canvas.setTextColor(CP_DIM);
    canvas.setCursor( 5, y); canvas.print("5x7");
    canvas.setCursor(25, y); canvas.print("6x12");
    canvas.setCursor(50, y); canvas.print("8x16");
    canvas.setCursor(80, y); canvas.print("12x24");
    canvas.setCursor(135,y); canvas.print("16x32");
    y += 12;

    // ---- Row 2: A–L in 8x16 at 2x ----
    canvas.setTextColor(CP_ACCENT);
    canvas.setCursor(5, y); canvas.print("A-L  8x16:");
    y += 11;
    uint8_t g[16];
    for (int ci = 0; ci < 12; ci++) {
      fontRead(16UL * ('A' + ci + 130174), g, 16);
      drawGlyph(5 + ci * 19, y, g, 8, 16, 1, 2, CP_OK);
    }
    y += 36;

    // ---- Row 3: 0–9 in 6x12 at 2x ----
    canvas.setTextColor(CP_ACCENT);
    canvas.setCursor(5, y); canvas.print("0-9  6x12:");
    y += 11;
    uint8_t g6[12];
    for (int ci = 0; ci < 10; ci++) {
      fontRead(12UL * ('0' + ci) + 2080864UL, g6, 12);
      drawGlyph(5 + ci * 22, y, g6, 6, 12, 1, 2, CP_BIG);
    }
    y += 30;

    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("24Mb GB2312/GBK. CS GPIO40 shared w/SD.");

  } else {
    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("Check SD card removed or SD.end() called."); y += 12;
    canvas.setCursor(5, y); canvas.print("GPIO40: font=HIGH, SD=LOW (NPN shared).");
  }

  spiSD.endTransaction();
  drawFooter();
}
