#pragma once

class beaconRSSI {
public:
    int withinCounter = 0;
    int RSSIoutCounter = 0;
    int missedCounter = 0;
    int maxRSSI;
    String beaconAddress;
    bool within = false;
    bool schedulePublish = false;
    uint16_t batteryVoltage;
    bool active = false;
    long publishTime;
};


class logicThresholds {
public:
    int RSSI_Filter = -70;
    int RSSIoutThresh = 10; // 
    int outThresh = 30; // set to one minute since it is a scan every 2 seconds 
    int withinThresh = 30; // set to one minute since it is a scan every 2 seconds 
    long beaconPublishInterval = 60 * 1000;
};

class setupData {
public:
    bool measureSupVoltage = false;
    double supVoltageConversion = 1;
    bool useScanningInput = false;
    bool scanningInputInverted = false;
    bool enabled = true;
};

void beaconLogicLoop(void);

void checkAndPublish(int voltagePin);

int updateScannerParams(String arg);

int getBeaconPublishInterval(void);

bool getEnabled(void);

void initLocation(void);

void tryUpdateLocation(void);

