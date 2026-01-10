#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <Preferences.h>
#include "shared_config.h"
#include "web_interface.h"

// --- Credenciales WiFi ---
// PRECAUCIÓN: Dejar vacío para configurar o usar hardcoded por ahora

// Instancias
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
SX1262 radio = new Module(RADIO_NSS, RADIO_DIO_1, RADIO_RST, RADIO_BUSY);
AsyncWebServer server(80);

// Referencia global al último paquete recibido (RX)
MeteorDataPacket lastData;
unsigned long lastPacketTime = 0;
float lastRSSI = 0;
float lastSNR = 0;
float lastFreqError = 0;
// Reliability Stats
unsigned long lastRxPacketId = 0;
uint32_t packetsReceived = 0;
uint32_t packetsLost = 0;
bool newData = false;

// Variables de Control Remoto
unsigned long txInterval = 10000; // Default 10s (Tracking only)
ConfigPacket pendingCmd;
bool hasPendingCmd = false;

// --- Variables UI & Config ---
// Configuración LoRa Mutable (RX side)
int currentSF = LORA_SF;
float currentBW = LORA_BW;

// Screen Timeout
const unsigned long scrValues[] = {0, 60000, 900000, 1800000, 3600000}; // 0 = Always On
const char* scrLabels[] = {"ON", "1m", "15m", "30m", "1h"};
int scrIndex = 2; // Default 15m
unsigned long lastInteraction = 0;
bool isScreenOn = true;

// Opciones Config
const int sfValues[] = {7, 8, 9, 10, 11, 12};
const char* sfLabels[] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};

const float bwValues[] = {125.0, 250.0, 500.0};
const char* bwLabels[] = {"BW125", "BW250", "BW500"};

// Estado UI
enum UIState { UI_VIEW, UI_EDIT, UI_POPUP };
UIState uiState = UI_VIEW;

int currentScreen = 0; // 0:Datos, 1:LoRa, 2:WiFi, 3:Config
int editCursor = 0;    // 0:SF, 1:BW, 2:Scr, 3:Exit
int popupSelection = 0;



// Flag de interrupcion
// Flag de interrupcion
volatile bool operationDone = false;
void setFlag(void) { operationDone = true; }

