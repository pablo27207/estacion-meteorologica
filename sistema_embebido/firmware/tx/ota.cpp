/**
 * OTA Update Module for TX Node
 * Handles HTTP-based firmware updates from server
 */

#include "ota.h"
#include "server_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// Status variables
bool otaUpdateAvailable = false;
String otaNewVersion = "";
int otaProgress = 0;
unsigned long otaLastCheck = 0;
bool otaInProgress = false;

// Internal
static WiFiClientSecure otaClient;

void otaInit() {
    otaClient.setInsecure();  // Skip certificate validation
    Serial.printf("[OTA] Initialized. Current version: %s\n", FW_VERSION);
}

// Compare version strings (returns true if newVer > currentVer)
bool isNewerVersion(const char* current, const char* newVer) {
    int currMajor, currMinor, currPatch;
    int newMajor, newMinor, newPatch;
    
    sscanf(current, "%d.%d.%d", &currMajor, &currMinor, &currPatch);
    sscanf(newVer, "%d.%d.%d", &newMajor, &newMinor, &newPatch);
    
    if (newMajor > currMajor) return true;
    if (newMajor < currMajor) return false;
    if (newMinor > currMinor) return true;
    if (newMinor < currMinor) return false;
    return newPatch > currPatch;
}

bool otaCheckForUpdate() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi not connected");
        return false;
    }
    
    otaLastCheck = millis();
    
    // Build URL
    String url = serverUrl + OTA_VERSION_ENDPOINT;
    
    HTTPClient http;
    http.begin(otaClient, url);
    http.addHeader("X-API-Key", apiKey);
    http.setTimeout(10000);
    
    Serial.printf("[OTA] Checking: %s\n", url.c_str());
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String response = http.getString();
        http.end();
        
        // Parse JSON
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            Serial.printf("[OTA] JSON parse error: %s\n", error.c_str());
            return false;
        }
        
        const char* serverVersion = doc["version"];
        
        if (isNewerVersion(FW_VERSION, serverVersion)) {
            otaUpdateAvailable = true;
            otaNewVersion = String(serverVersion);
            Serial.printf("[OTA] New version available: %s (current: %s)\n", serverVersion, FW_VERSION);
            return true;
        } else {
            otaUpdateAvailable = false;
            otaNewVersion = "";
            Serial.printf("[OTA] Already up to date (%s)\n", FW_VERSION);
            return false;
        }
    } else {
        Serial.printf("[OTA] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
}

bool otaPerformUpdate() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi not connected");
        return false;
    }
    
    if (!otaUpdateAvailable) {
        Serial.println("[OTA] No update available");
        return false;
    }
    
    otaInProgress = true;
    otaProgress = 0;
    
    String url = serverUrl + OTA_DOWNLOAD_ENDPOINT;
    
    HTTPClient http;
    http.begin(otaClient, url);
    http.addHeader("X-API-Key", apiKey);
    http.setTimeout(60000);  // 60 second timeout for download
    
    Serial.printf("[OTA] Downloading from: %s\n", url.c_str());
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        Serial.printf("[OTA] Download failed, HTTP: %d\n", httpCode);
        http.end();
        otaInProgress = false;
        return false;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("[OTA] Invalid content length");
        http.end();
        otaInProgress = false;
        return false;
    }
    
    Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);
    
    // Start update
    if (!Update.begin(contentLength)) {
        Serial.printf("[OTA] Not enough space: %s\n", Update.errorString());
        http.end();
        otaInProgress = false;
        return false;
    }
    
    // Get stream and write to flash
    WiFiClient* stream = http.getStreamPtr();
    
    uint8_t buff[1024];
    int written = 0;
    
    while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available) {
            int readBytes = stream->readBytes(buff, min(available, sizeof(buff)));
            Update.write(buff, readBytes);
            written += readBytes;
            
            // Update progress
            otaProgress = (written * 100) / contentLength;
            
            // Print progress every 10%
            static int lastPrint = -10;
            if (otaProgress >= lastPrint + 10) {
                Serial.printf("[OTA] Progress: %d%%\n", otaProgress);
                lastPrint = otaProgress;
            }
        }
        delay(1);
    }
    
    http.end();
    
    if (written != contentLength) {
        Serial.printf("[OTA] Download incomplete: %d/%d\n", written, contentLength);
        Update.abort();
        otaInProgress = false;
        return false;
    }
    
    if (!Update.end(true)) {
        Serial.printf("[OTA] Update failed: %s\n", Update.errorString());
        otaInProgress = false;
        return false;
    }
    
    Serial.println("[OTA] Update successful! Rebooting...");
    delay(1000);
    ESP.restart();
    
    return true;  // Never reached
}

String otaGetLastCheckStr() {
    if (otaLastCheck == 0) {
        return "Nunca";
    }
    
    unsigned long elapsed = (millis() - otaLastCheck) / 1000;  // seconds
    
    if (elapsed < 60) {
        return "Hace " + String(elapsed) + "s";
    } else if (elapsed < 3600) {
        return "Hace " + String(elapsed / 60) + "m";
    } else {
        return "Hace " + String(elapsed / 3600) + "h";
    }
}
