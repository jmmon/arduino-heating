void setup() {
    Serial.begin(9600);
    pump.stop(true);
    
    for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++) {
        dht[k].begin();
//        dhtnew[k].setType(22);
    }

    

    display.initialize();
    DEBUG_setup();
    updateTEMP();
    waterTank.init();


    pinMode(WATER_FLOW_PIN, INPUT);
    digitalWrite(WATER_FLOW_PIN, HIGH);

    WATER_FLOW_INTURRUPT = digitalPinToInterrupt(WATER_FLOW_PIN); // set in setup
//    attachInterrupt(WATER_FLOW_INTURRUPT, waterPulseCount, FALLING);

    // // AutoPID setup:
    // // if temperature is more than 4 degrees below or above Setpoint, OUTPUT will be set to min or max respectively
    // //myPID.setBangBang(4);
    // myPID.setBangBang(100); // so high that it disables the feature; setpoint +- 100
    // myPID.setTimeStep(1000);// set PID update interval


    // ArduPID setup:
    myController.begin(&Input, &Output, &Setpoint, Kp, Ki, Kd);
    const unsigned int _minSamplePeriodMs = 1000;
    myController.setSampleTime(_minSamplePeriodMs);
    myController.setOutputLimits(outputMin, outputMax);
   const double windUpMin = -10;
   const double windUpMax = 10;
    myController.setWindUpLimits(windUpMin, windUpMax);
    myController.start();

    
    last250ms = millis();
    
    tone(TONE_PIN, NOTE_C5, 100);

}


void loop() {
    currentTime = millis();

    // // AutoPID loop:
    // myPID.run(); // every second

    // ArduPID loop:
    myController.compute();

    // 250ms Loop:
    if (currentTime >= (last250ms + 250)) { // update Setpoint 
        last250ms += 250;
        // update display screen
        display.update();
        
        // 1000ms Loop:
        if (ms1000ctr >= 4) {
            ms1000ctr = 0;
            // calc flowrate (not working)
            // calcFlow();
            // run pump update
            pump.update();
						// waterCalcFlow();        
        }
        ms1000ctr++;
        

        // 2500ms Loop:
        if (ms2500ctr >= 10) {
            ms2500ctr = 0;

            // update water tank level
            waterTank.update();
            // update air temperature
            updateTEMP();
        }
        ms2500ctr++;
    }
}




/*
Insterrupt Service Routine
 */
void waterPulseCounter()
{
  // Increment the pulse counter
  waterPulseCount++;
}



void waterCalcFlow() {
    // Only process counters once per second
    if (waterPulseCount > 0) {
        // Disable the interrupt while calculating flow rate and sending the value to
        // the host
        detachInterrupt(WATER_FLOW_INTURRUPT);
            
        // Because this loop may not complete in exactly 1 second intervals we calculate
        // the number of milliseconds that have passed since the last execution and use
        // that to scale the output. We also apply the calibrationFactor to scale the output
        // based on the number of pulses per second per units of measure (litres/minute in
        // this case) coming from the sensor.
        flowRate = ((1000.0 / (millis() - oldTime)) * waterPulseCount) / calibrationFactor;
        
        // Note the time this processing pass was executed. Note that because we've
        // disabled interrupts the millis() function won't actually be incrementing right
        // at this point, but it will still return the value it was set to just before
        // interrupts went away.
        oldTime = millis();
        
        // Divide the flow rate in litres/minute by 60 to determine how many litres have
        // passed through the sensor in this 1 second interval, then multiply by 1000 to
        // convert to millilitres.
        flowMilliLitres = (flowRate / 60) * 1000;
        
        // Add the millilitres passed in this second to the cumulative total
        totalMilliLitres += flowMilliLitres;
        
        // // Print the flow rate for this second in litres / minute
        // Serial.print(int(flowRate));  // Print the integer part of the variable

        // // Print the cumulative total of litres flowed since starting
        // Serial.print(totalMilliLitres/1000);
        // Serial.print("L");
        
        // Reset the pulse counter so we can start incrementing again
        waterPulseCount = 0;
        
        // Enable the interrupt again now that we've finished sending output
        attachInterrupt(WATER_FLOW_INTURRUPT, waterPulseCount, FALLING);
    }
}
