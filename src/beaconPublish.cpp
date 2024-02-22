
#include "Particle.h"
#include "beaconPublish.h"
#include "beaconLogic.h"
#include <string>
#include "location_service.h"
#include "beaconLogic.h"

int cloudPublishDataTestFunction(String command){
    Log.info("compileJSONFunction Running");
    Log.info(command);
    if (command =="old"){
        //compileJSONold();
        return 1;
    }
    else if (command == "new"){
        int TestvoltagePin = 3;
        setupData Test_setup;
        scannerData sData;
        sData.Lat = -1;
        sData.Long = -1;
        sData.locationLastUpdated = -1;
        publishInfo("","test",TestvoltagePin,Test_setup, sData);
        return 1;
    }
    else{
        //do nothing...
        return 0;
    }
}




void publishInfo(String beaconInfo, String command, int voltagePin, setupData setup, scannerData sData){
    Log.trace("Entered publishInfo function");
    if (command == "test"){
        beaconInfo = "{\"beaconTimeStamp\":1702425217139,\"beaconBatteryGood\":false,\"beaconID\":\"AA:BB:BB:BB:BB:AA\",\"beaconInsideCount\":88,\"beaconMaxRSSI\":-88}";
    }
    //scannerData sData;
    //LocationPoint currentLocation = {};
    //LocationService::instance().getLocation(currentLocation);
    //sData.Lat = currentLocation.latitude;
    //sData.Long = currentLocation.longitude;
    sData.scannerBatPct = System.batteryCharge();
    if (setup.measureSupVoltage == true){
        sData.scannerSupVoltage = analogRead(voltagePin)/4096*3.3 * setup.supVoltageConversion; // converts to 3.3V scale and then applies offset based on voltage divider.
    }
    String messageJSON = ""; 
    messageJSON += "{\"scanner\":[{";
    messageJSON += "\"lat\":\"" + String(sData.Lat,9) + "\",";
    messageJSON += "\"long\":\"" + String(sData.Long,9) + "\",";
    messageJSON += "\"locLastUpdated\":\"" + String(sData.locationLastUpdated) + "\",";
    messageJSON += "\"scannerBatPct\":\"" + String(sData.scannerBatPct, 1) + "\",";
    messageJSON += "\"scannerSupVoltage\":\"" + String(sData.scannerSupVoltage,1) + "\"}],";
    Log.trace("added scanner data");
    Log.trace("beaconInfo: " + beaconInfo);
    messageJSON += beaconInfo;
    Log.trace("messageJSON:  " + messageJSON);
    delay(10);
    Particle.publish("beaconLeft", messageJSON, PRIVATE); // uncomment when ready to really test....
    //Log.trace("Publishing beacon: "+ data.beaconID);
    //Log.trace("beaconMaxRSSI: " + String(data.beaconMaxRSSI, 1));
    //Log.trace("beaconBatteryGood: " + beaconBatteryGoodString);
    //Log.trace("Latitude floating point: %.9f", data.Lat);
    //Log.trace("Latitude floating point: %.9f", data.Long);
    //Log.trace("Lat: " + String(data.Lat,9));
    //Log.trace("Long: " + String(data.Long,9));


    
}