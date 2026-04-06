#pragma once

#include <SD.h>
#include "config.h"
#include "globals.h"
#include "display.h"

// ======================================================
// SD Card — Micro SD via HSPI
// CS: GPIO 40, MOSI: 42, MISO: 41, SCK: 44
// Note: GPIO 40 is shared with FONT_CS (NPN inverter)
// ======================================================

void showSD() {
  drawHeader("SD Card");
  clearContent();
  int y = 32;
  char buf[32];

  canvas.setTextColor(CP_ACCENT);
  canvas.setTextSize(1);
  canvas.setCursor(5, y);
  canvas.print("Micro SD Card (SPI)");
  y += 18;

  printRow(y, CP_ACCENT, "Interface:", CP_VALUE, "HSPI");
  printRow(y, CP_ACCENT, "CS / MOSI:", CP_VALUE, "GPIO 40 / 42");
  printRow(y, CP_ACCENT, "MISO / SCK:",CP_VALUE, "GPIO 41 / 44");
  y += 8;

  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  bool mounted = SD.begin(SD_CS, spiSD);
  if (!mounted) {
    printRow(y, CP_ACCENT, "Status:", CP_ERR, "MOUNT FAILED");
    y += 8;
    canvas.setTextColor(CP_DIM);
    canvas.setCursor(5, y); canvas.print("Check SD card is inserted");
    drawFooter(); return;
  }

  printRow(y, CP_ACCENT, "Status:", CP_OK, "MOUNTED OK");

  uint8_t cardType = SD.cardType();
  const char* typeStr;
  switch (cardType) {
    case CARD_MMC:  typeStr = "MMC";     break;
    case CARD_SD:   typeStr = "SD";      break;
    case CARD_SDHC: typeStr = "SDHC";    break;
    default:        typeStr = "UNKNOWN"; break;
  }
  printRow(y, CP_ACCENT, "Type:", CP_VALUE, typeStr);

  uint64_t sizeMB = SD.cardSize() / (1024ULL * 1024ULL);
  snprintf(buf, sizeof(buf), "%llu MB", sizeMB);
  printRow(y, CP_ACCENT, "Size:", CP_VALUE, buf);
  y += 8;

  File root = SD.open("/");
  canvas.setTextColor(CP_ACCENT);
  canvas.setCursor(5, y); canvas.print("Root:"); y += 13;
  canvas.setTextColor(CP_VALUE);

  int count = 0;
  while (count < 9) {
    File entry = root.openNextFile();
    if (!entry) break;
    canvas.setCursor(10, y);
    canvas.print(entry.isDirectory() ? "[D] " : "    ");
    canvas.print(entry.name());
    y += 12;
    count++;
    entry.close();
  }
  if (count == 0) { canvas.setCursor(10, y); canvas.print("(empty)"); }
  root.close();
  SD.end();

  drawFooter();
}
