#include "lora.h"
#include <SPI.h>

// Radio Instance
SX1262 radio = new Module(RADIO_NSS, RADIO_DIO_1, RADIO_RST, RADIO_BUSY);

// Configuration State
int currentSF = LORA_SF;
float currentBW = LORA_BW;
unsigned long txInterval = 10000;

// Reception State
MeteorDataPacket lastData;
unsigned long lastPacketTime = 0;
float lastRSSI = 0;
float lastSNR = 0;
float lastFreqError = 0;
bool newData = false;

// Reliability Stats
unsigned long lastRxPacketId = 0;
uint32_t packetsReceived = 0;
uint32_t packetsLost = 0;

// Remote Command
ConfigPacket pendingCmd;
bool hasPendingCmd = false;

// ISR Flag
volatile bool operationDone = false;
static void setFlag() { operationDone = true; }

bool loraInit() {
    Serial.print(F("[SX1262] Initializing ... "));
    SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
    
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 
                            LORA_SYNC_WORD, LORA_POWER, LORA_PREAMBLE_LEN, 
                            LORA_TCXO_VOLTAGE);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        radio.setDio2AsRfSwitch(true);
        radio.setDio1Action(setFlag);
        radio.startReceive();
        return true;
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return false;
    }
}

void applyLoRaConfig() {
    Serial.print("Reconfig LoRa... ");
    radio.setSpreadingFactor(currentSF);
    radio.setBandwidth(currentBW);
    radio.startReceive();
    Serial.printf("Done: SF%d BW%.0f\n", currentSF, currentBW);
}

void loraStartReceive() {
    radio.startReceive();
}

bool loraProcessReceive() {
    int state = radio.readData((uint8_t*)&lastData, sizeof(MeteorDataPacket));
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("RX OK");
        newData = true;
        
        // Capture Metrics
        lastPacketTime = millis();
        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();
        lastFreqError = radio.getFrequencyError();
        
        // Packet Loss Calculation
        if (lastData.packetId > lastRxPacketId + 1) {
            if (lastRxPacketId != 0) {
                packetsLost += (lastData.packetId - lastRxPacketId - 1);
            }
        } else if (lastData.packetId < lastRxPacketId) {
            // TX Reset detected
            packetsReceived = 0;
            packetsLost = 0;
        }
        lastRxPacketId = lastData.packetId;
        packetsReceived++;
        
        // Sync State from TX
        txInterval = lastData.interval;
        
        return true;
    } else {
        Serial.print(F("RX Fail: ")); Serial.println(state);
        radio.startReceive();
        return false;
    }
}

bool loraSendCommand() {
    if (!hasPendingCmd) return false;
    
    delay(150); // Wait for TX to open RX window
    Serial.print("Sending CMD... ");
    
    operationDone = false;
    int txState = radio.transmit((uint8_t*)&pendingCmd, sizeof(ConfigPacket));
    
    if (txState == RADIOLIB_ERR_NONE) {
        Serial.println("OK");
        hasPendingCmd = false;
        
        // Apply locally
        bool reinit = false;
        if (pendingCmd.sf != currentSF) { currentSF = pendingCmd.sf; reinit = true; }
        if (pendingCmd.bw != currentBW) { currentBW = pendingCmd.bw; reinit = true; }
        txInterval = pendingCmd.interval;
        
        if (reinit) {
            delay(100);
            applyLoRaConfig();
        }
        
        operationDone = false;
        radio.startReceive();
        return true;
    } else {
        Serial.print("Fail: "); Serial.println(txState);
        operationDone = false;
        radio.startReceive();
        return false;
    }
}
