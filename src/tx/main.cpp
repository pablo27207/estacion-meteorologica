/**
 * TX Node - Main Entry Point
 * 
 * This is the transmitter node of the weather station.
 * Modules:
 *   - sensors: DHT22, DS18B20, EC-5
 *   - sd_logger: CSV data logging
 *   - lora: SX1262 communication
 *   - display: OLED UI
 *   - button: User input handling
 */

#include <Arduino.h>
#include "../shared/config.h"
#include "sensors.h"
#include "sd_logger.h"
#include "lora.h"
#include "display.h"
#include "button.h"

// Current sensor readings
MeteorDataPacket currentData;

// Timing
unsigned long lastTxTime = 0;

void setup() {
    Serial.begin(115200);
    
    // 1. Power Control
    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW); // Enable VExt
    pinMode(35, OUTPUT);
    digitalWrite(35, LOW); // LED Off
    delay(100);
    
    // 2. Initialize Modules
    displayInit();
    sensorsInit();
    sdInit();
    
    if (!loraInit()) {
        display.drawStr(0, 30, "LoRa Error!");
        display.sendBuffer();
        while (true);
    }
    
    buttonInit();
    lastInteraction = millis();
}

void loop() {
    // 1. Handle User Input
    handleButton();
    
    // 2. Check Screen Timeout
    checkScreenTimeout();
    
    // 3. Transmit Cycle
    if (millis() - lastTxTime > txInterval || lastTxTime == 0) {
        // Read Sensors
        readSensors(currentData);
        currentData.packetId = packetCounter;
        currentData.interval = txInterval;
        
        // Transmit
        int state = loraTransmit(currentData);
        lastTxTime = millis();
        
        if (state == RADIOLIB_ERR_NONE) {
            // Log to SD
            logToSD(currentData);
            
            // Listen for Remote Commands (2 second window)
            loraStartReceive();
            unsigned long rxStart = millis();
            
            while (millis() - rxStart < 2000) {
                handleButton(); // Keep UI alive
                
                if (operationDone) {
                    operationDone = false;
                    ConfigPacket packet;
                    int rxState = radio.readData((uint8_t*)&packet, sizeof(ConfigPacket));
                    
                    if (rxState == RADIOLIB_ERR_NONE && packet.magic == CMD_MAGIC) {
                        Serial.println("CMD Received!");
                        
                        // Visual Feedback
                        display.clearBuffer();
                        display.setFont(u8g2_font_ncenB14_tr);
                        display.drawStr(10, 40, "CMD RECIBIDO!");
                        display.sendBuffer();
                        delay(1000);
                        
                        // Apply Configuration
                        bool loraChanged = false;
                        if (packet.sf != currentSF) { currentSF = packet.sf; loraChanged = true; }
                        if (packet.bw != currentBW) { currentBW = packet.bw; loraChanged = true; }
                        txInterval = packet.interval;
                        
                        Serial.printf("New Config: SF%d BW%.0f Int%lu\n", currentSF, currentBW, txInterval);
                        if (loraChanged) applyLoRaConfig();
                        
                        break;
                    }
                }
                delay(10);
            }
            loraSleep();
        } else {
            Serial.println("TX Fail");
            loraSleep();
        }
    }
    
    // 4. Render Display
    renderScreen(currentData, lastTxTime, txInterval);
}
