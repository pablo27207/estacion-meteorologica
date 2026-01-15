#include "server_client.h"
#include <Preferences.h>
#include <WiFi.h>

// Configuration
String serverUrl = "";
String apiKey = "";
bool serverEnabled = false;

// Pending config
ServerConfig serverConfig = { false, 9, 125.0, 600000 };

// HTTP client
HTTPClient http;

void serverClientInit() {
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
    prefs.begin("server-cfg", true);  // Read-only

    serverUrl = prefs.getString("url", "");
    apiKey = prefs.getString("apiKey", "");
    serverEnabled = prefs.getBool("enabled", false);

    prefs.end();
}

void saveServerSettings(const String& url, const String& key, bool enabled) {
    Preferences prefs;
    prefs.begin("server-cfg", false);

    prefs.putString("url", url);
    prefs.putString("apiKey", key);
    prefs.putBool("enabled", enabled);

    prefs.end();

    // Update runtime values
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
        Serial.println("[Server] WiFi not connected, skipping");
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

    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", apiKey);
    http.setTimeout(10000);  // 10 second timeout

    int httpCode = http.POST(jsonPayload);

    if (httpCode == 200) {
        // Parse response for pending config
        String response = http.getString();
        StaticJsonDocument<256> respDoc;
        DeserializationError err = deserializeJson(respDoc, response);

        if (!err && respDoc.containsKey("config")) {
            JsonObject cfg = respDoc["config"];
            serverConfig.sf = cfg["sf"] | 9;
            serverConfig.bw = cfg["bw"] | 125.0;
            serverConfig.interval = cfg["interval"] | 600000;
            serverConfig.hasPending = true;

            Serial.printf("[Server] Config received: SF%d BW%.0f Int%lu\n",
                serverConfig.sf, serverConfig.bw, serverConfig.interval);
        }

        http.end();
        Serial.println("[Server] Data sent successfully");
        return true;

    } else {
        Serial.printf("[Server] Error sending data: %d\n", httpCode);
        http.end();
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
