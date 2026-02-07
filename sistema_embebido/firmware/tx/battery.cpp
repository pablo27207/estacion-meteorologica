/**
 * Battery Monitor Module for TX Node
 * Uses Heltec V3 internal VBAT_Read on GPIO1
 */

#include "battery.h"

// Cached values
static float cachedVoltage = 0.0;
static uint8_t cachedPercent = 0;

void batteryInit() {
    // Configure ADC for battery reading
    analogReadResolution(12);  // 12-bit (0-4095)
    analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
    
    // Initial reading
    readBatteryVoltage();
    
    Serial.printf("[Battery] Init OK - %.2fV (%d%%)\n", cachedVoltage, cachedPercent);
}

float readBatteryVoltage() {
    // Take multiple samples for stability
    uint32_t sum = 0;
    const int samples = 16;
    
    for (int i = 0; i < samples; i++) {
        sum += analogRead(VBAT_PIN);
        delayMicroseconds(100);
    }
    
    uint32_t avgRaw = sum / samples;
    
    // Convert to voltage
    // ESP32-S3 ADC: 12 bits (0-4095), ~3.3V reference with 11db attenuation
    float adcVoltage = (avgRaw / 4095.0) * 3.3;
    
    // Apply voltage divider ratio
    cachedVoltage = adcVoltage * VBAT_DIVIDER;
    
    // Calculate percentage
    float percent = (cachedVoltage - VBAT_MIN) / (VBAT_MAX - VBAT_MIN) * 100.0;
    cachedPercent = constrain((int)percent, 0, 100);
    
    return cachedVoltage;
}

uint8_t getBatteryPercent() {
    return cachedPercent;
}
