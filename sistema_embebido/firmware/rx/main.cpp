/**
 * RX Gateway - Main Entry Point
 * 
 * This is the receiver gateway of the weather station.
 * Modules:
 *   - lora: SX1262 reception and command sending
 *   - wifi_manager: WiFi connection and web server
 *   - display: OLED UI
 *   - button: User input handling
 *   - server_client: Remote server communication
 *   - web_interface: HTML templates (header only)
 */

#include <Arduino.h>
#include "../shared/config.h"
#include "lora.h"
#include "display.h"
#include "button.h"
#include "wifi_manager.h"
#include "server_client.h"

// Display update timing
unsigned long lastDisplayUpdate = 0;

void setup() {
    Serial.begin(115200);
    
    // 1. Power Control
    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW);
    delay(100);
    
    // 2. Initialize Modules
    displayInit();
    wifiInit();
    serverClientInit();  // Initialize remote server client
    
    if (!loraInit()) {
        display.drawStr(0, 60, "LoRa Init Failed!");
        display.sendBuffer();
    }
    
    webServerInit();
    buttonInit();
    lastInteraction = millis();
}

void loop() {
    // 1. Handle User Input
    handleButton();
    
    // 2. Check Screen Timeout
    checkScreenTimeout();
    
    // 3. LoRa Reception
    if (operationDone) {
        operationDone = false;
        
        if (loraProcessReceive()) {
            // Valid packet received, check for pending command
            if (hasPendingCmd) {
                loraSendCommand();
            } else {
                loraStartReceive();
            }
        }
    }
    
    // 4. Update Display (throttled)
    if (millis() - lastDisplayUpdate > 200) {
        lastDisplayUpdate = millis();
        renderScreen();
    }
}
