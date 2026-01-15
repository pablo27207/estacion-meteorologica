#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../shared/config.h"

// Server configuration
extern String serverUrl;
extern String apiKey;
extern bool serverEnabled;

// Pending config from server
struct ServerConfig {
    bool hasPending;
    uint8_t sf;
    float bw;
    uint32_t interval;
};

extern ServerConfig serverConfig;

/**
 * Initialize server client
 * Loads configuration from NVS
 */
void serverClientInit();

/**
 * Send data to server
 * @param data Sensor data packet
 * @param rssi Signal strength
 * @param snr Signal to noise ratio
 * @param freqError Frequency error
 * @return true if successful
 */
bool sendToServer(const MeteorDataPacket& data, float rssi, float snr, float freqError);

/**
 * Check for pending configuration from server
 * @return true if new config available
 */
bool checkServerConfig();

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
 * @return number of buffered packets
 */
int getPendingCount();

#endif
