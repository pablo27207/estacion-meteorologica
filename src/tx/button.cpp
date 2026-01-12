#include "button.h"
#include "display.h"
#include "lora.h"

// Button State
bool btnPressed = false;
unsigned long btnPressTime = 0;
bool longPressHandled = false;

void buttonInit() {
    pinMode(BTN_PIN, INPUT_PULLUP);
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
            // Long Press Detection
            if (!longPressHandled && (millis() - btnPressTime > 800)) {
                longPressHandled = true;
                
                if (uiState == UI_VIEW) {
                    if (currentScreen == 3) { // Config Screen
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
                    // Apply Selection
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
        // Release
        if (btnPressed) {
            if (!longPressHandled) {
                lastInteraction = millis();
                // Short Press
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
