/**
 * Display Module for TX Node (WiFi Mode)
 * 
 * Screens:
 *   0. Sensors - Current readings
 *   1. WiFi/Server Status
 *   2. SD Card Status  
 *   3. Configuration (intervals, screen timeout)
 *   4. RTC Clock
 *   5. Battery Status
 *   6. Firmware/OTA
 */

#include "display.h"
#include "sd_logger.h"
#include "rtc.h"
#include "wifi_manager.h"
#include "server_client.h"
#include "ota.h"
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

// Intervals
unsigned long measureInterval = 60000;  // 1 min default

// Config Options
const unsigned long measureValues[] = {10000, 60000, 300000, 600000, 1800000};
const char* measureLabels[] = {"10s", "1m", "5m", "10m", "30m"};
const unsigned long txValues[] = {10000, 60000, 600000, 1800000, 3600000, 10800000};
const char* txLabels[] = {"10s", "1m", "10m", "30m", "1h", "3h"};
const unsigned long scrValues[] = {0, 60000, 900000, 1800000, 3600000};
const char* scrLabels[] = {"ON", "1m", "15m", "30m", "1h"};

// WiFi Reset Popup options
const char* wifiResetOpts[] = {"CANCELAR", "BORRAR"};

void displayInit() {
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW); delay(50);
    digitalWrite(OLED_RST, HIGH); delay(50);
    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 15, "Iniciando TX WiFi...");
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

void drawBatteryIcon(int x, int y, uint8_t percent) {
    // Battery outline (16x8 pixels)
    display.drawFrame(x, y, 14, 8);     // Body
    display.drawBox(x + 14, y + 2, 2, 4); // Tip
    
    // Fill based on percentage
    int fillWidth = (percent * 11) / 100;
    if (fillWidth > 0) {
        display.drawBox(x + 1, y + 1, fillWidth, 6);
    }
}

void drawSensors(const MeteorDataPacket& data) {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "1. SENSORES");
    
    // Battery indicator (top right)
    drawBatteryIcon(96, 2, data.batPercent);
    char batBuf[8];
    sprintf(batBuf, "%d%%", data.batPercent);
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(116 - display.getStrWidth(batBuf), 10, batBuf);
    
    display.setFont(u8g2_font_6x10_tr);
    char buf[32];
    sprintf(buf, "T.Air: %.1f C", data.tempAire); display.drawStr(0, 24, buf);
    sprintf(buf, "Hum: %.1f %%", data.humAire);   display.drawStr(0, 36, buf);
    sprintf(buf, "VWC: %.1f %%", data.vwcSuelo);  display.drawStr(0, 48, buf);
    sprintf(buf, "T.Gnd: %.1f C", data.tempSuelo);display.drawStr(0, 60, buf);
}

void drawWiFiStatus() {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "2. WIFI/SERVER");
    char buf[32];
    
    // WiFi Status
    if (WiFi.status() == WL_CONNECTED) {
        display.drawStr(0, 24, "WiFi: Conectado");
        sprintf(buf, "RSSI: %d dBm", WiFi.RSSI());
        display.drawStr(0, 34, buf);
    } else if (wifiConnected) {
        display.drawStr(0, 24, "WiFi: Reconectando...");
    } else {
        display.drawStr(0, 24, "WiFi: Modo AP");
        display.drawStr(0, 34, "192.168.4.1");
    }
    
    // Server Status
    if (serverEnabled) {
        if (lastServerOk) {
            display.drawStr(0, 48, "Server: OK");
        } else {
            display.drawStr(0, 48, "Server: Error");
        }
        int pending = getPendingCount();
        if (pending > 0) {
            sprintf(buf, "Pendientes: %d", pending);
            display.drawStr(0, 58, buf);
        }
    } else {
        display.drawStr(0, 48, "Server: Deshabilitado");
    }
    
    // Help text
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(60, 63, "Largo:Reset");
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

void drawConfig() {
    display.setFont(u8g2_font_6x10_tr);
    if (uiState == UI_VIEW) display.drawStr(0, 10, "4. CONFIGURACION");
    else display.drawStr(0, 10, "4. CONFIG (EDIT)");
    
    extern unsigned long sendInterval;
    char buf[32]; int yStart = 24; int ySpace = 10;
    
    auto drawLine = [&](int idx, const char* label, const char* valStr) {
        if (uiState != UI_VIEW && editCursor == idx) display.drawStr(0, yStart + (idx*ySpace), ">");
        display.drawStr(8, yStart + (idx*ySpace), label); 
        display.drawStr(50, yStart + (idx*ySpace), valStr);
    };
    
    // Measure Interval
    if (measureInterval >= 60000) sprintf(buf, "%lum", measureInterval/60000);
    else sprintf(buf, "%lus", measureInterval/1000);
    drawLine(0, "Med:", buf);
    
    // Send Interval
    if (sendInterval >= 3600000) sprintf(buf, "%luh", sendInterval/3600000);
    else if (sendInterval >= 60000) sprintf(buf, "%lum", sendInterval/60000);
    else sprintf(buf, "%lus", sendInterval/1000);
    drawLine(1, "Envio:", buf);
    
    // Screen timeout
    drawLine(2, "Pant:", scrLabels[scrIndex]);
    
    if (uiState != UI_VIEW) { 
        if (editCursor == 3) display.drawStr(0, yStart + (3*ySpace), "> SALIR"); 
        else display.drawStr(8, yStart + (3*ySpace), "SALIR"); 
    }
    
    if (uiState == UI_POPUP) {
        if (editCursor == 0) drawPopup("Medicion", measureLabels, 5, popupSelection);
        else if (editCursor == 1) drawPopup("Envio", txLabels, 6, popupSelection);
        else if (editCursor == 2) drawPopup("Pantalla", scrLabels, 5, popupSelection);
    } else if (uiState == UI_VIEW) {
        display.setFont(u8g2_font_5x7_tr); display.drawStr(0, 63, "Largo: Editar");
    }
}

