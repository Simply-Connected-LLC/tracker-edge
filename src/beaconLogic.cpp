#include "Particle.h"
#include "beaconLogic.h"
#include "BeaconScanner.h"
#include "beaconPublish.h"
#include "JsonParserGeneratorRK.h"
#include "beaconPublish.h"
#include "location_service.h"

const int numBeaconsInMemory = 100;
beaconRSSI beaconList[numBeaconsInMemory];
int beaconListLength = 0;
logicThresholds logicParams;
bool beaconInList = false;
int beaconIndex;
int memoryIndex;
setupData setupInfo;
scannerData sData;
int maxBeaconsPerPublish = 5;

String compileData(beaconRSSI beacon);
void checkAndPublish(int voltagePin, String version);
void beaconLogicLoop(int voltagePin, String version);
void initBeacon(String address, int Rssi, int index);
void evaluateBeacons(void);
void checkInSightListForBeacon(String address);
void checkScannedListForBeacon(String address);
void resetBeacon(int index);
bool publishEnabledCheck(int inputPin);
bool getEnabled(void);
void resetAllBeacons(void);
void initLocation(void);
void tryUpdateLocation(void);
//scannerData getScannerData(void);




void evaluateBeacons(void) {
    
    for (auto& beacon : Scanner.getLairdBt510()) {
        Log.info("beacon:    " + beacon.getAddress().toString() + "   with RSSI:   " + String(beacon.getRssi()));
        if (beacon.getRssi() > logicParams.RSSI_Filter) {
            checkInSightListForBeacon(beacon.getAddress().toString());
            if (beaconInList != true){
                for (int j = 0; j < numBeaconsInMemory; j++){
                    if (beaconList[j].active == false){      // find the first open slot in memory.
                        memoryIndex = j;
                        break;
                    }
                }
                if ((memoryIndex+1) > beaconListLength){ // update the length of used memory array if necessary...
                    beaconListLength = memoryIndex + 1;
                }
                Log.trace("Initializing beacon at index: " + String(memoryIndex));
                initBeacon(beacon.getAddress().toString(), beacon.getRssi(), memoryIndex);
            }
            else { // beaconInList = true
                // update max RSSI if necessary
                if (beacon.getRssi() > beaconList[beaconIndex].maxRSSI) {
                    beaconList[beaconIndex].maxRSSI = beacon.getRssi();
                }
                // update the counters
                beaconList[beaconIndex].withinCounter += beaconList[beaconIndex].missedCounter + 1;
                beaconList[beaconIndex].missedCounter = 0;
                beaconList[beaconIndex].RSSIoutCounter = 0;

                // check if beacon has been in RSSI zone long enough.
                if (beaconList[beaconIndex].withinCounter >= logicParams.withinThresh) {
                    beaconList[beaconIndex].within = true;
                    beaconList[beaconIndex].batteryVoltage = beacon.getBattVoltage();
                }
            }
        } 
        else { // beacon RSSI not greater than RSSI_Filter
            checkInSightListForBeacon(beacon.getAddress().toString());
            if (beaconInList == true){
                beaconList[beaconIndex].RSSIoutCounter ++;
                Log.trace("within = " + String(beaconList[beaconIndex].within) + " for beacon: " + beaconList[beaconIndex].beaconAddress);
                if ((beaconList[beaconIndex].within != true) && (beaconList[beaconIndex].schedulePublish != true)) {
                    if (beaconList[beaconIndex].RSSIoutCounter >= logicParams.RSSIoutThresh){
                        beaconList[beaconIndex].withinCounter = 0;
                        Log.trace("Set within Counter for beacon: " + beaconList[beaconIndex].beaconAddress + " to zero.");
                    }
                }
            }
        }
        
    } // end of for each scanned beacon in list...
    // now begin second part of evaluation.
    for (int i = 0; i <  beaconListLength; i++){
        Log.info("i: " + String(i));
        Log.info("beacon:" + beaconList[i].beaconAddress);
        Log.info("beacon active: " + String(beaconList[i].active));
        Log.info("beacon within counter: " + String(beaconList[i].withinCounter));
        Log.info("beacon RSSI out counter: " + String(beaconList[i].RSSIoutCounter));
        Log.info("beacon missed counter: " + String(beaconList[i].missedCounter));
        Log.info("beacon within: " + String(beaconList[i].within));
        Log.info("beacon schedule publish: " + String(beaconList[i].schedulePublish));
        Log.info("beacon maxRSSI: " + String(beaconList[i].maxRSSI));
        Log.info("beacon List length: " + String(beaconListLength));
        if (beaconList[i].active == true){  // make sure there is an active beacon in that slot of memory...
            checkScannedListForBeacon(beaconList[i].beaconAddress);
            if(beaconInList == false){
                beaconList[i].missedCounter++;          
            }
            Log.trace("missedCounter: " + String(beaconList[i].missedCounter));
            Log.trace("RSSI out counter: " + String(beaconList[i].RSSIoutCounter));
            Log.trace("logic outThresh =" + String(logicParams.outThresh));
            if ((beaconList[i].missedCounter+beaconList[i].RSSIoutCounter) >= logicParams.outThresh){
                    Log.trace("beacon within status: " + String(beaconList[i].within));
                    if (beaconList[i].within == true){
                        beaconList[i].within = false;
                        beaconList[i].schedulePublish = true;
                        beaconList[i].publishTime = Time.now();
                    }
                    else { // within == false - IE the beacon was never considered to be in the RSSI zone, or not for long enough.
                        // free this space in memory...
                        //beaconList[i].active = false;
                        if (beaconList[i].schedulePublish == false){
                            resetBeacon(i);
                        }
                    }
                }   
                
            
        }
        
        
    }


}

