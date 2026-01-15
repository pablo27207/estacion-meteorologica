#include "display.h"
#include "lora.h"
#include <WiFi.h>

// Display Instance
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// Screen State
int currentScreen = 0;
bool isScreenOn = true;

// UI State
UIState uiState = UI_VIEW;
int editCursor = 0;
int popupSelection = 0;

// Screen Timeout
int scrIndex = 2; // Default 15m
unsigned long lastInteraction = 0;

// Config Options
const int sfValues[] = {7, 8, 9, 10, 11, 12};
const char* sfLabels[] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
const float bwValues[] = {125.0, 250.0, 500.0};
const char* bwLabels[] = {"BW125", "BW250", "BW500"};
const unsigned long scrValues[] = {0, 60000, 900000, 1800000, 3600000};
const char* scrLabels[] = {"ON", "1m", "15m", "30m", "1h"};

// WiFi Reset Options
static const char* wifiResetOpts[] = {"CANCELAR", "BORRAR"};

void displayInit() {
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW); delay(50);
    digitalWrite(OLED_RST, HIGH); delay(50);
    display.begin();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 15, "Iniciando...");
    display.sendBuffer();
}

void wakeScreen() {
    if (!isScreenOn) {
        display.setPowerSave(0);
        isScreenOn = true;
    }
    lastInteraction = millis();
}

void checkScreenTimeout() {
    unsigned long timeoutDur = scrValues[scrIndex];
    if (isScreenOn && timeoutDur > 0 && (millis() - lastInteraction > timeoutDur)) {
        isScreenOn = false;
        display.setPowerSave(1);
    }
}

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
        drawPopup("Reset WiFi?", wifiResetOpts, 2, popupSelection);
        return;
    }
    
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "3. ESTADO WIFI");
    
    if (WiFi.status() == WL_CONNECTED) {
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
    if (uiState == UI_VIEW) display.drawStr(0, 10, "4. CONFIGURACION");
    else display.drawStr(0, 10, "4. CONFIG (EDIT)");
    
    char buf[32];
    int yStart = 24;
    int ySpace = 12;
    
    auto drawLine = [&](int idx, const char* label, const char* valStr) {
        if (uiState != UI_VIEW && editCursor == idx) display.drawStr(0, yStart + (idx*ySpace), ">");
        display.drawStr(10, yStart + (idx*ySpace), label);
        display.drawStr(50, yStart + (idx*ySpace), valStr);
    };
    
    sprintf(buf, "SF%d", currentSF); drawLine(0, "SF:", buf);
    sprintf(buf, "%.0f", currentBW); drawLine(1, "BW:", buf);
    drawLine(2, "Scr:", scrLabels[scrIndex]);
    
    if (uiState != UI_VIEW) {
        if (editCursor == 3) display.drawStr(0, yStart + (3*ySpace), "> SALIR");
        else display.drawStr(10, yStart + (3*ySpace), "SALIR");
    }
    
    if (uiState == UI_POPUP) {
        if (editCursor == 0) drawPopup("Set SF", sfLabels, 6, popupSelection);
        else if (editCursor == 1) drawPopup("Set BW", bwLabels, 3, popupSelection);
        else if (editCursor == 2) drawPopup("Set Screen", scrLabels, 5, popupSelection);
    } else if (uiState == UI_VIEW) {
        display.setFont(u8g2_font_5x7_tr);
        display.drawStr(0, 63, "Largo: Editar");
    }
}

void renderScreen() {
    if (!isScreenOn) {
        delay(100);
        return;
    }
    
    display.clearBuffer();
    switch (currentScreen) {
        case 0: drawRemoteData(); break;
        case 1: drawLoRaInfo(); break;
        case 2: drawWiFiInfo(); break;
        case 3: drawConfig(); break;
    }
    
    // RX indicator
    if (uiState != UI_POPUP && millis() - lastPacketTime < 1000 && lastPacketTime != 0) {
        display.drawDisc(124, 4, 2);
    }
    // Pending command indicator
    if (hasPendingCmd) display.drawStr(64, 4, "!");
    
    display.sendBuffer();
}