void setup() {
  Serial.begin(115200);
  
  // 1. Energía VExt
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW); delay(100);
  
  // 2. Display
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50);
  digitalWrite(OLED_RST, HIGH); delay(50);
  display.begin();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Iniciando...");
  display.sendBuffer();
  
  // 3. WiFi Manager con Preferences
  Preferences preferences;
  preferences.begin("wifi-config", false); // Namespace "wifi-config"
  
  String ssid = preferences.getString("ssid", ""); 
  String pass = preferences.getString("pass", "");
  
  bool connected = false;
  
  if(ssid == "") {
      Serial.println("No SSID saved. Starting AP.");
  } else {
      Serial.print("Connecting to: "); Serial.println(ssid);
      display.drawStr(0, 30, "Conectando WiFi...");
      display.drawStr(0, 45, ssid.c_str());
      display.sendBuffer();
      
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
      
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 15) { // 7.5 seg timeout
          delay(500);
          Serial.print(".");
          retry++;
      }
      if(WiFi.status() == WL_CONNECTED) connected = true;
  }

  if(connected) {
      // --- MODO CLIENTE (Dashboard) ---
      Serial.println("\nWiFi Connected!");
      display.drawStr(0, 30, "WiFi OK!");
      display.drawStr(0, 45, WiFi.localIP().toString().c_str());
      display.sendBuffer();
      
      // Servir Dashboard Principal
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
      });
      
      // Endpoints de Datos y Configuración Remota (Solo disponibles si hay conexión)
       server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"tempAire\":" + String(lastData.tempAire) + ",";
        json += "\"humAire\":" + String(lastData.humAire) + ",";
        json += "\"tempSuelo\":" + String(lastData.tempSuelo) + ",";
        json += "\"vwcSuelo\":" + String(lastData.vwcSuelo) + ",";
        json += "\"packetId\":" + String(lastData.packetId) + ",";
        // Signal
        json += "\"rssi\":" + String(lastRSSI) + ",";
        json += "\"snr\":" + String(lastSNR) + ",";
        json += "\"freqErr\":" + String(lastFreqError) + ",";
        
        float reliability = 100.0;
        uint32_t total = packetsReceived + packetsLost;
        if(total > 0) reliability = (float)packetsReceived * 100.0 / (float)total;
        json += "\"reliability\":" + String(reliability) + ",";
        
        // Sync Params
        json += "\"sf\":" + String(currentSF) + ",";
        json += "\"bw\":" + String(currentBW) + ",";
        json += "\"interval\":" + String(txInterval / 60000.0); // ms -> min
        json += "}";
        request->send(200, "application/json", json);
      });

      // REMOTE CMD ENDPOINT
      server.on("/set_remote", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("int") && request->hasParam("sf") && request->hasParam("bw")) {
            float iVal = request->getParam("int")->value().toFloat();
            int sVal = request->getParam("sf")->value().toInt();
            float bVal = request->getParam("bw")->value().toFloat();
            
            pendingCmd.magic = CMD_MAGIC;
            pendingCmd.interval = (uint32_t)(iVal * 60000.0);
            pendingCmd.sf = (uint8_t)sVal;
            pendingCmd.bw = bVal;
            
            hasPendingCmd = true;
            // Also update local params immediately to stay compatible (Optimistic)
            txInterval = pendingCmd.interval; 
            
            request->send(200, "text/plain", "Encolado. Esperando prox dato...");
        } else {
            request->send(400, "text/plain", "Faltan parametros");
        }
      });
      
  } else {
      // --- MODO AP (Configuracion) ---
      Serial.println("\nStarting AP: GIPIS-Estacion");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("GIPIS-Estacion"); // Sin password para facilitar config
      
      display.clearBuffer();
      display.setFont(u8g2_font_ncenB08_tr);
      display.drawStr(0, 15, "MODO CONFIG");
      display.drawStr(0, 30, "WiFi: GIPIS-Estacion");
      display.drawStr(0, 45, "IP: 192.168.4.1");
      display.sendBuffer();
      
      // Servir Pagina de Configuración
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", setup_wifi_html);
      });
      
      // Async Scan Handlers
      
      // 1. Start Scan
      server.on("/start_scan", HTTP_GET, [](AsyncWebServerRequest *request){
         WiFi.scanNetworks(true); // Async = true
         request->send(200, "text/plain", "OK");
      });
      
      // 2. Poll Results
      server.on("/scan_results", HTTP_GET, [](AsyncWebServerRequest *request){
         int n = WiFi.scanComplete();
         if(n == -2) {
             // Not started -> Trigger it
             WiFi.scanNetworks(true);
             request->send(200, "application/json", "{\"status\":\"running\"}");
         } else if(n == -1) {
             // Still running
             request->send(200, "application/json", "{\"status\":\"running\"}");
         } else {
             // Done (n >= 0)
             String json = "[";
             for (int i = 0; i < n; ++i) {
                 if (i) json += ",";
                 json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
             }
             json += "]";
             WiFi.scanDelete(); // Clean up
             request->send(200, "application/json", json);
         }
      });
      
      // Save Endpoint
      server.on("/save_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
         if(request->hasParam("ssid") && request->hasParam("pass")) {
             Preferences p; 
             p.begin("wifi-config", false);
             p.putString("ssid", request->getParam("ssid")->value());
             p.putString("pass", request->getParam("pass")->value());
             p.end();
             request->send(200, "text/plain", "Saved. Restarting...");
             delay(1000);
             ESP.restart();
         } else {
             request->send(400, "text/plain", "Missing ssid or pass");
         }
      });
  }
  
  display.sendBuffer();
  
  // 4. LoRa Init (Siempre iniciamos LoRa, incluso en modo AP)
  SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
  int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_SYNC_WORD, LORA_POWER, LORA_PREAMBLE_LEN, LORA_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
      Serial.print("LoRa Failed: "); Serial.println(state);
      display.drawStr(0, 60, "LoRa Init Failed!");
      display.sendBuffer();
      // No detener loop infinito, permitir modo AP para debug
  } else {
    radio.setDio2AsRfSwitch(true); 
    radio.setDio1Action(setFlag);
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("Listening for packets..."));
    } else {
        Serial.print(F("failed, code ")); Serial.println(state);
    }
  }
  
  server.begin();
}



