void debugEmaWater() {
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
}

void debugAirTemp() {
    if (DEBUG) {
        Serial.print(airSensor[0].currentEMA[0]);
        if (airSensor[0].currentEMA[0] != airSensor[1].currentEMA[0]) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].currentEMA[0]);
        }
        Serial.print(F("  "));
    }
}


void debugHighsLowsFloor(uint32_t lastCycleDuration) {
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
