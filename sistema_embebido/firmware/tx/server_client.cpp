/**
 * Server Client for TX Node
 * Sends sensor data directly to server via HTTPS
 */

#include "server_client.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "rtc.h"

// Configuration - defaults (same as RX vivero-olivos station)
String serverUrl = "https://gipis.unp.edu.ar/weather";
String apiKey = "99d6d311131e329f5c2eff3fe3eb012453e07f66cc299ef58fcaedc6cc7ba30a";
bool serverEnabled = true;

// Status
bool lastServerOk = false;
unsigned long lastServerSendTime = 0;
int serverPendingCount = 0;

// HTTP client
WiFiClientSecure secureClient;
HTTPClient http;

// ============================================
// Retry Buffer
// ============================================
#define RETRY_BUFFER_SIZE 20

struct BufferedPacket {
    MeteorDataPacket data;
    String timestamp;
    bool valid;
};

BufferedPacket retryBuffer[RETRY_BUFFER_SIZE];
int retryHead = 0;
int retryCount = 0;

void bufferPacket(const MeteorDataPacket& data) {
    retryBuffer[retryHead].data = data;
    retryBuffer[retryHead].timestamp = rtcGetTimestamp();
    retryBuffer[retryHead].valid = true;
    
    retryHead = (retryHead + 1) % RETRY_BUFFER_SIZE;
    if (retryCount < RETRY_BUFFER_SIZE) {
        retryCount++;
    }
    
    serverPendingCount = retryCount;
    Serial.printf("[Server] Buffered packet (queue: %d)\n", retryCount);
}

bool sendPacketToServer(const MeteorDataPacket& data, const String& timestamp) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["packetId"] = data.packetId;
    doc["tempAire"] = data.tempAire;
    doc["humAire"] = data.humAire;
    doc["tempSuelo"] = data.tempSuelo;
    doc["vwcSuelo"] = data.vwcSuelo;
    doc["timestamp"] = timestamp;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Send HTTP POST
    String endpoint = serverUrl + "/api/data/ingest";

    http.begin(secureClient, endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", apiKey);
    http.setTimeout(10000);

    Serial.printf("[Server] POST to %s\n", endpoint.c_str());
    int httpCode = http.POST(jsonPayload);

    if (httpCode == 200) {
        String response = http.getString();
        Serial.printf("[Server] Response: %s\n", response.c_str());
        http.end();
        return true;
    } else {
        Serial.printf("[Server] HTTP Error: %d\n", httpCode);
        http.end();
        return false;
    }
}

void flushRetryBuffer() {
    if (retryCount == 0) return;
    
    Serial.printf("[Server] Flushing buffer (%d packets)...\n", retryCount);
    
    int sent = 0;
    int startIdx = (retryHead - retryCount + RETRY_BUFFER_SIZE) % RETRY_BUFFER_SIZE;
    
    for (int i = 0; i < retryCount; i++) {
        int idx = (startIdx + i) % RETRY_BUFFER_SIZE;
        if (retryBuffer[idx].valid) {
            if (sendPacketToServer(retryBuffer[idx].data, retryBuffer[idx].timestamp)) {
                retryBuffer[idx].valid = false;
                sent++;
            } else {
                break;  // Stop on failure
            }
        }
    }
    
    retryCount -= sent;
    if (retryCount < 0) retryCount = 0;
    serverPendingCount = retryCount;
    
    if (sent > 0) {
        Serial.printf("[Server] Flushed %d buffered packets\n", sent);
    }
}

// ============================================
// Public API
// ============================================

void serverClientInit() {
    // Allow insecure connection (skip certificate validation)
    secureClient.setInsecure();
    
    // Initialize retry buffer
    for (int i = 0; i < RETRY_BUFFER_SIZE; i++) {
        retryBuffer[i].valid = false;
    }
    
    loadServerSettings();
    Serial.println("[Server] Client initialized");
    if (serverEnabled && !apiKey.isEmpty()) {
        Serial.printf("[Server] URL: %s\n", serverUrl.c_str());
    } else {
        Serial.println("[Server] Not configured or disabled");
    }
}

void loadServerSettings() {
    Preferences prefs;
    prefs.begin("server-cfg", true);

    String savedUrl = prefs.getString("url", "");
    String savedKey = prefs.getString("apiKey", "");
    
    if (!savedUrl.isEmpty()) {
        serverUrl = savedUrl;
    }
    if (!savedKey.isEmpty()) {
        apiKey = savedKey;
    }
    serverEnabled = prefs.getBool("enabled", true);

    prefs.end();
}

void saveServerSettings(const String& url, const String& key, bool enabled) {
    Preferences prefs;
    prefs.begin("server-cfg", false);

    prefs.putString("url", url);
    prefs.putString("apiKey", key);
    prefs.putBool("enabled", enabled);

    prefs.end();

    serverUrl = url;
    apiKey = key;
    serverEnabled = enabled;

    Serial.println("[Server] Settings saved");
}

bool sendToServer(const MeteorDataPacket& data) {
    if (!serverEnabled || serverUrl.isEmpty() || apiKey.isEmpty()) {
        Serial.println("[Server] Not configured");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Server] WiFi not connected, buffering");
        bufferPacket(data);
        lastServerOk = false;
        return false;
    }

    String timestamp = rtcGetTimestamp();
    
    if (sendPacketToServer(data, timestamp)) {
        Serial.println("[Server] Data sent OK");
        lastServerOk = true;
        lastServerSendTime = millis();
        
        // Flush buffered packets
        if (retryCount > 0) {
            flushRetryBuffer();
        }
        return true;
    } else {
        Serial.println("[Server] Send failed, buffering");
        bufferPacket(data);
        lastServerOk = false;
        return false;
    }
}

int getPendingCount() {
    return retryCount;
}
