#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "shared_config.h"

// --- SD Card Pins ---
#define SD_SCK  48
#define SD_MISO 33
#define SD_MOSI 47
#define SD_CS   26

SPIClass sdSPI(HSPI); // Secondary SPI for SD

// Pin del sensor DS18B20 (temperatura de suelo)
#define ONE_WIRE_BUS 7

// Pin del sensor DHT22 (temperatura y humedad del aire)
#define DHT_PIN 6
#define DHT_TYPE DHT22

// Pin del sensor EC-5 (humedad de suelo) - Analógico
#define EC5_PIN 5

// Constantes VWC
const float VWC_SLOPE = 0.00119;
const float VWC_OFFSET = 0.400;

// Instancias
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuelo(&oneWire);
DHT sensorAire(DHT_PIN, DHT_TYPE);

// Instancia LoRa (SX1262)
SX1262 radio = new Module(RADIO_NSS, RADIO_DIO_1, RADIO_RST, RADIO_BUSY);

// Variables de datos
MeteorDataPacket currentData;
unsigned long packetCounter = 0;

// SD State
bool sdConnected = false;
uint64_t sdTotalSpace = 0;
uint64_t sdUsedSpace = 0;
unsigned long sdWriteCount = 0;
String sdStatusMsg = "No Card";

// Flag de Interrupcion
volatile bool operationDone = false;
void setFlag(void) { operationDone = true; }

void setup() {
  Serial.begin(115200);
  
  // 1. Energía VExt
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW); // ON
  
  // LED OFF
  pinMode(35, OUTPUT);
  digitalWrite(35, LOW);
  
  delay(100);
  
  // 2. Display Init
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50);
  digitalWrite(OLED_RST, HIGH); delay(50);
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Iniciando TX...");
  display.sendBuffer();
  
  // 3. Sensores Init
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  sensorSuelo.begin();
  sensorAire.begin();
  
  sensorAire.begin();

  // 4. SD Init
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if(!SD.begin(SD_CS, sdSPI)) {
      Serial.println("SD Mount Failed");
      sdStatusMsg = "Mount Fail";
      sdConnected = false;
  } else {
      Serial.println("SD Mounted");
      sdStatusMsg = "Ready";
      sdConnected = true;
      sdTotalSpace = SD.totalBytes() / (1024 * 1024);
      sdUsedSpace = SD.usedBytes() / (1024 * 1024);
      
      // Init Headers if needed
      if(!SD.exists("/data.csv")) {
          File file = SD.open("/data.csv", FILE_WRITE);
          if(file){
              file.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd");
              file.close();
          }
      }
  }
  
  // 5. LoRa Init
  Serial.print(F("[SX1262] Initializing ... "));
  SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
  
  int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_SYNC_WORD, LORA_POWER, LORA_PREAMBLE_LEN, LORA_TCXO_VOLTAGE);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    radio.setDio2AsRfSwitch(true);
    radio.setDio1Action(setFlag); // Configurar ISR
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    display.drawStr(0, 30, "LoRa Error!");
    display.sendBuffer();
    while (true);
  }
}

// Variables de Gestión de Tiempo y UI
unsigned long lastTxTime = 0;
unsigned long txInterval = 10000; // Por defecto 10s

// Configuración LoRa Mutable
int currentSF = LORA_SF;
float currentBW = LORA_BW;

// --- MAQUINA DE ESTADOS UI ---
enum UIState { UI_VIEW, UI_EDIT, UI_POPUP };
UIState uiState = UI_VIEW;

int currentScreen = 0; // 0: Sensores, 1: Estado TX, 2: Config
int editCursor = 0;    // 0: SF, 1: BW, 2: Int, 3: Scr, 4: Exit
int popupSelection = 0;

// Opciones de Configuración
const int sfValues[] = {7, 8, 9, 10, 11, 12};
const char* sfLabels[] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};

const float bwValues[] = {125.0, 250.0, 500.0};
const char* bwLabels[] = {"BW125", "BW250", "BW500"};