// Botón
#define BTN_PIN 0
bool btnPressed = false;
unsigned long btnPressTime = 0;
bool longPressHandled = false;

// Timer Display
unsigned long lastDisplayUpdate = 0;

void applyLoRaConfig() {
  // En RX, debemos parar, configurar y volver a escuchar
  Serial.print("Reconfig LoRa... ");
  // radio.standby() es implicito en calls de config, pero bueno
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(currentBW);
  
  // Reiniciar recepción
  radio.startReceive();
  Serial.printf("Done: SF%d BW%.0f\n", currentSF, currentBW);
}

void wakeScreen() {
  if (!isScreenOn) {
    display.setPowerSave(0); // Wake
    isScreenOn = true;
  }
  lastInteraction = millis();
}

void handleButton() {
  bool currState = digitalRead(BTN_PIN);
  
  if (currState == LOW) { // Press
    if (!btnPressed) {
      btnPressed = true;
      btnPressTime = millis();
      longPressHandled = false;
      
      if (!isScreenOn) {
        wakeScreen();
        longPressHandled = true; // Consume event
      } else {
        lastInteraction = millis();
      }
    } else {
      // Long Press
      if (!longPressHandled && (millis() - btnPressTime > 800)) {
        longPressHandled = true;
        
        if (uiState == UI_VIEW) {
           if (currentScreen == 3) { // Config Screen
             uiState = UI_EDIT;
             editCursor = 0;
           } else if (currentScreen == 2) { // WiFi Screen -> Popup
             uiState = UI_POPUP;
             popupSelection = 0; // 0=Cancel, 1=Confirm
           }
        } else if (uiState == UI_EDIT) {
           if (editCursor == 3) { // Exit row
             uiState = UI_VIEW;
           } else {
             uiState = UI_POPUP;
             popupSelection = 0;
           }
        } else if (uiState == UI_POPUP) {
           // Confirm Action
           if (currentScreen == 2) { // WiFi Reset Confirm
               if (popupSelection == 1) { // BORRAR
                   display.clearBuffer();
                   display.setFont(u8g2_font_ncenB14_tr);
                   display.drawStr(0, 40, "RESET WIFI!");
                   display.sendBuffer();
                   
                   Preferences p;
                   p.begin("wifi-config", false);
                   p.clear(); 
                   p.end();
                   
                   delay(2000);
                   ESP.restart();
               } else { // CANCELAR
                   uiState = UI_VIEW;
               }
           } else if (currentScreen == 3) { // Config Confirm
               if (editCursor == 0) { // SF
                  currentSF = sfValues[popupSelection];
                  applyLoRaConfig();
               } else if (editCursor == 1) { // BW
                  currentBW = bwValues[popupSelection];
                  applyLoRaConfig();
               } else if (editCursor == 2) { // Screen
                  scrIndex = popupSelection;
               }
               uiState = UI_EDIT;
           }
        }
      }
    }
  } else { // Release
    if (btnPressed) {
      if (!longPressHandled) {
        lastInteraction = millis();
        // Short Press
        if (uiState == UI_VIEW) {
           currentScreen++;
           if (currentScreen > 3) currentScreen = 0;
        } else if (uiState == UI_EDIT) {
           editCursor++;
           if (editCursor > 3) editCursor = 0;
        } else if (uiState == UI_POPUP) {
           popupSelection++;
           
           int maxOpts = 0;
           if (currentScreen == 2) {
               maxOpts = 2; // Cancel, Confirm
           } else { // Config Screen
               if (editCursor == 0) maxOpts = 6; // SF
               else if (editCursor == 1) maxOpts = 3; // BW
               else if (editCursor == 2) maxOpts = 5; // Scr
           }
           
           if (popupSelection >= maxOpts) popupSelection = 0;
        }
      }
      btnPressed = false;
    }
  }
}

// --- DRAW FUNCTIONS ---

void drawRemoteData() {
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "1. DATOS REMOTOS");
  
  char buf[32];
  if (lastData.packetId == 0 && !newData) {
    display.drawStr(0, 30, "Esperando datos...");
    return;
  }
  
  sprintf(buf, "T.Air: %.1f C", lastData.tempAire); display.drawStr(0, 24, buf);
  sprintf(buf, "Hum: %.1f %%", lastData.humAire);   display.drawStr(0, 36, buf);
  sprintf(buf, "VWC: %.1f %%", lastData.vwcSuelo);  display.drawStr(0, 48, buf);
  sprintf(buf, "T.Gnd: %.1f C", lastData.tempSuelo);display.drawStr(0, 60, buf);
}

