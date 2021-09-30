void updateTEMP() {
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        air[i].readTemp();
    }

    if (isnan(air[0].humid) || isnan(air[0].tempF)) { // check for errors in two main sensors
        if (isnan(air[1].humid) || isnan(air[1].tempF)) {
            Serial.println(F("ERROR BOTH SENSORS "));
        } else {
            if (errorCounter1 == 30) {
                Serial.print(F("DHT.main error! "));
                errorCounter1 = 0;
            }
            air[0].humid = air[1].humid;
            air[0].tempF = air[1].tempF;
        }
        errorCounter1++;
    } else {
        if (isnan(air[1].humid) || isnan(air[1].tempF)) {
            if (errorCounter2 == 30) {
                Serial.print(F("DHT.upstairs error! "));
                errorCounter2 = 0;
            }
            air[1].humid = air[0].humid;
            air[1].tempF = air[0].tempF;
            errorCounter2++;
        }
    }

    // check for strange readings (>20 from long avg) on two main air sensors
    if (air[0].currentEMA[2] != 0) { // don't run the first time 
        if (air[0].tempF > 20 + air[0].currentEMA[2] || air[0].tempF < -20 + air[0].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            if (DEBUG) {
                Serial.print(F(" DHT.main outlier! ")); Serial.print(air[0].tempF); 
                Serial.print(F(" vs EMA_Long ")); Serial.print(air[0].currentEMA[2]);
            }

            air[0].tempF = air[1].tempF;
        }
        if (air[1].tempF > 20 + air[1].currentEMA[2] || air[1].tempF < -20 + air[1].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            if (DEBUG) {
                Serial.print(F(" DHT.upstairs outlier! ")); Serial.print(air[1].tempF); 
                Serial.print(F(" vs EMA_Long ")); Serial.print(air[1].currentEMA[2]);
            }

            air[1].tempF = air[0].tempF;
        }
    }
    

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records (for all air sensors)
        air[i].update();
    }

    weightedTemp = (air[0].WEIGHT * air[0].currentEMA[0] + air[1].WEIGHT * air[1].currentEMA[0]) / (air[0].WEIGHT + air[1].WEIGHT);
    temperature = weightedTemp;

    for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) { // update floor emas
        floorSensor[i].update();
    }

    int difference = int(abs(floorSensor[0].currentEMA - floorSensor[1].currentEMA)); // floor sensors error check

    if (difference > 80) { // floor thermistor difference check
        if (DEBUG) {
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
    
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        air[i].updateEmaTrend();
    }
    avgTrend = (air[0].trendEMA + air[1].trendEMA) / 2.;



    if (tempDispCounter >= 4) { // every 4 reads = 10 seconds, calculate trend and print info
        debugAirEmas();

        tempDispCounter = 0;
    }
    tempDispCounter++;

    if (tempDispCounter2 >= 24) {  // every minute: 
        debugEmaWater();

        if (changePerHourMinuteCounter < 59) {
            changePerHourMinuteCounter++;
        }
        tempDispCounter2 = 0;
        
        for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
            last59MedEMAs[k] = last59MedEMAs[k+1];
        }
        last59MedEMAs[58] = (air[0].currentEMA[1] + air[1].currentEMA[1]) / 2;
    }

    tempDispCounter2++;





    // // ************************************************ old

    // //Set next pump state
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
    //     updatePumpState();
    // }
    // // ************************************************ old



    // new

    // do nothing, only need to update weightedTemp for PID

}



void updatePumpState() {
    Serial.println();

    pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase, overwritten if necessary later
    if (nextPumpState == 1) {
        if (currentPumpState == 1) { // continue to on from on
            if (DEBUG) {
                Serial.print(CYCLE[5].NAME);
            }

        } else {    // to on from other
            currentPumpState = (currentPumpState != 0) ? 1 : 2;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME; //  add START interval

            if (currentPumpState == 0) { // from off, start new cycle..
                lastCycleDuration = currentTime - cycleStartTime;
                cycleStartTime = currentTime;
            }
        }
    } else { // all other nextPumpStates
        if (nextPumpState == currentPumpState) {
            if (DEBUG) {
                Serial.print(CYCLE[5].NAME);
            }

        } else { // start fresh phase
            currentPumpState = nextPumpState;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
        }
    }
    analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
    if (DEBUG) {
        Serial.println(CYCLE[currentPumpState].NAME);
    }

    debugHighsLowsFloor();

    if (lastCycleDuration != 0) {  // if end of cycle, reset these
        lastCycleDuration = 0;
        cycleDuration = 0;
    }
}

void countWater() {
    waterCounter++;
}


void updateSetPoint() {
    int8_t buttonStatus = 0;
    uint16_t buttonRead = analogRead(THERMOSTAT_BUTTONS_PIN);
    if (buttonRead > 63) {
        Serial.println(buttonRead);
        if (buttonRead > 831) {
            buttonStatus = 1;
            for (uint8_t k = 0; k < 2; k++) {  // reset record temps
                air[k].lowest = air[k].tempF;
            }
        } else {
            buttonStatus = -1;
            for (uint8_t k = 0; k < 2; k++) {
                air[k].highest = air[k].tempF;
            }
        }
        
        tempSetPoint += (0.5 * buttonStatus);
        setPoint = tempSetPoint;
        lcdPageSet();
        
        delayPump = currentTime + 3000; // wait 3 seconds before accepting new tempoerature in case button will be pressed more than once
    }
}


void calcWater() {
    if (waterCounter != 0) {
        float thisDuration = (currentTime - lastTemperatureRead) / 1000;
        // this cycle's duration in seconds
        // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.    (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
        l_minute = (waterCounter / 7.5) / thisDuration; // divide by 2.5 because this is on a 2.5s timer  // returns vol per minute extrapolated from 2.5s
        totalVolume += (l_minute * thisDuration / 60);  // adds vol from the last 2.5s to totalVol
        waterCounter = 0;
    }
}