const unsigned long timerValues[] = {5000, 10000, 30000, 60000, 120000};
const char* timerLabels[] = {"5s", "10s", "30s", "1m", "2m"};

// Screen Timeout
const unsigned long scrValues[] = {0, 60000, 900000, 1800000, 3600000}; // 0 = Always On
const char* scrLabels[] = {"ON", "1m", "15m", "30m", "1h"};
int scrIndex = 2; // Default 15m
unsigned long lastInteraction = 0;
bool isScreenOn = true;

// Botón (GPIO 0)
#define BTN_PIN 0
bool btnPressed = false;
unsigned long btnPressTime = 0;
bool longPressHandled = false;

// Estado transmisión
int lastTxState = 0;
unsigned long timeOfLastTx = 0;

// --- SD LOGGING ---
void logToSD() {
    if(!sdConnected) return;
    
    File file = SD.open("/data.csv", FILE_APPEND);
    if(!file) {
        Serial.println("SD Write Fail");
        sdStatusMsg = "Write Err";
        return;
    }
    
    // CSV Format: time(ms), id, tA, hA, tG, vwc
    String line = String(millis()) + "," + String(currentData.packetId) + "," + 
                  String(currentData.tempAire) + "," + String(currentData.humAire) + "," + 
                  String(currentData.tempSuelo) + "," + String(currentData.vwcSuelo);
                  
    file.println(line);
    file.close();
    sdWriteCount++;
    sdStatusMsg = "Logging...";
    Serial.println("SD Log OK");
}

// --- FUNCIONES AUXILIARES ---

void applyLoRaConfig() {
  radio.standby();
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(currentBW);
  Serial.printf("Applied: SF%d BW%.0f Int%lu\n", currentSF, currentBW, txInterval);
}

void wakeScreen() {
  if (!isScreenOn) {
    display.setPowerSave(0);
    isScreenOn = true;
  }
  lastInteraction = millis();
}

void handleButton() {
  bool currState = digitalRead(BTN_PIN);
  
  if (currState == LOW) {
    if (!btnPressed) {
      btnPressed = true;
      btnPressTime = millis();
      longPressHandled = false;
      
      if (!isScreenOn) {
        wakeScreen();
        longPressHandled = true;
      } else {
        lastInteraction = millis();
      }
    } else {
      if (!longPressHandled && (millis() - btnPressTime > 800)) {
        longPressHandled = true;
        
        if (uiState == UI_VIEW) {
           if (currentScreen == 3) {
             uiState = UI_EDIT;
             editCursor = 0;
           }
        } else if (uiState == UI_EDIT) {
           if (editCursor == 4) {
             uiState = UI_VIEW;
           } else {
             uiState = UI_POPUP;
             popupSelection = 0;
           }
        } else if (uiState == UI_POPUP) {
           if (editCursor == 0) {
              currentSF = sfValues[popupSelection];
              applyLoRaConfig();
           } else if (editCursor == 1) {
              currentBW = bwValues[popupSelection];
              applyLoRaConfig();
           } else if (editCursor == 2) {
              txInterval = timerValues[popupSelection];
           } else if (editCursor == 3) {
              scrIndex = popupSelection;
           }
           uiState = UI_EDIT;
        }
      }
    }
  } else {
    if (btnPressed) {
      if (!longPressHandled) {
        lastInteraction = millis();
        if (uiState == UI_VIEW) {
           currentScreen++;
           if (currentScreen > 3) currentScreen = 0;
        } else if (uiState == UI_EDIT) {
           editCursor++;
           if (editCursor > 4) editCursor = 0;
        } else if (uiState == UI_POPUP) {
           popupSelection++;
           int maxOpts = 0;
           if (editCursor == 0) maxOpts = 6;
           else if (editCursor == 1) maxOpts = 3;
           else if (editCursor == 2) maxOpts = 5;
           else if (editCursor == 3) maxOpts = 5;
           if (popupSelection >= maxOpts) popupSelection = 0;
        }
      }
      btnPressed = false;
    }
  }
}