void drawLoRaInfo() {
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "2. ESTADO LORA");
  char buf[32];
  
  // Freq removed as requested
  
  if (radio.getRSSI() == 0) {
     display.drawStr(0, 30, "No Signal");
  } else {
     sprintf(buf, "RSSI: %.1f dBm", radio.getRSSI());
     display.drawStr(0, 30, buf);
     
     sprintf(buf, "SNR: %.1f dB", radio.getSNR());
     display.drawStr(0, 42, buf);
     
     unsigned long secondsAgo = (millis() - lastPacketTime) / 1000;
     if (lastPacketTime == 0) secondsAgo = 0;
     sprintf(buf, "Last: %lu s ago", secondsAgo);
     display.drawStr(0, 54, buf);
  }
}

const char* wifiResetOpts[] = {"CANCELAR", "BORRAR"};

void drawPopup(const char* title, const char* opts[], int count, int sel) {
  display.setDrawColor(0);
  display.drawBox(10, 10, 108, 54);
  display.setDrawColor(1);
  display.drawFrame(10, 10, 108, 54);
  
  display.drawStr(14, 22, title);
  display.drawHLine(14, 24, 100);
  
  char buf[32];
  sprintf(buf, "> %s <", opts[sel]);
  display.setFont(u8g2_font_ncenB08_tr);
  int w = display.getStrWidth(buf);
  display.drawStr(64 - (w/2), 45, buf);
  
  display.setFont(u8g2_font_5x7_tr);
  display.drawStr(20, 58, "Largo: Confirmar");
}

void drawWiFiInfo() {
  if (uiState == UI_POPUP) {
      drawPopup("¿RESET WiFi?", wifiResetOpts, 2, popupSelection);
      return;
  }

  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, "3. ESTADO WIFI");
  
  if(WiFi.status() == WL_CONNECTED) {
      display.drawStr(0, 25, "Mode: DATA CLIENT");
      display.drawStr(0, 38, WiFi.localIP().toString().c_str());
      display.drawStr(0, 50, WiFi.SSID().c_str());
      
      long rssi = WiFi.RSSI();
      char buf[20];
      sprintf(buf, "Sig: %ld dBm", rssi);
      display.drawStr(0, 62, buf);
  } else {
      display.drawStr(0, 25, "Mode: AP HOST");
      display.drawStr(0, 38, WiFi.softAPIP().toString().c_str());
      display.drawStr(0, 50, "Red: GIPIS-Estacion");
  }
}



void drawConfig() {
  display.setFont(u8g2_font_6x10_tr);
  if(uiState == UI_VIEW) display.drawStr(0, 10, "4. CONFIGURACION");
  else display.drawStr(0, 10, "4. CONFIG (EDIT)");
  
  char buf[32];
  int yStart = 24; 
  int ySpace = 12; // Slightly more space since fewer items
  
  auto drawLine = [&](int idx, const char* label, const char* valStr) {
     if(uiState != UI_VIEW && editCursor == idx) display.drawStr(0, yStart + (idx*ySpace), ">");
     display.drawStr(10, yStart + (idx*ySpace), label);
     display.drawStr(50, yStart + (idx*ySpace), valStr);
  };
  
  // 1. SF
  sprintf(buf, "SF%d", currentSF);
  drawLine(0, "SF:", buf);
  
  // 2. BW
  sprintf(buf, "%.0f", currentBW);
  drawLine(1, "BW:", buf);
  
  // 3. Scr
  drawLine(2, "Scr:", scrLabels[scrIndex]);
  
  // 4. Exit
  if(uiState != UI_VIEW) {
      if(editCursor == 3) display.drawStr(0, yStart + (3*ySpace), "> SALIR");
      else display.drawStr(10, yStart + (3*ySpace), "SALIR");
  }
  
  // Popups
  if(uiState == UI_POPUP) {
      if(editCursor == 0) drawPopup("Set SF", sfLabels, 6, popupSelection);
      else if(editCursor == 1) drawPopup("Set BW", bwLabels, 3, popupSelection);
      else if(editCursor == 2) drawPopup("Set Screen", scrLabels, 5, popupSelection);
  } else {
      if(uiState == UI_VIEW) {
         display.setFont(u8g2_font_5x7_tr);
         display.drawStr(0, 63, "Largo: Editar");
      }
  }
}

