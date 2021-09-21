// float ReadFloorTemperature(int pin, int a, int val = 0, int z = 0) {
//     for (z = 0; z < a; z++) {
//         val = val + analogRead(pin);
//     }
//     return val / a;
// }

void updateTEMP() {
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        airSensor[i].readTemp();
        // airSensor[i].tempF = airSensor[i].sensor->readTemperature(true);
        // airSensor[i].humid = airSensor[i].sensor->readHumidity();
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
            Serial.print(F(" DHT.main outlier! ")); 
            Serial.print(airSensor[0].tempF); 
            Serial.print(F(" vs EMA_Long ")); 
            Serial.print(airSensor[0].currentEMA[2]);

            airSensor[0].tempF = airSensor[1].tempF;
        }
        if (airSensor[1].tempF > 20 + airSensor[1].currentEMA[2] || airSensor[1].tempF < -20 + airSensor[1].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            Serial.print(F(" DHT.upstairs outlier! ")); 
            Serial.print(airSensor[1].tempF); 
            Serial.print(F(" vs EMA_Long ")); 
            Serial.print(airSensor[1].currentEMA[2]);

            airSensor[1].tempF = airSensor[0].tempF;
        }
    }
    

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records
        airSensor[i].update();
        // if (airSensor[i].highest < airSensor[i].tempF) {
        //     airSensor[i].highest = airSensor[i].tempF;
        // }
        // if (airSensor[i].lowest > airSensor[i].tempF) {
        //     airSensor[i].lowest = airSensor[i].tempF;
        // }

        // for (uint8_t z = 0; z < 3; z++) { // update EMA
        //     if (airSensor[i].lastEMA[z] == 0) {
        //         airSensor[i].lastEMA[z] = airSensor[i].tempF;
        //     }

        //     airSensor[i].currentEMA[z] = airSensor[i].tempF * EMA_MULT[z] + airSensor[i].lastEMA[z] * (1 - EMA_MULT[z]);
        //     airSensor[i].lastEMA[z] = airSensor[i].currentEMA[z];
        // }
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
            if (airSensor[i].currentEMA[0] < airSensor[i].currentEMA[1]) { // if (now) temp < (medium avg) temp; temp is falling: 
                if (airSensor[i].currentEMA[1] < weightedTemp) { // if medium 
                    if (airSensor[i].trendEMA <= -DOWNTREND) {
                        airSensor[i].trendEMA += DOWNTREND; // fast going back to 0
                    } else {
                        airSensor[i].trendEMA = 0;
                    }
                } else {
                    //                if (airSensor[i].currentEMA[1] - airSensor[i].currentEMA[0] < 0.5) {  //if difference is small, this means temp is falling fast, so we'll up the trend quicker so pump turns on sooner.
                    //                    airSensor[i].trendEMA += -UPTREND * 2;    //slow going down
                    //                } else {
                    //                    airSensor[i].trendEMA += -UPTREND;    //slow going down     //if difference is large, this means temp is falling slowly, so we don't need the pump to come on as soon.
                    //                }
                    airSensor[i].trendEMA += -UPTREND; //slow going down
                }
            }

            if (airSensor[i].currentEMA[0] > airSensor[i].currentEMA[1]) { // if rising
                if (airSensor[i].currentEMA[1] > weightedTemp) {
                    if (airSensor[i].trendEMA >= DOWNTREND) {
                        airSensor[i].trendEMA += -DOWNTREND; // fast going back to 0
                    } else {
                        airSensor[i].trendEMA = 0;
                    }
                } else {
                    //                if (airSensor[i].currentEMA[0] - airSensor[i].currentEMA[1] < 0.5) {  //if difference is small, this means temp is rising fast, so we'll up the trend quicker so pump shuts off sooner.
                    //                    airSensor[i].trendEMA += UPTREND * 2;    //slow going up
                    //                } else {
                    //                    airSensor[i].trendEMA += UPTREND;    //slow going up      //if difference is large, this means temp is rising slow, so we don't want the pump to shut off as soon.
                    //                }
                    airSensor[i].trendEMA += UPTREND; //slow going up
                }
            }

            if (airSensor[i].trendEMA > AIR_TEMP_TREND_MINMAX) {
                airSensor[i].trendEMA = AIR_TEMP_TREND_MINMAX;
            }
            if (airSensor[i].trendEMA < -AIR_TEMP_TREND_MINMAX) {
                airSensor[i].trendEMA = -AIR_TEMP_TREND_MINMAX;
            }
        }
        avgTrend = (airSensor[0].trendEMA + airSensor[1].trendEMA) / 2.;

        if (tempDispCounter2 >= 6) {  // every minute: 
            if (DEBUG) {
                Serial.println();
                Serial.print(F(" °F "));
                Serial.print(F("  EMA_Long.")); Serial.print(airSensor[0].currentEMA[2]);
                if (airSensor[0].currentEMA[2] != airSensor[1].currentEMA[2]) {
                    Serial.print(F("/"));
                    Serial.print(airSensor[1].currentEMA[2]);
                }
                Serial.print(F("  _Med.")); Serial.print(airSensor[0].currentEMA[1]);
                if (airSensor[0].currentEMA[1] != airSensor[1].currentEMA[1]) {
                    Serial.print(F("/"));
                    Serial.print(airSensor[1].currentEMA[1]);
                }
                Serial.print(F("  Trend: ")); Serial.print(airSensor[0].trendEMA);
                if (airSensor[0].trendEMA != airSensor[1].trendEMA) {
                    Serial.print(F(" | "));
                    Serial.print(airSensor[1].trendEMA);
                }

                Serial.print(F(" Water(Flow: "));
                Serial.print(l_minute);
                Serial.print(F(" l/m, totalVol: "));
                Serial.print(totalVolume);
                Serial.print(F(" liters, "));
                //check tank levels
                uint16_t tankReading = analogRead(WATER_LEVEL_PIN);
                Serial.print(F("Level: "));
                if (tankReading > 999) { // ~1023
                    Serial.print(F("< 25%"));
                } else if (tankReading > 685) { // ~750
                    Serial.print(F("25~50%"));
                } else if (tankReading > 32) { // ~330
                    Serial.print(F("50~75%"));
                } else { //~0
                    Serial.print(F("> 75%"));
                }
                Serial.print(F(") "));
            }


            if (changePerHourMinuteCounter < 59) {
              changePerHourMinuteCounter++;
            }
            tempDispCounter2 = 0;
            
            for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
                last59MedEMAs[k] = last59MedEMAs[k+1];
            }
            last59MedEMAs[58] = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
        }

        if (DEBUG) {
            Serial.print(airSensor[0].currentEMA[0]);
            if (airSensor[0].currentEMA[0] != airSensor[1].currentEMA[0]) {
                Serial.print(F("/"));
                Serial.print(airSensor[1].currentEMA[0]);
            }
            Serial.print(F("  "));
        }

        //lcdPage1(); // now happening inside its own loop!
        
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
    

    if (outputVal < 199) {
        analogWrite(PUMP_PIN, 0);
    } else if (outputVal > 255) {
        analogWrite(PUMP_PIN, 255);
    } else {
        analogWrite(PUMP_PIN, outputVal);
    }

}


