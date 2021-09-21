void setup() {
    Serial.begin(9600);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FLOOR_TEMP_PIN[0], INPUT);
    pinMode(FLOOR_TEMP_PIN[1], INPUT);
    pinMode(THERMOSTAT_BUTTONS_PIN, INPUT);

    for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++) {
        dht[k].begin();
    }

    lcdInitialize();

    analogWrite(PUMP_PIN, 0);
    attachInterrupt(digitalPinToInterrupt(WATER_FLOW_PIN), countWater, FALLING);
    Serial.println();
    Serial.print(F("Version "));
    Serial.println(VERSION_NUMBER);
    Serial.println(F("DHTxx test!"));

    delay(1200);
    lastButtonCheck = millis();
    lastPumpUpdate = lastButtonCheck;
    lastTemperatureRead = lastButtonCheck;
    cycleStartTime = lastButtonCheck;
    updateTEMP();


    delay(1200);
    //if temperature is more than 4 degrees below or above tempSetPoint, OUTPUT will be set to min or max respectively
    myPID.setBangBang(4);
    //set PID update interval to 4000ms
    myPID.setTimeStep(1000);
}



void loop() {
    myPID.run();
    currentTime = millis();

    // if (currentTime >= (lastPumpUpdate + pumpUpdateInterval)) { // pump update timer
    //     checkPumpCycleState();
    //     lastPumpUpdate += pumpUpdateInterval;
    // }

    // if (checkPump != 0 && currentTime >= checkPump) { // special case pump check i.e. initialization, after thermostat changes
    //     checkPumpCycleState();
    //     checkPump = 0;
    //     // switch display back to temperature?
    // }

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

    
    if (currentTime >= (lastLcdUpdate + LCD_UPDATE_INTERVAL)) { // update LCD
        lastLcdUpdate += LCD_UPDATE_INTERVAL;

        switch(nextLcdPage) {   // display proper page
            case(0):
                    lcdPage1();
                break;

            case(1):
                    lcdPage3();
                break;
        }


        nextLcdPage = (nextLcdPage == LCD_PAGE_MAX) ? 0: (nextLcdPage + 1); // set next page

    }
}