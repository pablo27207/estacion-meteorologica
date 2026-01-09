#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <Wire.h>

// Pines del display OLED en Heltec LoRa 32 V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21

// Pin de control de alimentación del OLED (Vext)
#define VEXT_CTRL 36

// Pin del sensor DS18B20 (temperatura de suelo)
#define ONE_WIRE_BUS 7

// Pin del sensor DHT22 (temperatura y humedad del aire)
#define DHT_PIN 6
#define DHT_TYPE DHT22

// Pin del sensor EC-5 (humedad de suelo) - Analógico
#define EC5_PIN 5

// Instancia del display OLED (SSD1306 128x64, I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// Instancia de OneWire y DallasTemperature para DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuelo(&oneWire);

// Instancia del sensor DHT22
DHT sensorAire(DHT_PIN, DHT_TYPE);

// Variables para almacenar las lecturas
float tempSuelo = 0.0;
float tempAire = 0.0;
float humedadAire = 0.0;
int humedadSueloRaw = 0;
float humedadSueloPct = 0.0;

// Calibración EC-5 (ajustar según tu sensor)
#define EC5_SECO 800    // Valor ADC en aire seco (~844 medido)
#define EC5_MOJADO 2500 // Valor ADC en agua/suelo saturado (ajustar después)

void setup() {
  Serial.begin(115200);
  
  // Activar alimentación del OLED (Vext)
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);
  delay(100);
  
  // Reset manual del display OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);
  digitalWrite(OLED_RST, HIGH);
  delay(50);
  
  // Inicializar el display OLED
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Iniciando...");
  display.sendBuffer();
  
  // Configurar ADC para el EC-5
  analogReadResolution(12);  // 12 bits = 0-4095
  analogSetAttenuation(ADC_11db);  // Rango completo 0-3.3V
  
  // Inicializar sensores
  sensorSuelo.begin();
  sensorAire.begin();
  
  Serial.println("Estacion meteorologica iniciada");
  Serial.print("Sensores DS18B20 encontrados: ");
  Serial.println(sensorSuelo.getDeviceCount());
  
  delay(2000);
}

void loop() {
  // Leer sensor DHT22 (aire)
  humedadAire = sensorAire.readHumidity();
  delay(100);
  tempAire = sensorAire.readTemperature();
  
  // Leer sensor DS18B20 (suelo)
  sensorSuelo.requestTemperatures();
  tempSuelo = sensorSuelo.getTempCByIndex(0);
  
  // Leer sensor EC-5 (humedad suelo)
  humedadSueloRaw = analogRead(EC5_PIN);
  // Convertir a porcentaje (0-100%)
  humedadSueloPct = map(humedadSueloRaw, EC5_SECO, EC5_MOJADO, 0, 100);
  humedadSueloPct = constrain(humedadSueloPct, 0, 100);
  
  // Imprimir en Serial
  Serial.println("--- Lecturas ---");
  Serial.printf("Temp Suelo: %.1f C\n", tempSuelo);
  Serial.printf("Temp Aire:  %.1f C\n", tempAire);
  Serial.printf("Hum Aire:   %.1f %%\n", humedadAire);
  Serial.printf("Hum Suelo:  %.1f %% (raw: %d)\n", humedadSueloPct, humedadSueloRaw);
  
  // Limpiar buffer del display
  display.clearBuffer();
  
  // Título
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 10, "ESTACION METEO");
  display.drawLine(0, 12, 128, 12);
  
  // Columna izquierda: AIRE
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 24, "-- AIRE --");
  
  char strTempAire[16];
  char strHumAire[16];
  if (!isnan(tempAire)) {
    sprintf(strTempAire, "T: %.1f C", tempAire);
  } else {
    strcpy(strTempAire, "T: Error");
  }
  if (!isnan(humedadAire)) {
    sprintf(strHumAire, "H: %.0f %%", humedadAire);
  } else {
    strcpy(strHumAire, "H: Error");
  }
  display.drawStr(0, 36, strTempAire);
  display.drawStr(0, 48, strHumAire);
  
  // Columna derecha: SUELO
  display.drawStr(70, 24, "-- SUELO --");
  
  char strTempSuelo[16];
  char strHumSuelo[16];
  if (tempSuelo != DEVICE_DISCONNECTED_C) {
    sprintf(strTempSuelo, "T: %.1f C", tempSuelo);
  } else {
    strcpy(strTempSuelo, "T: Error");
  }
  sprintf(strHumSuelo, "H: %.0f %%", humedadSueloPct);
  
  display.drawStr(70, 36, strTempSuelo);
  display.drawStr(70, 48, strHumSuelo);
  
  // Línea de estado inferior
  display.setFont(u8g2_font_5x7_tr);
  char strRaw[20];
  sprintf(strRaw, "EC5 raw: %d", humedadSueloRaw);
  display.drawStr(0, 62, strRaw);
  
  // Actualizar display
  display.sendBuffer();
  
  // Esperar 3 segundos
  delay(3000);
}