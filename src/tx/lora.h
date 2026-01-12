#ifndef TX_LORA_H
#define TX_LORA_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../shared/config.h"

// LoRa Radio Instance (extern for access from main)
extern SX1262 radio;

// Configuration State
extern int currentSF;
extern float currentBW;
extern unsigned long measureInterval;  // Interval between sensor readings (SD)
extern unsigned long txInterval;       // Interval between LoRa transmissions

// Transmission State
extern int lastTxState;
extern unsigned long packetCounter;
extern unsigned long timeOfLastTx;

// ISR Flag
extern volatile bool operationDone;

// Initialize LoRa radio
bool loraInit();

// Apply current SF/BW config
void applyLoRaConfig();

// Transmit data packet, returns state
int loraTransmit(const MeteorDataPacket& data);

// Start receiving mode
void loraStartReceive();

// Put radio to sleep
void loraSleep();

#endif