void loop() {
  // 1. Botón
  handleButton();
  
  // 2. Timeout
  unsigned long timeoutDur = scrValues[scrIndex];
  if (isScreenOn && timeoutDur > 0 && (millis() - lastInteraction > timeoutDur)) {
     isScreenOn = false;
     display.setPowerSave(1);
  }
  
  // 3. LoRa RX
  if(operationDone) {
      operationDone = false;
      int state = radio.readData((uint8_t*)&lastData, sizeof(MeteorDataPacket));
      if (state == RADIOLIB_ERR_NONE) {

          Serial.println("RX OK");
          newData = true;
          
          // Metrics
          lastPacketTime = millis();
          lastRSSI = radio.getRSSI();
          lastSNR = radio.getSNR();
          lastFreqError = radio.getFrequencyError();
          
          // Packet Loss Calculation
          if (lastData.packetId > lastRxPacketId + 1) {
              if (lastRxPacketId != 0) { // Ignorar primer paquete tras boot
                 packetsLost += (lastData.packetId - lastRxPacketId - 1);
              }
          } else if (lastData.packetId < lastRxPacketId) {
              // TX Reset detected
              packetsReceived = 0; 
              packetsLost = 0;
          }
          lastRxPacketId = lastData.packetId;
          packetsReceived++;
          
          // Sync State from TX
          txInterval = lastData.interval;
          
          // --- LOGICA DE ENVIO REMOTO ---
          if (hasPendingCmd) {
              delay(150); // Esperar que TX abra ventana RX (Aumentado para seguridad)
              Serial.print("Sending CMD... ");
              
              // IMPORTANTE: Limpiar flag antes de transmitir tambien, por si acaso
              operationDone = false; 
              
              int txState = radio.transmit((uint8_t*)&pendingCmd, sizeof(ConfigPacket));
              
              if(txState == RADIOLIB_ERR_NONE) {
                   Serial.println("OK");
                   hasPendingCmd = false;
                   
                   // Aplicar cambios localmente para mantener sincia
                   bool reinit = false;
                   if(pendingCmd.sf != currentSF) { currentSF = pendingCmd.sf; reinit=true; }
                   if(pendingCmd.bw != currentBW) { currentBW = pendingCmd.bw; reinit=true; }
                   txInterval = pendingCmd.interval;
                   
                   if(reinit) {
                       delay(100); // Dar tiempo al TX de aplicar
                       applyLoRaConfig(); 
                   }
              } else {
                   Serial.print("Fail: "); Serial.println(txState);
              }
              // IMPORTANTE: Transmit genera interrupcion (TxDone). Debemos limpiarla 
              // o asegurarnos que el proximo loop no la lea como RxDone.
              // En este codigo, operationDone se usa para ambos.
              // Al salir de transmit, operationDone estara en true (por la ISR).
              // Debemos limpiarlo.
              operationDone = false; 
              
              // Volver a escuchar
              radio.startReceive();
          } else {
             radio.startReceive();
          }
      } else {
          Serial.print(F("RX Fail: ")); Serial.println(state);
          radio.startReceive();
      }
  }
  
  // 4. Update Display
  if(isScreenOn && (millis() - lastDisplayUpdate > 200)) {
      lastDisplayUpdate = millis();
      display.clearBuffer();
      
      switch(currentScreen) {
        case 0: drawRemoteData(); break;
        case 1: drawLoRaInfo(); break;
        case 2: drawWiFiInfo(); break;
        case 3: drawConfig(); break;
      }
      
      // Indicador de RX reciente (solo si no hay popup tapandolo)
      if (uiState != UI_POPUP && millis() - lastPacketTime < 1000 && lastPacketTime != 0) {
         display.drawDisc(124, 4, 2); 
      }
      // Indicador de comando pendiente
      if (hasPendingCmd) display.drawStr(64, 4, "!");
      
      display.sendBuffer();
  }
}
