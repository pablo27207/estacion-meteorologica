#ifndef TX_SENSORS_H
#define TX_SENSORS_H

#include <Arduino.h>
#include <SDI12.h>
#include <DHT.h>
#include "../shared/config.h"

// TEROS 12 SDI-12 (reuses former EC-5 pin)
#define TEROS12_PIN  5
#define TEROS12_ADDR '0'

// DHT22 air sensor (unchanged)
#define DHT_PIN  6
#define DHT_TYPE DHT22

// PAR radiometer (reuses former DS18B20 pin)
// AD620 instrumentation amplifier: Rg = 200Ω → G = 49400/200 + 1 = 248
// Sensor sensitivity: 2.98 µV/(µmol/m²s) → 335.57 µmol/m²s per mV before amp
//
// PAR_ZERO_OFFSET_MV: DC bias from AD620 single-supply (+5V without -5V).
// Calibrate: cover the sensor completely, read the displayed µV value,
// divide by 1000 to get mV, and set that value here.
// Example: if display shows 3276 µV in darkness → set 3.276f
#define PAR_PIN             4
#define PAR_AD620_GAIN      248.0f
#define PAR_SENSITIVITY     335.57f  // µmol/m²s per mV at sensor output
#define PAR_SAMPLES         64
#define PAR_ZERO_OFFSET_MV  0.0f    // mV — set after dark calibration

extern float parSensorUV; // µV at sensor input (before AD620) — debug
extern float parRawMV;    // raw ADC reading at GPIO7 in mV — debug

void sensorsInit();
void readSensors(MeteorDataPacket& data);

#endif
