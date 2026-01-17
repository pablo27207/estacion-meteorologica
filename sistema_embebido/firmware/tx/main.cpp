/**
 * TX Node - Main Entry Point (WiFi Mode)
 * 
 * This is the transmitter node of the weather station.
 * Sends data directly to server via WiFi/HTTPS.
 * 
 * Modules:
 *   - sensors: DHT22, DS18B20, EC-5
 *   - sd_logger: CSV data logging
 *   - wifi_manager: WiFi connectivity + config portal
 *   - server_client: HTTPS data upload
 *   - display: OLED UI
 *   - button: User input handling
 *   - rtc: Real time clock
 */

#include <Arduino.h>
#include "../shared/config.h"
#include "sensors.h"
#include "sd_logger.h"
#include "display.h"
#include "button.h"
#include "rtc.h"
#include "wifi_manager.h"
#include "server_client.h"

// Current sensor readings
MeteorDataPacket currentData;

// Timing
unsigned long lastMeasureTime = 0;
unsigned long lastSendTime = 0;

// Send interval (ms) - configurable via web interface
unsigned long sendInterval = 60000;  // 1 minute default

// Packet counter
unsigned long packetCounter = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== TX WiFi Mode ===");
    
    // 1. Power Control
    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW); // Enable VExt
    pinMode(35, OUTPUT);
    digitalWrite(35, LOW); // LED Off
    delay(100);
    
    // 2. Initialize Display first (for status messages)
    displayInit();
    
    // 3. Initialize RTC
    rtcInit();
    
    // 4. Initialize sensors
    sensorsInit();
    
    // 5. Initialize SD card
    sdInit();
    
    // 6. Initialize WiFi
    bool wifiOk = wifiInit();
    delay(1000);
    
    // 7. Initialize web server
    webServerInit();
    
    // 8. Initialize server client
    serverClientInit();
    
    // 9. Initialize button
    buttonInit();
    lastInteraction = millis();
    
    Serial.println("TX Ready. Commands: SET_TIME,YYYY,MM,DD,HH,MM,SS | GET_TIME");
    if (wifiOk) {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
}

void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("SET_TIME,")) {
            // Format: SET_TIME,YYYY,MM,DD,HH,MM,SS
            int parts[6];
            int idx = 0;
            int start = 9;
            
            for (int i = 0; i < 6 && idx < 6; i++) {
                int comma = cmd.indexOf(',', start);
                if (comma == -1) comma = cmd.length();
                parts[idx++] = cmd.substring(start, comma).toInt();
                start = comma + 1;
            }
            
            if (idx == 6) {
                rtcSetTime(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
                Serial.printf("RTC Set: %04d-%02d-%02d %02d:%02d:%02d\n",
                              parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]);
            } else {
                Serial.println("Error: Use SET_TIME,YYYY,MM,DD,HH,MM,SS");
            }
        } else if (cmd == "GET_TIME") {
            Serial.printf("RTC: %s\n", rtcGetTimestamp().c_str());
        } else if (cmd == "SEND") {
            // Force send now
            readSensors(currentData);
            currentData.packetId = ++packetCounter;
            sendToServer(currentData);
        } else if (cmd == "STATUS") {
            Serial.printf("WiFi: %s RSSI: %d\n", 
                         WiFi.status() == WL_CONNECTED ? "OK" : "DISC",
                         WiFi.RSSI());
            Serial.printf("Server: %s Pending: %d\n",
                         lastServerOk ? "OK" : "FAIL",
                         getPendingCount());
        }
    }
}

void loop() {
    // 1. Handle User Input
    handleButton();
    handleSerial();
    
    // 2. Check Screen Timeout
    checkScreenTimeout();
    
    // 3. Maintain WiFi connection
    wifiLoop();
    
    // 4. Measurement Cycle (every measureInterval)
    if (millis() - lastMeasureTime > measureInterval || lastMeasureTime == 0) {
        readSensors(currentData);
        currentData.packetId = ++packetCounter;
        
        // Log to SD
        logToSD(currentData);
        lastMeasureTime = millis();
        
        Serial.printf("[%s] Measured: T=%.1f H=%.1f\n", 
                      rtcGetTimestamp().c_str(), currentData.tempAire, currentData.humAire);
    }
    
    // 5. Server Send Cycle (every sendInterval)
    if (millis() - lastSendTime > sendInterval || lastSendTime == 0) {
        // Ensure fresh data
        if (lastMeasureTime == 0 || millis() - lastMeasureTime > measureInterval) {
            readSensors(currentData);
            currentData.packetId = ++packetCounter;
        }
        
        // Send to server
        if (wifiConnected) {
            bool ok = sendToServer(currentData);
            Serial.printf("[%s] Server send: %s\n", 
                         rtcGetTimestamp().c_str(), 
                         ok ? "OK" : "FAIL");
        }
        
        lastSendTime = millis();
    }
    
    // 6. Render Display
    renderScreen(currentData, lastSendTime, sendInterval);
}
