void setup() {
    Serial.begin(9600);
    pinMode(THERMOSTAT_BUTTONS_PIN, INPUT);

    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FLOOR_TEMP_PIN[0], INPUT);
    pinMode(FLOOR_TEMP_PIN[1], INPUT);
    

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
    currentTime = millis();

    myPID.run(); // every second

    // if (currentTime >= (lastPumpUpdate + pumpUpdateInterval)) { // pump update timer
    //     updatePumpState();
    //     lastPumpUpdate += pumpUpdateInterval;
    // }

    // if (delayPump != 0 && currentTime >= delayPump) { // special case pump check i.e. initialization, after thermostat changes
    //     updatePumpState();
    //     delayPump = 0;
    //     // switch display back to temperature?
    // }
    


    if (currentTime >= (lastButtonCheck + BUTTON_CHECK_INTERVAL)) { // update tempSetPoint 
        lastButtonCheck += BUTTON_CHECK_INTERVAL;
        updateSetPoint();
    }



    if (currentTime >= (lastTemperatureRead + TEMPERATURE_READ_INTERVAL)) { // Read temperature


        cycleDuration = currentTime - cycleStartTime; // for LCD
        updateTEMP();
        lastTemperatureRead += TEMPERATURE_READ_INTERVAL;
    }



    

    if (currentTime >= lastSecond + secondInterval) { // every second
        lastSecond += secondInterval;
        calcWater();
        pump.update();

        lcdUpdate();

    }
}