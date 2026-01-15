#include "data_logger.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

#define LOG_FILE "/data.csv"
#define MAX_FILE_SIZE (2 * 1024 * 1024)  // 2MB max, then rotate

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (-3 * 3600)  // Argentina UTC-3
#define DAYLIGHT_OFFSET_SEC 0

// Stats
unsigned long logFileSize = 0;
unsigned long logEntryCount = 0;
bool ntpSynced = false;

// Get formatted timestamp
String getTimestamp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return String(millis());  // Fallback to millis
    }
    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

bool dataLoggerInit() {
    if (!LittleFS.begin(true)) {  // true = format on fail
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    
    Serial.println("LittleFS Mounted");
    
    // Configure NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    // Wait for time sync (max 5 seconds)
    struct tm timeinfo;
    for (int i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo)) {
            ntpSynced = true;
            Serial.println("NTP Time synced!");
            break;
        }
        delay(500);
    }
    
    // Check if log file exists
    if (LittleFS.exists(LOG_FILE)) {
        File f = LittleFS.open(LOG_FILE, "r");
        if (f) {
            logFileSize = f.size();
            f.close();
        }
    } else {
        // Create with headers
        File f = LittleFS.open(LOG_FILE, "w");
        if (f) {
            f.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd,rssi,snr");
            f.close();
        }
    }
    
    Serial.printf("Log file size: %lu bytes\n", logFileSize);
    return true;
}

void logReceivedData(const MeteorDataPacket& data, float rssi, float snr) {
    // Check if we need to rotate (delete old file)
    if (logFileSize > MAX_FILE_SIZE) {
        Serial.println("Log file too large, rotating...");
        LittleFS.remove(LOG_FILE);
        
        // Recreate with headers
        File f = LittleFS.open(LOG_FILE, "w");
        if (f) {
            f.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd,rssi,snr");
            f.close();
        }
        logFileSize = 0;
        logEntryCount = 0;
    }
    
    File file = LittleFS.open(LOG_FILE, "a");
    if (!file) {
        Serial.println("Failed to open log file");
        return;
    }
    
    // Write CSV line with real timestamp
    String line = getTimestamp() + "," + 
                  String(data.packetId) + "," +
                  String(data.tempAire, 2) + "," + 
                  String(data.humAire, 2) + "," +
                  String(data.tempSuelo, 2) + "," + 
                  String(data.vwcSuelo, 2) + "," +
                  String(rssi, 1) + "," + 
                  String(snr, 1);
    
    file.println(line);
    logFileSize += line.length() + 1;
    logEntryCount++;
    file.close();
    
    Serial.printf("Logged entry #%lu (file: %lu bytes)\n", logEntryCount, logFileSize);
}

unsigned long getLogFileSize() {
    if (LittleFS.exists(LOG_FILE)) {
        File f = LittleFS.open(LOG_FILE, "r");
        if (f) {
            unsigned long size = f.size();
            f.close();
            return size;
        }
    }
    return 0;
}

void clearLogFile() {
    LittleFS.remove(LOG_FILE);
    
    // Recreate with headers
    File f = LittleFS.open(LOG_FILE, "w");
    if (f) {
        f.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd,rssi,snr");
        f.close();
    }
    logFileSize = 0;
    logEntryCount = 0;
    Serial.println("Log file cleared");
}
