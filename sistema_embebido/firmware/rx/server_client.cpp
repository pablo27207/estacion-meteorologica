#include "server_client.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Configuration - Default values for vivero-olivos station
String serverUrl = "https://gipis.unp.edu.ar/weather";
String apiKey = "99d6d311131e329f5c2eff3fe3eb012453e07f66cc299ef58fcaedc6cc7ba30a";
bool serverEnabled = true;

// Pending config from server
ServerConfig serverConfig = { false, 9, 125.0, 600000 };

// HTTP client with secure support
WiFiClientSecure secureClient;
HTTPClient http;

// ============================================
// Retry Buffer - Circular buffer for failed packets
// ============================================
#define RETRY_BUFFER_SIZE 20

struct BufferedPacket {
    MeteorDataPacket data;
    float rssi;
    float snr;
    float freqError;
    bool valid;
};

BufferedPacket retryBuffer[RETRY_BUFFER_SIZE];
int retryHead = 0;  // Next write position
int retryCount = 0; // Number of items in buffer

// Add packet to retry buffer
void bufferPacket(const MeteorDataPacket& data, float rssi, float snr, float freqError) {
    retryBuffer[retryHead].data = data;
    retryBuffer[retryHead].rssi = rssi;
    retryBuffer[retryHead].snr = snr;
    retryBuffer[retryHead].freqError = freqError;
    retryBuffer[retryHead].valid = true;
    
    retryHead = (retryHead + 1) % RETRY_BUFFER_SIZE;
    if (retryCount < RETRY_BUFFER_SIZE) {
        retryCount++;
    }
    
    Serial.printf("[Server] Buffered packet (queue: %d)\n", retryCount);
}

// Internal function to send a single packet
bool sendPacketToServer(const MeteorDataPacket& data, float rssi, float snr, float freqError) {
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
    doc["rssi"] = rssi;
    doc["snr"] = snr;
    doc["freqError"] = freqError;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Send HTTP POST
    String endpoint = serverUrl + "/api/data/ingest";

    http.begin(secureClient, endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", apiKey);
    http.setTimeout(10000);

    int httpCode = http.POST(jsonPayload);

    if (httpCode == 200) {
        String response = http.getString();
        StaticJsonDocument<256> respDoc;
        DeserializationError err = deserializeJson(respDoc, response);

        if (!err && respDoc.containsKey("config")) {
            JsonObject cfg = respDoc["config"];
            serverConfig.sf = cfg["sf"] | 9;
            serverConfig.bw = cfg["bw"] | 125.0;
            serverConfig.interval = cfg["interval"] | 600000;
            serverConfig.hasPending = true;
        }

        http.end();
        return true;
    } else {
        http.end();
        return false;
    }
}

// Try to send buffered packets
void flushRetryBuffer() {
    if (retryCount == 0) return;
    
    Serial.printf("[Server] Flushing buffer (%d packets)...\n", retryCount);
    
    int sent = 0;
    int startIdx = (retryHead - retryCount + RETRY_BUFFER_SIZE) % RETRY_BUFFER_SIZE;
    
    for (int i = 0; i < retryCount; i++) {
        int idx = (startIdx + i) % RETRY_BUFFER_SIZE;
        if (retryBuffer[idx].valid) {
            if (sendPacketToServer(retryBuffer[idx].data, retryBuffer[idx].rssi, 
                                   retryBuffer[idx].snr, retryBuffer[idx].freqError)) {
                retryBuffer[idx].valid = false;
                sent++;
            } else {
                // Stop trying if we hit a failure
                break;
            }
        }
    }
    
    // Recalculate count (remove sent packets)
    retryCount -= sent;
    if (retryCount < 0) retryCount = 0;
    
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
    if (serverEnabled) {
        Serial.print("[Server] URL: ");
        Serial.println(serverUrl);
    } else {
        Serial.println("[Server] Remote server disabled");
    }
}

void loadServerSettings() {
    Preferences prefs;
    prefs.begin("server-cfg", true);

    String savedUrl = prefs.getString("url", "");
    String savedKey = prefs.getString("apiKey", "");
    
    if (!savedUrl.isEmpty()) {
        serverUrl = savedUrl;
        apiKey = savedKey;
        serverEnabled = prefs.getBool("enabled", true);
    }

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

bool sendToServer(const MeteorDataPacket& data, float rssi, float snr, float freqError) {
    if (!serverEnabled || serverUrl.isEmpty() || apiKey.isEmpty()) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Server] WiFi not connected, buffering");
        bufferPacket(data, rssi, snr, freqError);
        return false;
    }

    // Try to send current packet
    if (sendPacketToServer(data, rssi, snr, freqError)) {
        Serial.println("[Server] Data sent OK");
        
        // Success! Try to flush any buffered packets
        if (retryCount > 0) {
            flushRetryBuffer();
        }
        return true;
    } else {
        // Failed, buffer it for retry
        Serial.println("[Server] Send failed, buffering");
        bufferPacket(data, rssi, snr, freqError);
        return false;
    }
}

bool checkServerConfig() {
    if (serverConfig.hasPending) {
        serverConfig.hasPending = false;
        return true;
    }
    return false;
}

int getPendingCount() {
    return retryCount;
}