// --- DIBUJADO DE PANTALLAS ---
void drawSensors() {
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "1. SENSORES");
  char buf[32];
  sprintf(buf, "T.Air: %.1f C", currentData.tempAire); display.drawStr(0, 24, buf);
  sprintf(buf, "Hum: %.1f %%", currentData.humAire);   display.drawStr(0, 36, buf);
  sprintf(buf, "VWC: %.1f %%", currentData.vwcSuelo);  display.drawStr(0, 48, buf);
  sprintf(buf, "T.Gnd: %.1f C", currentData.tempSuelo);display.drawStr(0, 60, buf);
}

void drawStatus() {
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "2. ESTADO TX");
  char buf[32];
  if (lastTxState == RADIOLIB_ERR_NONE && packetCounter > 0) display.drawStr(0, 25, "Res: OK");
  else if (packetCounter==0) display.drawStr(0, 25, "Res: ...");
  else { sprintf(buf, "Err: %d", lastTxState); display.drawStr(0, 25, buf); }
  
  sprintf(buf, "Sent: %lu", packetCounter); display.drawStr(0, 38, buf);
  unsigned long sAgo = (millis()-timeOfLastTx)/1000;
  if(packetCounter==0) sAgo=0;
  sprintf(buf, "Ago: %lu s", sAgo); display.drawStr(0, 50, buf);
  
  if (operationDone) display.drawStr(80, 25, "RX CMD!");
}

void drawSDStatus() {
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "3. ESTADO SD");
  
  char buf[32];
  sprintf(buf, "Status: %s", sdStatusMsg.c_str());
  display.drawStr(0, 25, buf);
  
  if(sdConnected) {
      sprintf(buf, "Size: %llu MB", sdTotalSpace);
      display.drawStr(0, 38, buf);
      
      sprintf(buf, "Writes: %lu", sdWriteCount);
      display.drawStr(0, 50, buf);
  } else {
      display.drawStr(0, 38, "Check Cable");
  }
}

void drawPopup(const char* title, const char* opts[], int count, int sel) {
  display.setDrawColor(0); display.drawBox(10, 10, 108, 54);
  display.setDrawColor(1); display.drawFrame(10, 10, 108, 54);
  display.drawStr(14, 22, title); display.drawHLine(14, 24, 100);
  char buf[32]; sprintf(buf, "> %s <", opts[sel]);
  display.setFont(u8g2_font_ncenB08_tr);
  int w = display.getStrWidth(buf); display.drawStr(64 - (w/2), 45, buf);
  display.setFont(u8g2_font_5x7_tr); display.drawStr(20, 58, "Largo: Confirmar");
}

void drawConfig() {
  display.setFont(u8g2_font_6x10_tr);
  if(uiState == UI_VIEW) display.drawStr(0, 10, "3. CONFIGURACION");
  else display.drawStr(0, 10, "3. CONFIG (EDIT)");
  char buf[32]; int yStart = 24; int ySpace = 10;
  auto drawLine = [&](int idx, const char* label, const char* valStr) {
     if(uiState != UI_VIEW && editCursor == idx) display.drawStr(0, yStart + (idx*ySpace), ">");
     display.drawStr(10, yStart + (idx*ySpace), label); display.drawStr(50, yStart + (idx*ySpace), valStr);
  };
  sprintf(buf, "SF%d", currentSF); drawLine(0, "SF:", buf);
  sprintf(buf, "%.0f", currentBW); drawLine(1, "BW:", buf);
  if(txInterval >= 60000) sprintf(buf, "%lu m", txInterval/60000); else sprintf(buf, "%lu s", txInterval/1000); drawLine(2, "Int:", buf);
  drawLine(3, "Scr:", scrLabels[scrIndex]);
  if(uiState != UI_VIEW) { if(editCursor == 4) display.drawStr(0, yStart + (4*ySpace), "> SALIR"); else display.drawStr(10, yStart + (4*ySpace), "SALIR"); }
  
  if (uiState == UI_POPUP) {
    if(editCursor == 0) drawPopup("Set SF", sfLabels, 6, popupSelection);
    else if(editCursor == 1) drawPopup("Set BW", bwLabels, 3, popupSelection);
    else if(editCursor == 2) drawPopup("Set Time", timerLabels, 5, popupSelection);
    else if(editCursor == 3) drawPopup("Set Screen", scrLabels, 5, popupSelection);
  } else if(uiState == UI_VIEW) {
    display.setFont(u8g2_font_5x7_tr); display.drawStr(0, 63, "Largo: Editar");
  }
}

