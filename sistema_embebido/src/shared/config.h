#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#include <Arduino.h>

// --- LoRa Configuration (Heltec V3 / SX1262) ---
// Frecuencia: 433.0 MHz (User specific)
#define LORA_FREQ 433.0
#define LORA_BW 125.0
#define LORA_SF 9
#define LORA_CR 7
#define LORA_SYNC_WORD 0x12 
#define LORA_POWER 22 
#define LORA_PREAMBLE_LEN 8
#define LORA_GAIN 0

// Pines LoRa en Heltec V3
#define RADIO_DIO_1 14
#define RADIO_NSS   8
#define RADIO_RST   12
#define RADIO_BUSY  13
#define RADIO_SCLK  9
#define RADIO_MISO  11
#define RADIO_MOSI  10
#define LORA_TCXO_VOLTAGE 1.8

// Pines Display OLED
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_CTRL 36 // Control de energía VExt (OLED + Sensores)

// --- Estructura de Datos (Protocolo) ---
// Esta estructura DEBE ser idéntica en TX y RX
struct MeteorDataPacket {
    float tempAire;
    float humAire;
    float tempSuelo;
    float vwcSuelo;
    uint32_t rawSuelo;
    unsigned long packetId;
    uint32_t interval; // Sync TX interval state
};

// --- Estructura del Paquete de Control Unificado (RX -> TX) ---
struct __attribute__((packed)) ConfigPacket {
  uint32_t magic;      // 0xCAFEBABE para validar
  uint32_t interval;   // ms
  uint8_t  sf;         // 7-12
  float    bw;         // 125, 250, 500
};

#define CMD_MAGIC 0xCAFEBABE

#endif
