#ifndef TX_SD_LOGGER_H
#define TX_SD_LOGGER_H

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "../shared/config.h"

// --- SD Card Pins ---
#define SD_SCK  48
#define SD_MISO 33
#define SD_MOSI 47
#define SD_CS   26

// State (read-only access)
extern bool sdConnected;
extern uint64_t sdTotalSpace;
extern unsigned long sdWriteCount;
extern String sdStatusMsg;

// Initialize SD Card
void sdInit();

// Log data to CSV
void logToSD(const MeteorDataPacket& data);

#endif