String compileData(beaconRSSI beacon){
    String temp_String = "";
    String beaconBatteryGoodString;
    Log.trace("batteryVoltage");
    Log.trace(String(beacon.batteryVoltage));
    if (beacon.batteryVoltage == true){
        beaconBatteryGoodString = "true";
    }else{
        beaconBatteryGoodString = "false";
    }
        
    temp_String+= "{\"beaconTimeStamp\":\"" + String(beacon.publishTime)+ "\",";
    temp_String += "\"beaconID\":\"" + beacon.beaconAddress + "\",";
    temp_String += "\"beaconMaxRSSI\":" + String(beacon.maxRSSI) + ",";
    temp_String += "\"beaconInsideCount\":\"" + String(beacon.withinCounter) + "\",";
    temp_String += "\"beaconBatteryGood\":\"" + beaconBatteryGoodString + "\"}";  
    Log.trace("compiledData! ");
    Log.trace(temp_String);
    return temp_String;
}


void checkAndPublish(int voltagePin){
    if (publishEnabledCheck(voltagePin) == true){
        String beaconPackets[] = {};
        int beaconsPublished = 0;
        //find out how many publishes we need...
        String beaconJSON = "\"beacon\":[";
        int beaconCounter = 0;
        Log.trace("in checkAndPublish ... beaconListLength = "  + String(beaconListLength));
    for (int q = 0; q < beaconListLength; q++){
        Log.trace("in check and publish... beacon active: " + String(beaconList[q].active));
        Log.trace("in check and publish... schedulePublish = " + String(beaconList[q].schedulePublish));
        if ((beaconList[q].active == true) && (beaconList[q].schedulePublish)){
            if ((beaconCounter % maxBeaconsPerPublish) != 0){
            beaconJSON += ",";
            }
            beaconCounter++;
            beaconJSON += compileData(beaconList[q]);
            Log.trace("beaconJSON: " + beaconJSON);
            resetBeacon(q);
        }

        if (((beaconCounter % maxBeaconsPerPublish) == 0) && (beaconCounter > 0)){
            beaconsPublished += maxBeaconsPerPublish;
            Log.trace("about to publish compiled data");
            beaconJSON += "]}";
            Log.info("publishing beaconJSON: " + beaconJSON);
            publishInfo(beaconJSON,"",voltagePin,setupInfo,sData);
            beaconJSON = "\"beacon\":["; // reset beacon json

        }
    }
    Log.info("beaconCounter after loop: " + String(beaconCounter));
    Log.info("beaconsPublished after loop: " + String(beaconsPublished));
        if (beaconCounter > beaconsPublished){ // check to makes sure there are beacons and there are more than have already been published.
            Log.trace("about to publish compiled data");
            beaconJSON += "]}";
            publishInfo(beaconJSON,"",voltagePin,setupInfo,sData);
        }
    }else{
        Log.trace("not publishing beaconJSON");
    }

    
}

