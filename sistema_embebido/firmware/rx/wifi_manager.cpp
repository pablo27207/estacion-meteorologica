#include "wifi_manager.h"
#include "lora.h"
#include "display.h"
#include "web_interface.h"
#include "server_client.h"

// Web Server Instance
AsyncWebServer server(80);

// Connection State
bool wifiConnected = false;

bool wifiInit() {
    Preferences preferences;
    preferences.begin("wifi-config", false);
    
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");
    preferences.end();
    
    if (ssid == "") {
        Serial.println("No SSID saved. Starting AP.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("GIPIS-Estacion");
        
        display.clearBuffer();
        display.setFont(u8g2_font_ncenB08_tr);
        display.drawStr(0, 15, "MODO CONFIG");
        display.drawStr(0, 30, "WiFi: GIPIS-Estacion");
        display.drawStr(0, 45, "IP: 192.168.4.1");
        display.sendBuffer();
        
        wifiConnected = false;
        return false;
    }
    
    Serial.print("Connecting to: "); Serial.println(ssid);
    display.drawStr(0, 30, "Conectando WiFi...");
    display.drawStr(0, 45, ssid.c_str());
    display.sendBuffer();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 15) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        
        // Configure DNS servers (UNPSJB DNS + Google DNS as fallback)
        IPAddress dns1(10, 15, 24, 16);  // UNPSJB DNS
        IPAddress dns2(8, 8, 8, 8);      // Google DNS
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2);
        Serial.println("[WiFi] DNS configured: 10.15.24.16, 8.8.8.8");
        
        display.drawStr(0, 30, "WiFi OK!");
        display.drawStr(0, 45, WiFi.localIP().toString().c_str());
        display.sendBuffer();
        wifiConnected = true;
        return true;
    } else {
        // Fallback to AP mode
        Serial.println("\nConnection failed. Starting AP.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("GIPIS-Estacion");
        wifiConnected = false;
        return false;
    }
}

void webServerInit() {
    if (wifiConnected) {
        // --- CLIENT MODE (Dashboard) ---
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html);
        });
        
        server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
            String json = "{";
            json += "\"tempAire\":" + String(lastData.tempAire) + ",";
            json += "\"humAire\":" + String(lastData.humAire) + ",";
            json += "\"tempSuelo\":" + String(lastData.tempSuelo) + ",";
            json += "\"vwcSuelo\":" + String(lastData.vwcSuelo) + ",";
            json += "\"packetId\":" + String(lastData.packetId) + ",";
            json += "\"rssi\":" + String(lastRSSI) + ",";
            json += "\"snr\":" + String(lastSNR) + ",";
            json += "\"freqErr\":" + String(lastFreqError) + ",";
            
            float reliability = 100.0;
            uint32_t total = packetsReceived + packetsLost;
            if (total > 0) reliability = (float)packetsReceived * 100.0 / (float)total;
            json += "\"reliability\":" + String(reliability) + ",";
            
            json += "\"sf\":" + String(currentSF) + ",";
            json += "\"bw\":" + String(currentBW) + ",";
            json += "\"interval\":" + String(txInterval / 60000.0);
            json += "}";
            request->send(200, "application/json", json);
        });
        
        server.on("/set_remote", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("int") && request->hasParam("sf") && request->hasParam("bw")) {
                float iVal = request->getParam("int")->value().toFloat();
                int sVal = request->getParam("sf")->value().toInt();
                float bVal = request->getParam("bw")->value().toFloat();
                
                pendingCmd.magic = CMD_MAGIC;
                pendingCmd.interval = (uint32_t)(iVal * 60000.0);
                pendingCmd.sf = (uint8_t)sVal;
                pendingCmd.bw = bVal;
                
                hasPendingCmd = true;
                txInterval = pendingCmd.interval;
                
                request->send(200, "text/plain", "Encolado. Esperando prox dato...");
            } else {
                request->send(400, "text/plain", "Faltan parametros");
            }
        });
        
        // Get server configuration
        server.on("/get_server", HTTP_GET, [](AsyncWebServerRequest *request) {
            String json = "{";
            json += "\"url\":\"" + serverUrl + "\",";
            json += "\"apiKey\":\"" + apiKey + "\",";
            json += "\"enabled\":" + String(serverEnabled ? "true" : "false");
            json += "}";
            request->send(200, "application/json", json);
        });
        
        // Save server configuration
        server.on("/save_server", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("url") && request->hasParam("apiKey")) {
                String url = request->getParam("url")->value();
                String key = request->getParam("apiKey")->value();
                bool enabled = request->hasParam("enabled") && request->getParam("enabled")->value() == "1";
                
                saveServerSettings(url, key, enabled);
                request->send(200, "text/plain", "Configuración guardada");
            } else {
                request->send(400, "text/plain", "Faltan parámetros (url, apiKey)");
            }
        });
    } else {
        // --- AP MODE (Configuration) ---
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", setup_wifi_html);
        });
        
        server.on("/start_scan", HTTP_GET, [](AsyncWebServerRequest *request) {
            WiFi.scanNetworks(true);
            request->send(200, "text/plain", "OK");
        });
        
        server.on("/scan_results", HTTP_GET, [](AsyncWebServerRequest *request) {
            int n = WiFi.scanComplete();
            if (n == -2) {
                WiFi.scanNetworks(true);
                request->send(200, "application/json", "{\"status\":\"running\"}");
            } else if (n == -1) {
                request->send(200, "application/json", "{\"status\":\"running\"}");
            } else {
                String json = "[";
                for (int i = 0; i < n; ++i) {
                    if (i) json += ",";
                    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
                }
                json += "]";
                WiFi.scanDelete();
                request->send(200, "application/json", json);
            }
        });
        
        server.on("/save_wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("ssid") && request->hasParam("pass")) {
                Preferences p;
                p.begin("wifi-config", false);
                p.putString("ssid", request->getParam("ssid")->value());
                p.putString("pass", request->getParam("pass")->value());
                p.end();
                request->send(200, "text/plain", "Saved. Restarting...");
                delay(1000);
                ESP.restart();
            } else {
                request->send(400, "text/plain", "Missing ssid or pass");
            }
        });
    }
    
    server.begin();
}

void wifiReset() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB14_tr);
    display.drawStr(0, 40, "RESET WIFI!");
    display.sendBuffer();
    
    Preferences p;
    p.begin("wifi-config", false);
    p.clear();
    p.end();
    
    delay(2000);
    ESP.restart();
}
