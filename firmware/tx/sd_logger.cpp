#include "sd_logger.h"

// SPI Instance for SD
static SPIClass sdSPI(HSPI);

// State Variables
bool sdConnected = false;
uint64_t sdTotalSpace = 0;
unsigned long sdWriteCount = 0;
String sdStatusMsg = "No Card";

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
                file.println("timestamp,packetId,tempAir,humAir,tempGnd,vwcGnd");
                file.close();
            }
        }
    }
}

void logToSD(const MeteorDataPacket& data) {
    if(!sdConnected) return;
    
    File file = SD.open("/data.csv", FILE_APPEND);
    if(!file) {
        Serial.println("SD Write Fail");
        sdStatusMsg = "Write Err";
        return;
    }
    
    // CSV Format: time(ms), id, tA, hA, tG, vwc
    String line = String(millis()) + "," + String(data.packetId) + "," + 
                  String(data.tempAire) + "," + String(data.humAire) + "," + 
                  String(data.tempSuelo) + "," + String(data.vwcSuelo);
                  
    file.println(line);
    file.close();
    sdWriteCount++;
    sdStatusMsg = "Logging...";
    Serial.println("SD Log OK");
}