bool publishEnabledCheck(int inputPin){
    Log.trace("publish enabled: " + String(setupInfo.enabled));
    Log.trace("useScanningInput: " + String(setupInfo.useScanningInput));
    Log.trace("useScanningInputInverted: " + String(setupInfo.scanningInputInverted));
    if (setupInfo.enabled == true){
        if (setupInfo.useScanningInput == true){
            bool input = digitalRead(inputPin);
            // scanningInput Inverted is low is true.  non inverted is high is true.
            if (input != setupInfo.scanningInputInverted){
                Log.trace("using input pin and enabled");
                return true;
            }else{
                Log.trace("using input pin and disabled");
                return false;
            }
        }else{ // not using enable pin and enabled.
            Log.trace("not using the enable pin and enabled.");
            return true;
        }
    }else{ // the scanner is not enabled.
        Log.trace("scanner is disabled.");
        return false;
    }
}

void resetBeacon(int index){
    beaconList[index].active = false;
    beaconList[index].beaconAddress = "";
    beaconList[index].within = 0;
}

void beaconLogicLoop(void){
    //Log.trace("Beacon Logic Loop Running...");
    evaluateBeacons();
    //checkAndPublish(voltagePin);
}


void initBeacon(String address, int Rssi, int index) {
    Log.trace("Initializing beacon: " + address + "at index: " + String(index));

    beaconList[index].beaconAddress = address;
    beaconList[index].maxRSSI = Rssi;
    beaconList[index].withinCounter = 1;
    beaconList[index].missedCounter = 0;
    beaconList[index].RSSIoutCounter = 0;
    beaconList[index].within = false;
    beaconList[index].batteryVoltage = -1; // init to -1 which it cannot be physically.
    beaconList[index].schedulePublish = false;
    beaconList[index].active = true;
};

void checkInSightListForBeacon(String address) {
    beaconInList = false;
    for (int i = 0; i < (beaconListLength + 1); i++){
        if (address == beaconList[i].beaconAddress) {
            beaconInList = true;
            beaconIndex = i;
            break;
        }
    } 
}

void checkScannedListForBeacon(String address) {
    beaconInList = false;
    int k = -1;
    for (auto& beacon : Scanner.getLairdBt510()) {
        k++;
        if (address == beacon.getAddress().toString()){
            beaconInList = true;
            beaconIndex = k;
            break;
        }
    }
}


