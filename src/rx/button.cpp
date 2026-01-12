#include "button.h"
#include "display.h"
#include "lora.h"
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
    
    if (currState == LOW) { // Press
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
                    } else if (currentScreen == 2) { // WiFi Screen -> Popup
                        uiState = UI_POPUP;
                        popupSelection = 0;
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
                            wifiReset();
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