void drawRTC() {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "5. RELOJ (RTC)");
    
    if (!rtcIsRunning()) {
        display.drawStr(0, 30, "RTC no disponible");
        display.drawStr(0, 45, "Verificar conexion");
        return;
    }
    
    RtcTime t = rtcGetTime();
    char buf[32];
    
    // Large time display
    display.setFont(u8g2_font_ncenB14_tr);
    sprintf(buf, "%02d:%02d:%02d", t.hour, t.minute, t.second);
    int w = display.getStrWidth(buf);
    display.drawStr(64 - (w/2), 35, buf);
    
    // Date below
    display.setFont(u8g2_font_6x10_tr);
    sprintf(buf, "%02d/%02d/%04d", t.day, t.month, t.year);
    w = display.getStrWidth(buf);
    display.drawStr(64 - (w/2), 52, buf);
    
    // Help text
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(0, 63, "Serial: SET_TIME");
}

void drawBattery(const MeteorDataPacket& data) {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "6. BATERIA");
    
    // Large battery icon (centered, 40x20 pixels)
    int bx = 44, by = 18;
    display.drawFrame(bx, by, 36, 16);        // Body
    display.drawBox(bx + 36, by + 4, 4, 8);   // Tip
    
    // Fill based on percentage
    int fillWidth = (data.batPercent * 32) / 100;
    if (fillWidth > 0) {
        display.drawBox(bx + 2, by + 2, fillWidth, 12);
    }
    
    // Percentage (large, centered)
    char buf[16];
    display.setFont(u8g2_font_ncenB14_tr);
    sprintf(buf, "%d%%", data.batPercent);
    int w = display.getStrWidth(buf);
    display.drawStr(64 - (w/2), 52, buf);
    
    // Voltage (left side)
    display.setFont(u8g2_font_6x10_tr);
    sprintf(buf, "%.2fV", data.vBat);
    display.drawStr(0, 52, buf);
    
    // Status text (right side)
    const char* status;
    if (data.batPercent >= 80) status = "Excelente";
    else if (data.batPercent >= 50) status = "Buena";
    else if (data.batPercent >= 20) status = "Baja";
    else status = "CRITICA!";
    
    w = display.getStrWidth(status);
    display.drawStr(128 - w, 52, status);
    
    // Range info
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(0, 63, "Min:3.0V");
    display.drawStr(85, 63, "Max:4.2V");
}

void drawPopup(const char* title, const char* opts[], int count, int sel) {
    display.setDrawColor(0); display.drawBox(10, 10, 108, 54);
    display.setDrawColor(1); display.drawFrame(10, 10, 108, 54);
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(14, 22, title); display.drawHLine(14, 24, 100);
    char buf[32]; sprintf(buf, "> %s <", opts[sel]);
    display.setFont(u8g2_font_ncenB08_tr);
    int w = display.getStrWidth(buf); display.drawStr(64 - (w/2), 45, buf);
    display.setFont(u8g2_font_5x7_tr); display.drawStr(20, 58, "Largo: Confirmar");
}

void drawFirmware() {
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 10, "7. FIRMWARE");
    
    char buf[32];
    
    // Current version
    display.setFont(u8g2_font_ncenB10_tr);
    sprintf(buf, "v%s", FW_VERSION);
    int w = display.getStrWidth(buf);
    display.drawStr(64 - (w/2), 28, buf);
    
    display.setFont(u8g2_font_6x10_tr);
    
    // Update status
    if (otaInProgress) {
        sprintf(buf, "Actualizando: %d%%", otaProgress);
        display.drawStr(0, 42, buf);
        // Progress bar
        display.drawFrame(0, 46, 128, 8);
        display.drawBox(1, 47, (otaProgress * 126) / 100, 6);
    } else if (otaUpdateAvailable) {
        sprintf(buf, "Disponible: v%s", otaNewVersion.c_str());
        display.drawStr(0, 42, buf);
    } else {
        display.drawStr(0, 42, "Estado: Actualizado");
    }
    
    // Last check
    sprintf(buf, "Ult.verif: %s", otaGetLastCheckStr().c_str());
    display.drawStr(0, 54, buf);
    
    // Help text
    display.setFont(u8g2_font_5x7_tr);
    if (otaUpdateAvailable && !otaInProgress) {
        display.drawStr(0, 63, "Largo: Actualizar");
    } else if (!otaInProgress) {
        display.drawStr(0, 63, "Largo: Buscar");
    }
}

void renderScreen(const MeteorDataPacket& data, unsigned long lastSendTime, unsigned long sendIntervalMs) {
    if (!isScreenOn) {
        delay(100);
        return;
    }
    
    display.clearBuffer();
    switch (currentScreen) {
        case 0: drawSensors(data); break;
        case 1: 
            drawWiFiStatus(); 
            // Show WiFi reset popup if active
            if (uiState == UI_POPUP) {
                drawPopup("Reset WiFi?", wifiResetOpts, 2, popupSelection);
            }
            break;
        case 2: drawSDStatus(); break;
        case 3: drawConfig(); break;
        case 4: drawRTC(); break;
        case 5: drawBattery(data); break;
        case 6: drawFirmware(); break;
    }
    
    // Progress bar (time until next send)
    if (uiState != UI_POPUP && currentScreen != 4) {
        int prog = map(millis() - lastSendTime, 0, sendIntervalMs, 0, 128);
        if (prog > 128) prog = 128;
        display.drawHLine(0, 63, prog);
    }
    display.sendBuffer();
}
