#include "sd_logger.h"
#include "rtc.h"

// SPI Instance for SD
static SPIClass sdSPI(HSPI);

// State Variables
bool sdConnected = false;
uint64_t sdTotalSpace = 0;
unsigned long sdWriteCount = 0;
String sdStatusMsg = "No Card";

static int sdFailCount = 0;
static const int SD_MAX_FAILS = 3;

void sdInit() {
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    if(!SD.begin(SD_CS, sdSPI)) {
        Serial.println("SD Mount Failed");
        sdStatusMsg = "Mount Fail";
        sdConnected = false;
    } else {
        Serial.println("SD Mounted");
        sdStatusMsg = "Ready";
        sdConnected = true;
        sdTotalSpace = SD.totalBytes() / (1024 * 1024);
        
        // Init CSV Headers if needed
        if(!SD.exists("/data.csv")) {
            File file = SD.open("/data.csv", FILE_WRITE);
            if(file) {
                file.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd,ecGnd,par,vBat,batPct");
                file.close();
            }
        }
    }
}

void logToSD(const MeteorDataPacket& data) {
    if(!sdConnected) {
        // Try to recover if previously connected
        if(sdFailCount > 0) sdInit();
        if(!sdConnected) return;
    }

    File file = SD.open("/data.csv", FILE_APPEND);
    if(!file) {
        sdFailCount++;
        Serial.printf("SD Write Fail (%d/%d)\n", sdFailCount, SD_MAX_FAILS);
        sdStatusMsg = "Write Err";
        if(sdFailCount >= SD_MAX_FAILS) {
            // Force re-initialization on next call
            sdConnected = false;
            sdFailCount = 0;
            Serial.println("SD: forcing re-init");
        }
        return;
    }

    String line = rtcGetTimestamp() + "," + String(data.packetId) + "," +
                  String(data.tempAire) + "," + String(data.humAire) + "," +
                  String(data.tempSuelo) + "," + String(data.vwcSuelo) + "," +
                  String(data.ecSuelo, 1) + "," +
                  String(data.par, 1) + "," +
                  String(data.vBat, 2) + "," + String(data.batPercent);

    file.println(line);
    file.close();
    sdFailCount = 0;
    sdWriteCount++;
    sdStatusMsg = "Logging...";
    Serial.println("SD Log OK");
}

