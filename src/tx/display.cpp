#include "display.h"
#include "lora.h"
#include "sd_logger.h"

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
// Measure Interval (SD logging)
const unsigned long measureValues[] = {10000, 60000, 300000, 600000, 1800000};
const char* measureLabels[] = {"10s", "1m", "5m", "10m", "30m"};
// TX Interval (LoRa)
const unsigned long txValues[] = {10000, 60000, 600000, 1800000, 3600000, 10800000};
const char* txLabels[] = {"10s", "1m", "10m", "30m", "1h", "3h"};
// Screen Timeout
const unsigned long scrValues[] = {0, 60000, 900000, 1800000, 3600000};
const char* scrLabels[] = {"ON", "1m", "15m", "30m", "1h"};

void displayInit() {
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW); delay(50);
    digitalWrite(OLED_RST, HIGH); delay(50);
    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 15, "Iniciando TX...");
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

void drawSensors(const MeteorDataPacket& data) {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "1. SENSORES");
    char buf[32];
    sprintf(buf, "T.Air: %.1f C", data.tempAire); display.drawStr(0, 24, buf);
    sprintf(buf, "Hum: %.1f %%", data.humAire);   display.drawStr(0, 36, buf);
    sprintf(buf, "VWC: %.1f %%", data.vwcSuelo);  display.drawStr(0, 48, buf);
    sprintf(buf, "T.Gnd: %.1f C", data.tempSuelo);display.drawStr(0, 60, buf);
}

void drawStatus() {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "2. ESTADO TX");
    char buf[32];
    if (lastTxState == RADIOLIB_ERR_NONE && packetCounter > 0) display.drawStr(0, 25, "Res: OK");
    else if (packetCounter == 0) display.drawStr(0, 25, "Res: ...");
    else { sprintf(buf, "Err: %d", lastTxState); display.drawStr(0, 25, buf); }
    
    sprintf(buf, "Sent: %lu", packetCounter); display.drawStr(0, 38, buf);
    unsigned long sAgo = (millis() - timeOfLastTx) / 1000;
    if (packetCounter == 0) sAgo = 0;
    sprintf(buf, "Ago: %lu s", sAgo); display.drawStr(0, 50, buf);
    
    if (operationDone) display.drawStr(80, 25, "RX CMD!");
}

void drawSDStatus() {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "3. ESTADO SD");
    
    char buf[32];
    sprintf(buf, "Status: %s", sdStatusMsg.c_str());
    display.drawStr(0, 25, buf);
    
    if (sdConnected) {
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
    if (uiState == UI_VIEW) display.drawStr(0, 10, "4. CONFIGURACION");
    else display.drawStr(0, 10, "4. CONFIG (EDIT)");
    
    char buf[32]; int yStart = 22; int ySpace = 8;
    auto drawLine = [&](int idx, const char* label, const char* valStr) {
        if (uiState != UI_VIEW && editCursor == idx) display.drawStr(0, yStart + (idx*ySpace), ">");
        display.drawStr(8, yStart + (idx*ySpace), label); 
        display.drawStr(40, yStart + (idx*ySpace), valStr);
    };
    
    sprintf(buf, "SF%d", currentSF); drawLine(0, "SF:", buf);
    sprintf(buf, "%.0f", currentBW); drawLine(1, "BW:", buf);
    
    // Measure Interval
    if (measureInterval >= 60000) sprintf(buf, "%lum", measureInterval/60000);
    else sprintf(buf, "%lus", measureInterval/1000);
    drawLine(2, "Med:", buf);
    
    // TX Interval
    if (txInterval >= 3600000) sprintf(buf, "%luh", txInterval/3600000);
    else if (txInterval >= 60000) sprintf(buf, "%lum", txInterval/60000);
    else sprintf(buf, "%lus", txInterval/1000);
    drawLine(3, "TX:", buf);
    
    drawLine(4, "Scr:", scrLabels[scrIndex]);
    
    if (uiState != UI_VIEW) { 
        if (editCursor == 5) display.drawStr(0, yStart + (5*ySpace), "> SALIR"); 
        else display.drawStr(8, yStart + (5*ySpace), "SALIR"); 
    }
    
    if (uiState == UI_POPUP) {
        if (editCursor == 0) drawPopup("Set SF", sfLabels, 6, popupSelection);
        else if (editCursor == 1) drawPopup("Set BW", bwLabels, 3, popupSelection);
        else if (editCursor == 2) drawPopup("Medicion", measureLabels, 5, popupSelection);
        else if (editCursor == 3) drawPopup("Transmision", txLabels, 6, popupSelection);
        else if (editCursor == 4) drawPopup("Pantalla", scrLabels, 5, popupSelection);
    } else if (uiState == UI_VIEW) {
        display.setFont(u8g2_font_5x7_tr); display.drawStr(0, 63, "Largo: Editar");
    }
}

void renderScreen(const MeteorDataPacket& data, unsigned long lastTxTime, unsigned long txIntervalMs) {
    if (!isScreenOn) {
        delay(100);
        return;
    }
    
    display.clearBuffer();
    switch (currentScreen) {
        case 0: drawSensors(data); break;
        case 1: drawStatus(); break;
        case 2: drawSDStatus(); break;
        case 3: drawConfig(); break;
    }
    
    if (uiState != UI_POPUP) {
        int prog = map(millis() - lastTxTime, 0, txIntervalMs, 0, 128);
        if (prog > 128) prog = 128;
        display.drawHLine(0, 63, prog);
    }
    display.sendBuffer();
}