void loop() {
  handleButton();
  
  unsigned long timeoutDur = scrValues[scrIndex];
  if (isScreenOn && timeoutDur > 0 && (millis() - lastInteraction > timeoutDur)) {
     isScreenOn = false; display.setPowerSave(1);
  }
  
  if (millis() - lastTxTime > txInterval || lastTxTime == 0) {
    // 1. Lectura
    currentData.humAire = sensorAire.readHumidity();
    currentData.tempAire = sensorAire.readTemperature();
    sensorSuelo.requestTemperatures();
    currentData.tempSuelo = sensorSuelo.getTempCByIndex(0);
    currentData.rawSuelo = analogRead(EC5_PIN);
    float voltageMV = (currentData.rawSuelo / 4095.0) * 3300.0;
    float vwc = (VWC_SLOPE * voltageMV) - VWC_OFFSET;
    currentData.vwcSuelo = vwc * 100.0;
    if(currentData.vwcSuelo < 0) currentData.vwcSuelo = 0.0;
    currentData.packetId = packetCounter;
    currentData.interval = txInterval; // Sync state
    
    // 2. Transmision
    // Habilitar TX
    int state = radio.transmit((uint8_t*)&currentData, sizeof(MeteorDataPacket));
    lastTxState = state;
    timeOfLastTx = millis();
    lastTxTime = millis();
    
    if (state == RADIOLIB_ERR_NONE) {
       packetCounter++;
       
       // 3. Log & Listen
       logToSD();
       radio.startReceive();
       unsigned long rxStart = millis();
       bool cmdReceived = false;
       
       while(millis() - rxStart < 2000) {
           handleButton(); // Mantener UI viva
           if(operationDone) {
               operationDone = false;
               ConfigPacket packet;
               int rxState = radio.readData((uint8_t*)&packet, sizeof(ConfigPacket));
               
                if(rxState == RADIOLIB_ERR_NONE && packet.magic == CMD_MAGIC) {
                   Serial.println(" CMD Received!");
                   cmdReceived = true;
                   
                   // Feedback Visual Inmediato
                   display.clearBuffer();
                   display.setFont(u8g2_font_ncenB14_tr);
                   display.drawStr(10, 40, "CMD RECIBIDO!");
                   display.sendBuffer();
                   delay(1000); // Mostrar mensaje por 1s
                   
                   // Aplicar Configuración (Solo si cambio)
                   bool loraChanged = false;
                   if (packet.sf != currentSF) { currentSF = packet.sf; loraChanged = true; }
                   if (packet.bw != currentBW) { currentBW = packet.bw; loraChanged = true; }
                   
                   txInterval = packet.interval;
                   Serial.printf("New Config: SF%d BW%.0f Int%lu\n", currentSF, currentBW, txInterval);
                   
                   if (loraChanged) applyLoRaConfig();
                   
                   break; // Salir del loop si recibimos comando
               }
           }
           delay(10);
       }
       radio.sleep(); // Dormir radio hasta la proxima TX
       
    } else {
       Serial.println("TX Fail");
       radio.sleep();
    }
  }
  
  // Dibujar UI
  if (isScreenOn) {
    display.clearBuffer();
    switch(currentScreen) {
      case 0: drawSensors(); break;
      case 1: drawStatus(); break;
      case 2: drawSDStatus(); break;
      case 3: drawConfig(); break;
    }
    if(uiState != UI_POPUP) {
      int prog = map(millis() - lastTxTime, 0, txInterval, 0, 128);
      if(prog > 128) prog = 128;
      display.drawHLine(0, 63, prog);
    }
    display.sendBuffer();
  } else {
     delay(100); 
  }
}
