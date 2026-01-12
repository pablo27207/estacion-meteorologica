#include "lora.h"
#include <SPI.h>

// Radio Instance
SX1262 radio = new Module(RADIO_NSS, RADIO_DIO_1, RADIO_RST, RADIO_BUSY);

// Configuration State
int currentSF = LORA_SF;
float currentBW = LORA_BW;
unsigned long txInterval = 10000;

// Transmission State
int lastTxState = 0;
unsigned long packetCounter = 0;
unsigned long timeOfLastTx = 0;

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
        return true;
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        return false;
    }
}

void applyLoRaConfig() {
    radio.standby();
    radio.setSpreadingFactor(currentSF);
    radio.setBandwidth(currentBW);
    Serial.printf("Applied: SF%d BW%.0f Int%lu\n", currentSF, currentBW, txInterval);
}

int loraTransmit(const MeteorDataPacket& data) {
    int state = radio.transmit((uint8_t*)&data, sizeof(MeteorDataPacket));
    lastTxState = state;
    timeOfLastTx = millis();
    if (state == RADIOLIB_ERR_NONE) {
        packetCounter++;
    }
    return state;
}

void loraStartReceive() {
    radio.startReceive();
}

void loraSleep() {
    radio.sleep();
}
