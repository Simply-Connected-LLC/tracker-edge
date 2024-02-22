#include "particle.h"
#include "beaconLogic.h"
#pragma once

class scannerData {
public:
    //String version;
    double scannerSupVoltage;
    double scannerBatPct;
    double Lat;
    double Long;
    int locationLastUpdated;
};

int cloudPublishDataTestFunction(String command);

void publishInfo(String beaconInfo, String command, int voltagePin, setupData setup,scannerData sData);

//int updateScannerParams(String arg);

