#ifndef TX_SERVER_CLIENT_H
#define TX_SERVER_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../shared/config.h"

// Server configuration
extern String serverUrl;
extern String apiKey;
extern bool serverEnabled;

// Last send status
extern bool lastServerOk;
extern unsigned long lastServerSendTime;
extern int serverPendingCount;

/**
 * Initialize server client
 * Loads configuration from NVS
 */
void serverClientInit();

/**
 * Send data to server
 * @param data Sensor data packet
 * @return true if successful
 */
bool sendToServer(const MeteorDataPacket& data);

/**
 * Save server settings to NVS
 */
void saveServerSettings(const String& url, const String& key, bool enabled);

/**
 * Load server settings from NVS
 */
void loadServerSettings();

/**
 * Get number of packets pending in retry buffer
 */
int getPendingCount();

#endif
