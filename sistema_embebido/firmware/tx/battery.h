#ifndef TX_BATTERY_H
#define TX_BATTERY_H

#include <Arduino.h>

// GPIO1 = VBAT_Read (internal voltage divider on Heltec V3)
#define VBAT_PIN 1

// Voltage divider ratio (calibrated: 4.17V real / 3.3V ADC)
#define VBAT_DIVIDER 1.264

// Li-Ion voltage range
#define VBAT_MAX 4.2
#define VBAT_MIN 3.0

// Initialize battery ADC
void batteryInit();

// Read battery voltage (returns volts)
float readBatteryVoltage();

// Get battery percentage (0-100)
uint8_t getBatteryPercent();

#endif
