void updateTEMP() {
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        airSensor[i].readTemp();
    }

    if (isnan(airSensor[0].humid) || isnan(airSensor[0].tempF)) { // check for errors in two sensors
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
            Serial.println(F("ERROR BOTH SENSORS "));
        } else {
            if (errorCounter1 == 30) {
                Serial.print(F("DHT.main error! "));
                errorCounter1 = 0;
            }
            airSensor[0].humid = airSensor[1].humid;
            airSensor[0].tempF = airSensor[1].tempF;
        }
        errorCounter1++;
    } else {
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
            if (errorCounter2 == 30) {
                Serial.print(F("DHT.upstairs error! "));
                errorCounter2 = 0;
            }
            airSensor[1].humid = airSensor[0].humid;
            airSensor[1].tempF = airSensor[0].tempF;
            errorCounter2++;
        }
    }

    // check for strange readings (>20 from long avg)
    if (airSensor[0].currentEMA[2] != 0) { // don't run the first time 
        if (airSensor[0].tempF > 20 + airSensor[0].currentEMA[2] || airSensor[0].tempF < -20 + airSensor[0].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            Serial.print(F(" DHT.main outlier! ")); Serial.print(airSensor[0].tempF); 
            Serial.print(F(" vs EMA_Long ")); Serial.print(airSensor[0].currentEMA[2]);

            airSensor[0].tempF = airSensor[1].tempF;
        }
        if (airSensor[1].tempF > 20 + airSensor[1].currentEMA[2] || airSensor[1].tempF < -20 + airSensor[1].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            Serial.print(F(" DHT.upstairs outlier! ")); Serial.print(airSensor[1].tempF); 
            Serial.print(F(" vs EMA_Long ")); Serial.print(airSensor[1].currentEMA[2]);

            airSensor[1].tempF = airSensor[0].tempF;
        }
    }
    

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records
        airSensor[i].update();
    }

    weightedTemp = (airSensor[0].WEIGHT * airSensor[0].currentEMA[0] + airSensor[1].WEIGHT * airSensor[1].currentEMA[0]) / (airSensor[0].WEIGHT + airSensor[1].WEIGHT);
    temperature = weightedTemp;

    for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) { // update floor emas
        floorSensor[i].update();
    }

    int difference = int(abs(floorSensor[0].currentEMA - floorSensor[1].currentEMA)); // error check

    if (difference > 80) { // floor thermistor difference check
        if (VERBOSE) {
            Serial.print(F("Floor Read Error. Difference: "));
            Serial.println(difference);
        }
        if (floorSensor[0].currentEMA > floorSensor[1].currentEMA) {
            floorSensor[1].currentEMA = floorSensor[0].currentEMA;
        } else {
            floorSensor[0].currentEMA = floorSensor[1].currentEMA;
        }
    }
    floorEmaAvg = (floorSensor[0].currentEMA + floorSensor[1].currentEMA) / FLOOR_SENSOR_COUNT; // avg two readings

    if (tempDispCounter >= 4) { // every 4 reads = 10 seconds calculate trend and print info
        for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
            airSensor[i].updateEmaTrend();
        }
        avgTrend = (airSensor[0].trendEMA + airSensor[1].trendEMA) / 2.;

        if (tempDispCounter2 >= 6) {  // every minute: 
            debugEmaWater();

            if (changePerHourMinuteCounter < 59) {
              changePerHourMinuteCounter++;
            }
            tempDispCounter2 = 0;
            
            for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
                last59MedEMAs[k] = last59MedEMAs[k+1];
            }
            last59MedEMAs[58] = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
        }

        debugAirTemp();
        
        tempDispCounter2++;
        tempDispCounter = 0;
    }
    tempDispCounter++;


    // ************************************************ old

    //Set next pump state
    // if (weightedTemp > tempSetPoint || (avgTrend > 0 && weightedTemp > (tempSetPoint - AIR_TEMP_TREND_FACTOR * avgTrend))) { // check if should be off
    //     nextPumpState = 0;
    // } else { // else should be on:
    //     if (weightedTemp > tempSetPoint - 3) {   // if temp needs to move <3 degrees, turn on/start       OLD:  floorEmaAvg > FLOOR_WARMUP_TEMPERATURE && 
    //         nextPumpState = 1;
    //     } else if (weightedTemp > tempSetPoint - 5) { //if temp needs to move 3-5 degrees, go medium
    //         nextPumpState = 3; //  medium
    //     } else {                 // else, go full speed
    //         nextPumpState = 4; //  high
    //     }
    // }
    // if (currentPumpState == 0 && nextPumpState != 0) {   // Force update if switching to on from off (i.e. cut short the off cycle if turning on)
    //     checkPumpCycleState();
    // }
    // ************************************************ old
    

    if (outputVal <= 0 ) {
        analogWrite(PUMP_PIN, 0);
    } else {
        analogWrite(PUMP_PIN, (outputVal + 198)); //1 + 198 = 199
    }

}


