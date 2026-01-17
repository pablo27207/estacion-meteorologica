#ifndef TX_DISPLAY_H
#define TX_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../shared/config.h"

// Display Instance (extern for access)
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

// Screen State
extern int currentScreen;
extern bool isScreenOn;

// UI State Machine
enum UIState { UI_VIEW, UI_EDIT, UI_POPUP };
extern UIState uiState;
extern int editCursor;
extern int popupSelection;

// Screen Timeout
extern int scrIndex;
extern unsigned long lastInteraction;

// Intervals
extern unsigned long measureInterval;

// Config options (arrays)
extern const unsigned long measureValues[];
extern const char* measureLabels[];
extern const unsigned long txValues[];
extern const char* txLabels[];
extern const unsigned long scrValues[];
extern const char* scrLabels[];

// Initialize display
void displayInit();

// Wake screen from sleep
void wakeScreen();

// Check and apply screen timeout
void checkScreenTimeout();

// Draw functions
void drawSensors(const MeteorDataPacket& data);
void drawWiFiStatus();
void drawSDStatus();
void drawRTC();
void drawConfig();
void drawPopup(const char* title, const char* opts[], int count, int sel);

// Full screen render cycle
void renderScreen(const MeteorDataPacket& data, unsigned long lastSendTime, unsigned long sendInterval);

#endif
