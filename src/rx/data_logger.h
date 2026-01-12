#ifndef RX_DATA_LOGGER_H
#define RX_DATA_LOGGER_H

#include <Arduino.h>
#include "../shared/config.h"

// File size and stats
extern unsigned long logFileSize;
extern unsigned long logEntryCount;

// Initialize LittleFS and data file
bool dataLoggerInit();

// Append data entry to log file
void logReceivedData(const MeteorDataPacket& data, float rssi, float snr);

// Get current log file size in bytes
unsigned long getLogFileSize();

// Clear the log file
void clearLogFile();

#endif
