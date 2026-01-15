#ifndef TX_SENSORS_H
#define TX_SENSORS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include "../shared/config.h"

// --- Pin Definitions ---
#define ONE_WIRE_BUS 7
#define DHT_PIN 6
#define DHT_TYPE DHT22
#define EC5_PIN 5

// Calibration Constants
#define VWC_SLOPE 0.00119f
#define VWC_OFFSET 0.400f

// Initialize sensors
void sensorsInit();

// Read all sensors and populate currentData
void readSensors(MeteorDataPacket& data);

#endif
