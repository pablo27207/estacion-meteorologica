#ifndef TX_BUTTON_H
#define TX_BUTTON_H

#include <Arduino.h>

#define BTN_PIN 0

// Button state
extern bool btnPressed;
extern unsigned long btnPressTime;
extern bool longPressHandled;

// Initialize button pin
void buttonInit();

// Handle button input - call in loop()
void handleButton();

#endif
