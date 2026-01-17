#ifndef TX_WIFI_MANAGER_H
#define TX_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>

// Web Server Instance
extern AsyncWebServer server;

// Connection State
extern bool wifiConnected;

// Initialize WiFi (returns true if connected to STA, false if AP mode)
bool wifiInit();

// Setup web server endpoints (call after wifiInit)
void webServerInit();

// Reset WiFi credentials and restart
void wifiReset();

// Check and maintain WiFi connection
void wifiLoop();

#endif
