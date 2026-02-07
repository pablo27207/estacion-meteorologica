#ifndef TX_OTA_H
#define TX_OTA_H

#include <Arduino.h>

// Firmware version (increment when releasing new firmware)
#define FW_VERSION "1.0.0"

// OTA Server endpoints (uses same server as data upload)
// These are relative paths, base URL comes from server_client
#define OTA_VERSION_ENDPOINT "/api/firmware/version"
#define OTA_DOWNLOAD_ENDPOINT "/api/firmware/download"

// Check interval (1 hour in ms)
#define OTA_CHECK_INTERVAL 3600000

// Status
extern bool otaUpdateAvailable;
extern String otaNewVersion;
extern int otaProgress;
extern unsigned long otaLastCheck;
extern bool otaInProgress;

// Initialize OTA module
void otaInit();

// Check for available update (non-blocking)
// Returns true if new version available
bool otaCheckForUpdate();

// Perform the update (blocking, will reboot on success)
// Returns false only if update fails
bool otaPerformUpdate();

// Get time since last check as human-readable string
String otaGetLastCheckStr();

#endif
