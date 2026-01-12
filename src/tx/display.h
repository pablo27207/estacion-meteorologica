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

// Config options (arrays)
extern const int sfValues[];
extern const char* sfLabels[];
extern const float bwValues[];
extern const char* bwLabels[];
extern const unsigned long timerValues[];
extern const char* timerLabels[];
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
void drawStatus();
void drawSDStatus();
void drawConfig();
void drawPopup(const char* title, const char* opts[], int count, int sel);

// Full screen render cycle
void renderScreen(const MeteorDataPacket& data, unsigned long lastTxTime, unsigned long txInterval);

#endif
