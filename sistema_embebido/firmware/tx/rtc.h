/**
 * RTC Module - PCF8563 Real Time Clock
 * 
 * Provides time/date functionality for the TX node.
 * Uses I2C bus shared with OLED display (SDA=17, SCL=18).
 * PCF8563 I2C address: 0x51
 */

#ifndef TX_RTC_H
#define TX_RTC_H

#include <Arduino.h>
#include "../shared/config.h"

// RTC I2C pins (separate bus Wire1)
// GPIO 41 = SDA, GPIO 42 = SCL (safe pins, no boot strapping)
#define RTC_SDA 41
#define RTC_SCL 42

// Time structure for easy handling
struct RtcTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Initialize RTC module
bool rtcInit();

// Get current time from RTC
RtcTime rtcGetTime();

// Set RTC time
void rtcSetTime(uint16_t year, uint8_t month, uint8_t day, 
                uint8_t hour, uint8_t minute, uint8_t second);

// Get formatted timestamp string (YYYY-MM-DD HH:MM:SS)
String rtcGetTimestamp();

// Get formatted time string (HH:MM:SS)
String rtcGetTimeStr();

// Get formatted date string (DD/MM/YYYY)
String rtcGetDateStr();

// Check if RTC is running
bool rtcIsRunning();

#endif