void checkPumpCycleState() {
    uint32_t lastCycleDuration = 0;
    Serial.println();

    if (nextPumpState == 1) {
        if (currentPumpState == 1) {                                                // continue from on to on
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - On cycle continue phase"));
        } else if (currentPumpState != 0) {         //  on from start or warmup
            currentPumpState = 1;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            Serial.println(F("TESTING - On cycle initialization (after Start)"));
            analogWrite(PUMP_PIN, 199);
        } else {                             // if (currentPumpState == (0)          start from off or warm up (currentPumpState == (0 || 3))
            currentPumpState = 2;                                       //  set new current pump state to START
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME; //  add START interval
            lastCycleDuration = currentTime - cycleStartTime;           //  start new cycle..
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - Start cycle initialization"));
            analogWrite(PUMP_PIN, 223);
        }
    } else if (nextPumpState == 3) { // warm up changeto medium
        if (currentPumpState == 3) {                                                // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - medium cycle continue phase"));
        } else { // start as different phase
            currentPumpState = 3;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - medium cycle initialization"));
            analogWrite(PUMP_PIN, 223);
        }
    } else if (nextPumpState == 4) { // warm up
        if (currentPumpState == 4) {                                                // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - warm up cycle continue phase"));
        } else { // start as different phase
            currentPumpState = 4;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - warm up cycle initialization"));
            analogWrite(PUMP_PIN, 255);
        }
    } else if (nextPumpState == 0) { // off
        if (currentPumpState == 0) { // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - Off cycle continue phase"));
        } else { // start as different phase
            currentPumpState = 0;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - Off cycle initialization"));
            analogWrite(PUMP_PIN, 0);
        }
    }


    // if (nextPumpState == 1) {
    //     if (currentPumpState == 1) {                                                // continue from on to on
    //         pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
    //         Serial.println(F("TESTING - On cycle continue phase"));
    //     } else if (currentPumpState != 0) {         //  on from start or warmup
    //         currentPumpState = 1;
    //         pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
    //         Serial.println(F("TESTING - On cycle initialization (after Start)"));
    //         analogWrite(PUMP_PIN, 199);
    //     } else {                             // if (currentPumpState == (0)          start from off or warm up (currentPumpState == (0 || 3))
    //         currentPumpState = 2;                                       //  set new current pump state to START
    //         pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME; //  add START interval
    //         lastCycleDuration = currentTime - cycleStartTime;           //  start new cycle..
    //         cycleStartTime = currentTime;
    //         Serial.println(F("TESTING - Start cycle initialization"));
    //         analogWrite(PUMP_PIN, 223);
    //     }
    // } else {
    //     //all other nextPumpStates
    //     if (nextPumpState == currentPumpState) {
    //         pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
    //         Serial.print(F("TESTING - ")); Serial.print(CYCLE[nextPumpState].NAME); Serial.println(F(" cycle continue phase"));
    //     } else {

    //     }
    //     switch(nextPumpState) {

    //     }
    // }


    debugHighsLowsFloor(lastCycleDuration);
}

void countWater() {
    waterCounter++;
}


void updatePumpState() {
    if (currentTime >= (lastPumpUpdate + pumpUpdateInterval)) { // pump update timer
        checkPumpCycleState();
        lastPumpUpdate += pumpUpdateInterval;
    }

    if (checkPump != 0 && currentTime >= checkPump) { // special case pump check i.e. initialization, after thermostat changes
        checkPumpCycleState();
        checkPump = 0;
        // switch display back to temperature?
    }
}

void readTemp() {
    if (currentTime >= (lastTemperatureRead + TEMPERATURE_READ_INTERVAL)) { // Read temperature
        if (waterCounter != 0) {
            float thisDuration = (currentTime - lastTemperatureRead) / 1000;
            // this cycle's duration in seconds
            // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.    (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
            l_minute = (waterCounter / 7.5) / thisDuration; // divide by 2.5 because this is on a 2.5s timer  // returns vol per minute extrapolated from 2.5s
            totalVolume += (l_minute * thisDuration / 60);  // adds vol from the last 2.5s to totalVol
            waterCounter = 0;
        }

        updateTEMP();
        lastTemperatureRead += TEMPERATURE_READ_INTERVAL;
    }
}

void updateTempSetPoint() {
    if (currentTime >= (lastButtonCheck + BUTTON_CHECK_INTERVAL)) { // update tempSetPoint 
        lastButtonCheck += BUTTON_CHECK_INTERVAL;
        int8_t buttonStatus = 0;
        uint16_t buttonRead = analogRead(THERMOSTAT_BUTTONS_PIN);
        if (buttonRead > 63) {
            Serial.println(buttonRead);
            if (buttonRead > 831) {
                buttonStatus = 1;
                for (uint8_t k = 0; k < 2; k++) {  // reset record temps
                   airSensor[k].lowest = airSensor[k].tempF;
                }
            } else {
                buttonStatus = -1;
                for (uint8_t k = 0; k < 2; k++) {
                   airSensor[k].highest = airSensor[k].tempF;
                }
            }
            
            tempSetPoint += (0.5 * buttonStatus);
            setPoint = tempSetPoint;
            lcdPageSet();
            
            checkPump = currentTime + 3000; // wait 3 seconds before accepting new tempoerature in case button will be pressed more than once
        }
    }
}
void updateLcd() {
    if (currentTime >= (lastLcdUpdate + LCD_UPDATE_INTERVAL)) { // update LCD
        lastLcdUpdate += LCD_UPDATE_INTERVAL;

        lcdPage = (lcdPage == LCD_PAGE_MAX) ? 0: (lcdPage + 1); // cycle page

        lcdSwitchPage(); // display page
    }
}