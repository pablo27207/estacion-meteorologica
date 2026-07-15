#include "sensors.h"

static SDI12 sdi12(TEROS12_PIN);
static DHT sensorAire(DHT_PIN, DHT_TYPE);

float parSensorUV = 0;
float parRawMV    = 0;

void sensorsInit() {
    sensorAire.begin();
    sdi12.begin();
    analogSetPinAttenuation(PAR_PIN, ADC_11db);
    delay(500); // Allow TEROS 12 to power up
}

static float readPAR() {
    long suma = 0;
    for (int i = 0; i < PAR_SAMPLES; i++) {
        suma += analogReadMilliVolts(PAR_PIN);
        delayMicroseconds(200);
    }
    float amplificado_mV = suma / (float)PAR_SAMPLES;
    parRawMV             = amplificado_mV;
    float sensor_mV      = amplificado_mV / PAR_AD620_GAIN;
    parSensorUV          = sensor_mV * 1000.0f;
    float par            = (sensor_mV - PAR_ZERO_OFFSET_MV) * PAR_SENSITIVITY;
    return par < 0 ? 0 : par;
}

// Discard any bytes sitting in the SDI-12 receive buffer.
// Called before every sendCommand to prevent stale responses from a previous
// cycle from being read as the current response.
static void flushSDI12() {
    while (sdi12.available()) sdi12.read();
}

// Parse SDI-12 response: "0+VWC+TEMP+EC"
// Signs (+/-) act as both delimiters and value signs.
static bool parseTEROS12(const String& response, float& vwc, float& temp, float& ec) {
    if (response.length() < 5) return false;

    float values[3] = {0, 0, 0};
    int count = 0;
    int i = 1; // skip address character

    while (i < (int)response.length() && count < 3) {
        if (response[i] == '+' || response[i] == '-') {
            int start = i++;
            while (i < (int)response.length() &&
                   response[i] != '+' && response[i] != '-' &&
                   response[i] != '\r' && response[i] != '\n') {
                i++;
            }
            values[count++] = response.substring(start, i).toFloat();
        } else {
            i++;
        }
    }

    if (count < 3) return false;

    vwc  = values[0] / 100.0f; // raw is ε×100, divide to get VWC%
    if (vwc < 0) vwc = 0;
    temp = values[1];
    ec   = values[2]; // µS/cm, no scaling needed
    return true;
}

void readSensors(MeteorDataPacket& data) {
    // Air sensor (DHT22)
    data.humAire  = sensorAire.readHumidity();
    data.tempAire = sensorAire.readTemperature();

    // TEROS 12 via SDI-12 — up to 3 attempts.
    // Root cause of -1 failures: stale bytes from the previous measurement cycle
    // accumulate in the buffer. When readStringUntil('\n') runs for the 0M! ACK it
    // consumes a leftover '\n' from old data, leaving the real ACK in the buffer.
    // Then the 0D0! read picks up the ACK instead of the data → parse fails.
    // Fix: flush before every sendCommand so the buffer is always empty.
    bool ok = false;
    for (int attempt = 0; attempt < 3 && !ok; attempt++) {
        if (attempt > 0) delay(200);

        // Step 1: start measurement — response is "a0003\r\n"
        flushSDI12();
        sdi12.sendCommand("0M!");
        sdi12.setTimeout(300);
        sdi12.readStringUntil('\n'); // consume ACK, discard
        flushSDI12();                // drop any trailing \r or extra bytes

        delay(600); // TEROS 12 measurement time < 500ms

        // Step 2: request data
        flushSDI12();
        sdi12.sendCommand("0D0!");
        sdi12.setTimeout(1500);
        String response = sdi12.readStringUntil('\n');
        response.trim();

        Serial.printf("[TEROS12] att%d raw: '%s'\n", attempt + 1, response.c_str());

        float vwc, tempSuelo, ec;
        if (parseTEROS12(response, vwc, tempSuelo, ec)) {
            data.vwcSuelo  = vwc;
            data.tempSuelo = tempSuelo;
            data.ecSuelo   = ec;
            ok = true;
        } else {
            Serial.printf("[TEROS12] Parse failed: '%s'\n", response.c_str());
        }
    }

    if (!ok) {
        data.vwcSuelo  = -1;
        data.tempSuelo = -1;
        data.ecSuelo   = -1;
    }

    // PAR radiometer
    data.par = readPAR();
    Serial.printf("[PAR] %.0f umol/m2s (%.1f uV)\n", data.par, parSensorUV);
}
