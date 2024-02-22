/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Particle.h"
#include "tracker_config.h"
#include "tracker.h"
#include "beaconLogic.h"
#include "beaconPublish.h"
#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

#if TRACKER_PRODUCT_NEEDED
PRODUCT_ID(TRACKER_PRODUCT_ID);
#endif // TRACKER_PRODUCT_NEEDED
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

STARTUP(
    Tracker::startup();
);

// Pin definition for supplied voltage...
int supVoltagePin = D3;
long lastBeaconPubCheck = millis();
long lastBeaconLoop = millis();
long loopTime = 2000;
long pubTime = 1 * 60 * 1000; // once a minute.
long startupTimeDelayMax = 1 * 60 * 1000;
long startupTime = millis();
bool publishStartup = true;
bool enabled = true;

SerialLogHandler logHandler(115200, LOG_LEVEL_INFO, {
    { "app.gps.nmea", LOG_LEVEL_NONE },
    { "app.gps.ubx",  LOG_LEVEL_NONE },
    { "ncp.at", LOG_LEVEL_NONE },
    { "net.ppp.client", LOG_LEVEL_NONE },
});

void setup()
{
    Tracker::instance().init();
    BLE.on();
    Scanner.startContinuous();
    lastBeaconPubCheck = millis();
    Particle.function("testBeaconPublish", cloudPublishDataTestFunction);
    Particle.function("updateSettings", updateScannerParams);
    pinMode(supVoltagePin, INPUT);
    Particle.keepAlive(23 * 60);    // send a ping every 23 minutes
    initLocation();
}

void loop()
{
    Tracker::instance().loop();
    Scanner.loop();
    // check if you need to run publish loop
    pubTime = getBeaconPublishInterval();
    enabled = getEnabled();
    //Log.trace("pubTime: " + String(pubTime));
    // Log.trace("compared value: " + String(millis() - lastBeaconPubCheck));
    if (enabled == true) {
         if (((millis() - lastBeaconPubCheck) > pubTime) && Particle.connected()) {
            lastBeaconPubCheck = millis();
            checkAndPublish(supVoltagePin);
        }
        // check if you need to run beacon logic loop.
        if((millis()-lastBeaconLoop) > loopTime){
            lastBeaconLoop = millis();
            beaconLogicLoop();
            tryUpdateLocation();
        }
    }
    
    if (Particle.connected() && publishStartup){
        String timeString = String(Time.now());
        String startupJSON = "{\"timeStamp\":" + timeString +"}";
        Particle.publish("startUp", startupJSON, PRIVATE);
        Log.trace("finished publish startup");
        publishStartup = false;
        delay(10);
    }

}