void checkPumpCycleState() {
    unsigned long lastCycleDuration = 0;
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

    if (DEBUG) {
        if (lastCycleDuration != 0) {  // if end of cycle: 
            Serial.print(F(" ms:"));
            Serial.print(lastCycleDuration);
            uint16_t hours = lastCycleDuration / 3600000;
            lastCycleDuration -= (hours * 3600000);
            uint16_t minutes = lastCycleDuration / 60000;
            lastCycleDuration -= (minutes * 60000);
            uint16_t seconds = lastCycleDuration / 1000;
            lastCycleDuration -= (seconds * 1000);
            if (seconds >= 60) {
                minutes += 1;
                seconds -= 60;
            }
            if (minutes >= 60) {
                hours += 1;
                minutes -= 60;
            }
            if (lastCycleDuration >= 500 && (minutes > 0 || hours > 0)) {
                seconds += 1;
            }

            Serial.print(F("                          Total cycle time: "));

            if (hours > 0) {
                Serial.print(hours);
                Serial.print(F("h"));
            }
            if (minutes > 0) {
                Serial.print(minutes);
                Serial.print(F("m"));
            }
            if (seconds > 0) {
                Serial.print(seconds);
                Serial.print(F("s"));
            }
            if (hours == 0 && minutes == 0) {
                Serial.print(lastCycleDuration);
            }
            //Serial.println();
        }

        Serial.println();
        Serial.print(F(" Set: "));
        Serial.print(tempSetPoint, 1);
        Serial.print(F("°F"));
        Serial.print(F("   Floor: "));
        Serial.print(floorSensor[0].currentEMA);
        if (floorSensor[0].currentEMA != floorSensor[1].currentEMA) {
            Serial.print(F(" / "));
            Serial.print(floorSensor[1].currentEMA);
        }
        Serial.print(F("   PWM "));
        if (CYCLE[currentPumpState].PWM == 0) {
            Serial.print(F("  "));
        }
        Serial.print(CYCLE[currentPumpState].PWM);
        Serial.print(F("                                       *Pump"));
        Serial.print(CYCLE[currentPumpState].NAME);
        Serial.print(F("(HIGHS "));
        Serial.print(airSensor[0].highest);
        if (airSensor[0].highest != airSensor[1].highest) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].highest);
        }
        Serial.print(F(" LOWS "));
        Serial.print(airSensor[0].lowest);
        if (airSensor[0].lowest != airSensor[1].lowest) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].lowest);
        }
        Serial.print(F(")"));
        if (changePerHourMinuteCounter == 60) {
            Serial.print(F("Temp Change per Hr (Medium EMA): "));
            float avgNow = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
            float diff = last59MedEMAs[0] - avgNow;
            Serial.print(diff);
        }
        Serial.println();
    }
}

void countWater() {
    waterCounter++;
}