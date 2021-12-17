void setup() {
    Serial.begin(9600);
    pump.stop(true);
    
    for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++) {
        dht[k].begin();
    }

    display.initialize();
    DEBUG_setup();
    updateTEMP();
    waterTank.init();

    //attachInterrupt(digitalPinToInterrupt(WATER_FLOW_PIN), countWater, FALLING);

    // if temperature is more than 4 degrees below or above Setpoint, OUTPUT will be set to min or max respectively
    //myPID.setBangBang(4);
    myPID.setBangBang(100); // so high that it disables the feature; setpoint +- 100
    myPID.setTimeStep(1000);// set PID update interval

    
    last250ms = millis();


// //    if (constrain(output, outputMin, outputMax - outputStep - 5) < output) {
// //      lcd.setCursor(0,0);
// //      lcd.print(F("AutoTune Err"));
// //      
// //      lcd.setCursor(0,1);
// //      lcd.print(F("otpt,hyst,otptStp"));
// //      while (1);
// //    }

  
//     // Select one, reference: https://github.com/Dlloydev/QuickPID/wiki
//     //_myPID.AutoTune(tuningMethod::ZIEGLER_NICHOLS_PI);
//     _myPID.AutoTune(tuningMethod::ZIEGLER_NICHOLS_PID);
//     //_myPID.AutoTune(tuningMethod::TYREUS_LUYBEN_PI);
//     //_myPID.AutoTune(tuningMethod::TYREUS_LUYBEN_PID);
//     //_myPID.AutoTune(tuningMethod::CIANCONE_MARLIN_PI);
//     //_myPID.AutoTune(tuningMethod::CIANCONE_MARLIN_PID);
//     //_myPID.AutoTune(tuningMethod::AMIGOF_PID);
//     //_myPID.AutoTune(tuningMethod::PESSEN_INTEGRAL_PID);
//     //_myPID.AutoTune(tuningMethod::SOME_OVERSHOOT_PID);
//     //_myPID.AutoTune(tuningMethod::NO_OVERSHOOT_PID);
  
//     _myPID.autoTune->autoTuneConfig(outputStep, hysteresis, setpoint, output, QuickPID::DIRECT, printOrPlotter, sampleTimeUs);
}


void loop() {
    currentTime = millis();

    myPID.run(); // every second
    
    // pidAutoUpdate();
    // pidLoopUpdate();

    if (currentTime >= (last250ms + 250)) { // update Setpoint 
        last250ms += 250;
        //updateSetPoint();
        //thermostat.updateSetpoint();

        // lcdUpdate();
        display.update();

        if (ms1000ctr >= 4) {
            ms1000ctr = 0;
            calcFlow();
            pump.update();
        }
        ms1000ctr++;
        
        
        if (ms2500ctr >= 10) {
            ms2500ctr = 0;

            //emaReadTank();
            waterTank.update();
            updateTEMP();
        }
        ms2500ctr++;
    }
}
