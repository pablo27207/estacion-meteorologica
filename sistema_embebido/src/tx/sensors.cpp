#include "sensors.h"

// Sensor Instances
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensorSuelo(&oneWire);
static DHT sensorAire(DHT_PIN, DHT_TYPE);

void sensorsInit() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    sensorSuelo.begin();
    sensorAire.begin();
}

void readSensors(MeteorDataPacket& data) {
    // Air Sensor (DHT22)
    data.humAire = sensorAire.readHumidity();
    data.tempAire = sensorAire.readTemperature();
    
    // Ground Temperature (DS18B20)
    sensorSuelo.requestTemperatures();
    data.tempSuelo = sensorSuelo.getTempCByIndex(0);
    
    // Soil Moisture (EC-5)
    data.rawSuelo = analogRead(EC5_PIN);
    float voltageMV = (data.rawSuelo / 4095.0f) * 3300.0f;
    float vwc = (VWC_SLOPE * voltageMV) - VWC_OFFSET;
    data.vwcSuelo = vwc * 100.0f;
    if(data.vwcSuelo < 0) data.vwcSuelo = 0.0f;
}