int updateScannerParams(String arg) {

    Particle.publish("updatedParams",arg, PRIVATE);
    
    // init temp values and set them to something that will never occur.
    int RSSI_Filter_temp = 1000; 
    int RSSIoutThresh_temp = -1;   
    int outThresh_temp = -1; 
    int withinThresh_temp = -1; 
    int measureSupVoltage_temp = -1;
    double supVoltageConversion_temp = -1;
    int useScanningInput_temp = -1;
    int scanningInputInverted_temp = -1;
    int enabled_temp = -1;
    int publishDelaySeconds_temp = -1;


    JSONValue outerObj = JSONValue::parseCopy(arg);

    JSONObjectIterator iter(outerObj);
    while(iter.next()) {
        if (iter.name() == "RSSI_filter") {
          RSSI_Filter_temp = iter.value().toInt(); // set RSSI filter value
          Log.info("updated RSSI Filter to %d", RSSI_Filter_temp);
        }
        else if (iter.name() == "RSSI_thresh"){
            RSSIoutThresh_temp = iter.value().toInt(); 
            Log.info("updated RSSIoutThresh to %d", RSSIoutThresh_temp);
        }
        else if (iter.name() == "out_thresh"){
            outThresh_temp = iter.value().toInt();
            Log.info("updated outThresh to %d", outThresh_temp); 
        }
        else if (iter.name() == "within_thresh"){
            withinThresh_temp = iter.value().toInt();
            Log.info("updated withinThresh to %d", withinThresh_temp);
        }
        else if (iter.name() == "measureVoltage"){
            measureSupVoltage_temp = iter.value().toInt();
            /*
            if (iter.value().toBool() == true){
                measureSupVoltage_temp = 1; 
            } else {
                measureSupVoltage_temp = 0; 
            }  
            */ 
            Log.trace("updated measureVoltage to %d", measureSupVoltage_temp); 
        }
        else if (iter.name() == "supVoltageConversion") {
        supVoltageConversion_temp = iter.value().toDouble();
        Log.trace("updated supVoltageConversion to %d", supVoltageConversion_temp);
        }
        else if (iter.name() == "useScanningInput") {
            useScanningInput_temp = iter.value().toInt();
            /*
            if (iter.value().toBool() == true){
                useScanningInput_temp = 1; 
            } else {
                useScanningInput_temp = 0; 
            }
            */
            Log.trace("updated useScanningInput to %d", useScanningInput_temp);   
        }
        else if (iter.name() == "scanningInputInverted") {
            scanningInputInverted_temp = iter.value().toInt();
            /*
             if (iter.value().toBool() == true){
                scanningInputInverted_temp = 1; 
            } else {
                scanningInputInverted_temp = 0; 
            }
            */
            Log.trace("updated scanningInputInverted to %d", scanningInputInverted_temp); 
        }
        else if (iter.name() == "enabled") {
            enabled_temp = iter.value().toInt();
            /*
            if (iter.value().toBool() == true){
                enabled_temp = 1; 
            } else {
                enabled_temp = 0; 
            }
            */
            Log.info("updated enabled to %d", enabled_temp);
        }
         else if (iter.name() == "beaconPublishIntervalSeconds"){
            publishDelaySeconds_temp = iter.value().toInt();
            Log.info("updated beaconPublishInterval to %d", publishDelaySeconds_temp);
        }
        else {
        Log.trace("unknown key %s", 
          (const char *)iter.name());
        }
    }
    // now check if there are values to update and update them

    if (RSSI_Filter_temp != 1000){
        logicParams.RSSI_Filter = RSSI_Filter_temp;
    }  
    if (RSSIoutThresh_temp != -1){
        logicParams.RSSIoutThresh = RSSIoutThresh_temp;
    }
    if (outThresh_temp != -1){
        logicParams.outThresh = outThresh_temp;
    }
    if (withinThresh_temp != -1){
        logicParams.withinThresh = withinThresh_temp;
    }
    if (measureSupVoltage_temp != -1){
        if (measureSupVoltage_temp == 1){
            setupInfo.measureSupVoltage = true;
        }else if (measureSupVoltage_temp == 0){
            setupInfo.measureSupVoltage = false;
        }
    }
    if (supVoltageConversion_temp != -1){
        setupInfo.supVoltageConversion = supVoltageConversion_temp;
    }
    if (useScanningInput_temp != -1){
        if (useScanningInput_temp == 1){
            setupInfo.useScanningInput = true;
        }else if (useScanningInput_temp == 0){
            setupInfo.useScanningInput = false;
        }
    }
    if (scanningInputInverted_temp != -1){
        if (scanningInputInverted_temp == 1){
            setupInfo.scanningInputInverted = true;
        }else if (scanningInputInverted_temp == 0){
            setupInfo.scanningInputInverted = false;
        }
    }
    if (enabled_temp != -1){
        Log.trace(String(enabled_temp));
        if (enabled_temp == 1){
            setupInfo.enabled = true;
        }else if (enabled_temp == 0){
            setupInfo.enabled = false;
            resetAllBeacons(); // remove all beacons from list to require a full restart on enable.
        }
    }
    if (publishDelaySeconds_temp != -1){
        logicParams.beaconPublishInterval = publishDelaySeconds_temp * 1000;
    }



return 1;


}

void resetAllBeacons(void){
    for (int i = 0; i < numBeaconsInMemory; i++){
        resetBeacon(i);
    }
}

int getBeaconPublishInterval(void){
    return logicParams.beaconPublishInterval;
}

bool getEnabled(void){
    return setupInfo.enabled;
}

void tryUpdateLocation(void){
    LocationPoint currentLocation = {};
    LocationService::instance().getLocation(currentLocation);

    if (currentLocation.latitude != 0 && currentLocation.longitude != 0){
        sData.Lat = currentLocation.latitude;
        sData.Long = currentLocation.longitude;
        sData.locationLastUpdated =  Time.now();
    }
    
    return;
}

void initLocation(void){
        sData.Lat = 0;
        sData.Long = 0;
        sData.locationLastUpdated =  0;
}

scannerData getScannerData(void){
    return sData;
}