/**
 * WiFi Manager for TX Node
 * Adapted from RX - handles AP mode config portal and STA mode connection
 */

#include "wifi_manager.h"
#include "display.h"
#include "web_interface.h"
#include "server_client.h"
#include "sensors.h"
#include "rtc.h"

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
        WiFi.softAP("GIPIS-TX-Config");
        
        display.clearBuffer();
        display.setFont(u8g2_font_ncenB08_tr);
        display.drawStr(0, 15, "MODO CONFIG");
        display.drawStr(0, 30, "WiFi: GIPIS-TX-Config");
        display.drawStr(0, 45, "IP: 192.168.4.1");
        display.sendBuffer();
        
        wifiConnected = false;
        return false;
    }
    
    Serial.print("Connecting to: "); Serial.println(ssid);
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 15, "Conectando WiFi...");
    display.drawStr(0, 30, ssid.c_str());
    display.sendBuffer();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP: "); Serial.println(WiFi.localIP());
        
        display.clearBuffer();
        display.drawStr(0, 15, "WiFi OK!");
        display.drawStr(0, 30, WiFi.localIP().toString().c_str());
        display.sendBuffer();
        
        wifiConnected = true;
        return true;
    } else {
        Serial.println("\nConnection failed. Starting AP.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("GIPIS-TX-Config");
        
        display.clearBuffer();
        display.drawStr(0, 15, "WiFi FAIL");
        display.drawStr(0, 30, "AP: GIPIS-TX-Config");
        display.drawStr(0, 45, "192.168.4.1");
        display.sendBuffer();
        
        wifiConnected = false;
        return false;
    }
}

void webServerInit() {
    if (wifiConnected) {
        // --- STA MODE (Status page) ---
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html);
        });
        
        // Current sensor data
        server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
            extern MeteorDataPacket currentData;
            
            String json = "{";
            json += "\"tempAire\":" + String(currentData.tempAire) + ",";
            json += "\"humAire\":" + String(currentData.humAire) + ",";
            json += "\"tempSuelo\":" + String(currentData.tempSuelo) + ",";
            json += "\"vwcSuelo\":" + String(currentData.vwcSuelo) + ",";
            json += "\"wifiRSSI\":" + String(WiFi.RSSI()) + ",";
            json += "\"serverOk\":" + String(lastServerOk ? "true" : "false");
            json += "}";
            request->send(200, "application/json", json);
        });
        
        // Get server config
        server.on("/get_server", HTTP_GET, [](AsyncWebServerRequest *request) {
            String json = "{";
            json += "\"url\":\"" + serverUrl + "\",";
            json += "\"apiKey\":\"" + apiKey + "\",";
            json += "\"enabled\":" + String(serverEnabled ? "true" : "false");
            json += "}";
            request->send(200, "application/json", json);
        });
        
        // Save server config
        server.on("/save_server", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (request->hasParam("url") && request->hasParam("apiKey")) {
                String url = request->getParam("url")->value();
                String key = request->getParam("apiKey")->value();
                bool enabled = request->hasParam("enabled") && request->getParam("enabled")->value() == "1";
                
                saveServerSettings(url, key, enabled);
                request->send(200, "text/plain", "Configuración guardada");
            } else {
                request->send(400, "text/plain", "Faltan parámetros");
            }
        });
    } else {
        // --- AP MODE (WiFi Configuration) ---
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
    Serial.println("Web server started");
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

void wifiLoop() {
    // Reconnect if disconnected
    if (wifiConnected && WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        WiFi.reconnect();
        
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 10) {
            delay(500);
            retry++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Reconnect failed");
        }
    }
}
