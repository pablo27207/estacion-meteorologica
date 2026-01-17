/**
 * Button Handler for TX Node (WiFi Mode)
 * 
 * Short press: Navigate screens
 * Long press on WiFi screen: Reset WiFi credentials
 * Long press on Config screen: Enter edit mode
 */

#include "button.h"
#include "display.h"
#include "wifi_manager.h"

// Button State
bool btnPressed = false;
unsigned long btnPressTime = 0;
bool longPressHandled = false;

void buttonInit() {
    pinMode(BTN_PIN, INPUT_PULLUP);
}

void handleButton() {
    bool currState = digitalRead(BTN_PIN);
    
    if (currState == LOW) { // Pressed
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
            // Long Press Detection (800ms)
            if (!longPressHandled && (millis() - btnPressTime > 800)) {
                longPressHandled = true;
                
                if (uiState == UI_VIEW) {
                    if (currentScreen == 1) { 
                        // WiFi Status Screen -> Show reset popup
                        uiState = UI_POPUP;
                        popupSelection = 0;
                    } else if (currentScreen == 3) { 
                        // Config Screen -> Enter edit mode
                        uiState = UI_EDIT;
                        editCursor = 0;
                    }
                } else if (uiState == UI_EDIT) {
                    if (editCursor == 3) { 
                        // Exit row
                        uiState = UI_VIEW;
                    } else {
                        uiState = UI_POPUP;
                        popupSelection = 0;
                    }
                } else if (uiState == UI_POPUP) {
                    // Confirm action
                    if (currentScreen == 1) { 
                        // WiFi Reset confirmation
                        if (popupSelection == 1) { 
                            // BORRAR selected
                            wifiReset();
                        } else { 
                            // CANCELAR
                            uiState = UI_VIEW;
                        }
                    } else if (currentScreen == 3) { 
                        // Config confirmation
                        if (editCursor == 0) {
                            measureInterval = measureValues[popupSelection];
                        } else if (editCursor == 1) {
                            // sendInterval will be handled separately
                            extern unsigned long sendInterval;
                            sendInterval = txValues[popupSelection];
                        } else if (editCursor == 2) {
                            scrIndex = popupSelection;
                        }
                        uiState = UI_EDIT;
                    }
                }
            }
            
            // Very Long Press Detection (5 seconds) - Force WiFi Reset
            if (!longPressHandled && (millis() - btnPressTime > 5000)) {
                longPressHandled = true;
                wifiReset();
            }
        }
    } else { // Released
        if (btnPressed) {
            if (!longPressHandled) {
                lastInteraction = millis();
                // Short Press
                if (uiState == UI_VIEW) {
                    currentScreen++;
                    if (currentScreen > 4) currentScreen = 0;
                } else if (uiState == UI_EDIT) {
                    editCursor++;
                    if (editCursor > 3) editCursor = 0;
                } else if (uiState == UI_POPUP) {
                    popupSelection++;
                    int maxOpts = 0;
                    if (currentScreen == 1) {
                        maxOpts = 2; // Cancel, Confirm
                    } else if (currentScreen == 3) {
                        if (editCursor == 0) maxOpts = 5;       // Measure
                        else if (editCursor == 1) maxOpts = 6;  // Send
                        else if (editCursor == 2) maxOpts = 5;  // Screen
                    }
                    if (popupSelection >= maxOpts) popupSelection = 0;
                }
            }
            btnPressed = false;
        }
    }
}
