/**
 * RTC Module Implementation - PCF8563
 */

#include "rtc.h"
#include <Wire.h>
#include <PCF8563.h>

// RTC instance
static PCF8563_Class rtc;

// Flag to track initialization status
static bool rtcInitialized = false;

bool rtcInit() {
    // Initialize second I2C bus for RTC (separate from OLED)
    Wire1.begin(RTC_SDA, RTC_SCL);
    
    rtc.begin(Wire1);
    
    // Check if RTC is responding on Wire1
    Wire1.beginTransmission(0x51);
    if (Wire1.endTransmission() != 0) {
        Serial.println("RTC: PCF8563 not found on Wire1!");
        return false;
    }
    
    rtcInitialized = true;
    
    // If RTC lost power, set a default time
    if (!rtcIsRunning()) {
        Serial.println("RTC: Setting default time");
        rtcSetTime(2026, 1, 17, 12, 0, 0);
    }
    
    RtcTime t = rtcGetTime();
    Serial.printf("RTC: Initialized - %04d-%02d-%02d %02d:%02d:%02d\n",
                  t.year, t.month, t.day, t.hour, t.minute, t.second);
    
    return true;
}

RtcTime rtcGetTime() {
    RtcTime t;
    RTC_Date datetime = rtc.getDateTime();
    
    t.year = datetime.year;
    t.month = datetime.month;
    t.day = datetime.day;
    t.hour = datetime.hour;
    t.minute = datetime.minute;
    t.second = datetime.second;
    
    return t;
}

void rtcSetTime(uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour, uint8_t minute, uint8_t second) {
    rtc.setDateTime(year, month, day, hour, minute, second);
}

String rtcGetTimestamp() {
    RtcTime t = rtcGetTime();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             t.year, t.month, t.day, t.hour, t.minute, t.second);
    return String(buf);
}

String rtcGetTimeStr() {
    RtcTime t = rtcGetTime();
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hour, t.minute, t.second);
    return String(buf);
}

String rtcGetDateStr() {
    RtcTime t = rtcGetTime();
    char buf[11];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t.day, t.month, t.year);
    return String(buf);
}

bool rtcIsRunning() {
    if (!rtcInitialized) return false;
    
    // PCF8563 has a voltage low detector
    // We check if time seems valid (year > 2020)
    RtcTime t = rtcGetTime();
    return (t.year >= 2020 && t.year <= 2100);
}
