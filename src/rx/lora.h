#ifndef RX_LORA_H
#define RX_LORA_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../shared/config.h"

// LoRa Radio Instance
extern SX1262 radio;

// Configuration State
extern int currentSF;
extern float currentBW;
extern unsigned long txInterval;

// Reception State
extern MeteorDataPacket lastData;
extern unsigned long lastPacketTime;
extern float lastRSSI;
extern float lastSNR;
extern float lastFreqError;
extern bool newData;

// Reliability Stats
extern unsigned long lastRxPacketId;
extern uint32_t packetsReceived;
extern uint32_t packetsLost;

// Remote Command
extern ConfigPacket pendingCmd;
extern bool hasPendingCmd;

// ISR Flag
extern volatile bool operationDone;

// Initialize LoRa radio
bool loraInit();

// Apply current SF/BW config and restart receive
void applyLoRaConfig();

// Start receiving mode
void loraStartReceive();

// Process received packet (call when operationDone is true)
// Returns true if valid data received
bool loraProcessReceive();

// Send pending command to TX
bool loraSendCommand();

#endif
