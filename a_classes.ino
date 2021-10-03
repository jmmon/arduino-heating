class Air_C {
    private:
        DHT *sensor;

    public:
        Air_C(DHT *s, String l, float w) {
            sensor = s;
            label = l;
            WEIGHT = w;
        }

        float tempC = 0;
        float tempF = 0;
        float WEIGHT = 1;
        float humid = 0;

        float highest = -1000;
        float lowest = 1000;
        float currentEMA[3]; // {short, medium, long} EMA
        float lastEMA[3];    // {short, medium, long}
        
        bool working = true;
        String label;

        readTemp() {
            //read temp/humid
            tempC = sensor->readTemperature();
            tempF = sensor->readTemperature(true);
            humid = sensor->readHumidity();
        };

        float getTemp(bool weighted = false) {
            if (weighted) return (tempF * WEIGHT);
            return tempF;
        }

        update() {
            //update highest / lowest
            if (highest < tempF) {
                highest = tempF;
            }
            if (lowest > tempF) {
                lowest = tempF;
            }

            //update EMAs (short, medium, long)
            for (uint8_t ema = 0; ema < 3; ema++) { // update EMA
                if (lastEMA[ema] == 0) {
                    lastEMA[ema] = tempF;
                }

                currentEMA[ema] = tempF * EMA_MULT[ema] + lastEMA[ema] * (1 - EMA_MULT[ema]);
                lastEMA[ema] = currentEMA[ema];
            }
        };

}
air[] = {
    Air_C(&dht[0], "MAIN", 1),
    Air_C(&dht[1], "UPSTAIRS", 2),
    Air_C(&dht[2], "OUTSIDE", 2),
    Air_C(&dht[3], "GREENHOUSE", 2),
};

















class FloorSensor_C {
    private:
        uint8_t PIN;

    public:
        FloorSensor_C(uint8_t pin) { // constructor
            PIN = pin;
            pinMode(pin, INPUT);
        };

        float currentEMA = 0;
        float lastEMA = 0;

        uint16_t readTemp(int a = 10) {
            int val = 0;
            for (int z = 0; z < a; z++) {
                val += analogRead(PIN);
            }
            return val / a;
        };

        update() {
            if (lastEMA == 0) {
                lastEMA = readTemp();
            }
            currentEMA = (readTemp() * FLOOR_EMA_MULT + lastEMA * (1 - FLOOR_EMA_MULT));
            lastEMA = currentEMA;
        };

        

} 
floorSensor[] = { 
    FloorSensor_C(FLOOR_TEMP_PIN[0]), 
    FloorSensor_C(FLOOR_TEMP_PIN[1])
};


















class Pump_C {
    private:
        uint8_t PIN = 5;

        uint8_t STARTING_PHASE_PWM = 220;
        uint8_t ON_PHASE_BASE_PWM = 191;

        uint8_t STARTING_PHASE_SECONDS = 4;
        uint8_t ON_CYCLE_MINIMUM_SECONDS = 180;
        uint16_t OFF_CYCLE_MINIMUM_SECONDS = 300;

        uint8_t state = 0;
        uint32_t timeRemaining = 0;
        uint8_t pwm = 0;

        uint32_t duration = 0;
        uint32_t lastDuration = 0;

        uint32_t accumOn = 0;
        uint32_t accumOff = 0;


    public:
        Pump_C () { // constructor
            pinMode(PIN, OUTPUT);
        }

        setState(uint8_t i) {
            state = i;
        }

        uint8_t getState() {
            return state;
        }
        
        uint32_t getDuration() {
            return duration;
        }

        resetDuration() {
            lastDuration = duration;
            duration = 0;
        }

        String getStatus() {
            if (state == 1) return String(pwm);
            return "OFF";
        }

        uint32_t getAccumOn() {
            return accumOn;
        }

        uint32_t getAccumOff() {
            return accumOff;
        }

        uint8_t getPwm() {
            return pwm;
        }



        void start() {
            setState(2);
            resetDuration();
            timeRemaining = ON_CYCLE_MINIMUM_SECONDS; // 3 minute minimum
            pwm = STARTING_PHASE_PWM;
            analogWrite(PIN, pwm);
        }

        void stop() {
            setState(0);
            resetDuration();
            timeRemaining = OFF_CYCLE_MINIMUM_SECONDS; // 5 minute minimum
            pwm = 0;
            analogWrite(PIN, pwm);
        }

        void checkAfter(uint16_t delaySeconds = 3) {
            setState(3);
            timeRemaining = delaySeconds;
        }


        void update() { // called every second  
            debugHighsLowsFloor();

            duration++;

            if (state == 0) {
                accumOff++;
            } else if (state != 3) {
                accumOn++;
            }

            if (timeRemaining > 0) { // if restrained by timer
                timeRemaining --;
                switch(state) {
                    case(0): // if off (with timeRemaining)
                        if (Output == outputMax) { // special case
                            start();
                        } else {
                            pwm = 0;
                        }
                        break;

                    case(1): // if on
                        pwm = ((Output > 127) ? 
                            (ON_PHASE_BASE_PWM + ((Output - 127) / 2)) : 
                            ON_PHASE_BASE_PWM);
                        break;

                    case(2): // motor start here
                        uint32_t endOfMotorStartup = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;
                        if (timeRemaining > endOfMotorStartup) {
                            pwm = STARTING_PHASE_PWM;
                        } else {
                            setState(1);
                        }
                        break;

                    case(3): // do nothing
                        break;
                }

            } else { // if not restrained by timeRemaining
                switch(state) {
                    case(0): // while off, not restrained by timeRemaining
                        if (Output > 127) {
                            start();
                        } else {
                            pwm = 0;
                        }
                        break;

                    case(1): // while on
                        if (Output > 127) {
                            pwm = (ON_PHASE_BASE_PWM + ((Output - 127) / 2));
                        } else {    //if Output drops below 0 turn it off
                            stop();
                        }
                    break;

                    case(2): // won't happen
                        break;

                    case(3): // delay timeRemaining is up, check if motor needs to be on
                        set = false;
                        setState(0);
                        break;
                }
            }

            analogWrite(PIN, pwm);
        }
} 
pump = Pump_C();












class Thermostat_C {
    private:
        uint8_t BUTTONS_PIN = A2;
//        Air_C air[4] = {
//            Air_C(&dht[0], "MAIN", 1),
//            Air_C(&dht[1], "UPSTAIRS", 2),
//            Air_C(&dht[2], "OUTSIDE", 2),
//            Air_C(&dht[3], "GREENHOUSE", 2),
//        };
//
    public:
//        uint8_t weightedTemp;
//        
        Thermostat_C() {
            pinMode(BUTTONS_PIN, INPUT);
        }
//        
//        //what can the thermostat do?
//        /**
//         * 
//         * update water?
//         * update setpoint (buttons)
//         * update pages (display pages)
//         * // check for errors in two main sensors
//         */
//
//        lcdUpdate() {
//            lcdCounter--;
//            if (lcdCounter <= 0) { // update LCD
//                lcdCounter = LCD_INTERVAL_SECONDS;
//                
//                lcdPage ++;
//                if (lcdPage >= LCD_PAGE_MAX) { // page 4 == page 0
//                    lcdPage = 0;
//                }
//
//                lcdSwitchPage(); // display page
//            }
//        }
//
        updateSetpoint() { // every 0.25 seconds
           int8_t buttonStatus = 0;
           uint16_t buttonRead = analogRead(BUTTONS_PIN);
           if (buttonRead > 63) {
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
               
               Setpoint += double(0.5 * buttonStatus);
               lcdPageSet();
               
               pump.checkAfter(); // default 3 seconds
           }
        }

        adjustPid() { // every 20ms
            static int PATTERN[5] = {1,2,1,1,2};
            static int pattern[5];
            static int count;
            int reading;
            count++;
            if (count >= 5) count = 0;
            pattern[count] = reading;
        }

        // float avg(int inputVal) {
//   static int arrDat[16];
//   static int pos;
//   static long sum;
//   pos++;
//   if (pos >= 16) pos = 0;
//   sum = sum - arrDat[pos] + inputVal;
//   arrDat[pos] = inputVal;
//   return (float)sum / 16.0;
// }
        
//
//        String readFloors() {
//            for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) { // update floor emas
//                floorSensor[i].update();
//            }
//
//            int difference = int(abs(floorSensor[0].currentEMA - floorSensor[1].currentEMA)); // floor sensors error check
//
//            String err = "";
//            if (difference > 80) { // floor thermistor difference check
//                String err = "FLR ERR " + difference;
//                
//                if (floorSensor[0].currentEMA > floorSensor[1].currentEMA) {
//                    floorSensor[1].currentEMA = floorSensor[0].currentEMA;
//                } else {
//                    floorSensor[0].currentEMA = floorSensor[1].currentEMA;
//                }
//            }
//            floorEmaAvg = (floorSensor[0].currentEMA + floorSensor[1].currentEMA) / FLOOR_SENSOR_COUNT; // avg two readings
//
//            return err;
//        }
//
//        updateTemp() {
//            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
//                air[i].readTemp();
//            }
//
//            String err = errorCheck();
//
//            if (err != "") {
//                lcdPageErr(err);
//            }
//
//            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records
//                air[i].update();
//            }
//
//            weightedTemp = (air[0].WEIGHT * air[0].currentEMA[0] + air[1].WEIGHT * air[1].currentEMA[0]) / (air[0].WEIGHT + air[1].WEIGHT);
//            temperature = weightedTemp;
//
//            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
//                air[i].updateEmaTrend();
//            }
//            avgTrend = (air[0].trendEMA + air[1].trendEMA) / 2.;
//
//            //debugAirEmas();
//
//            if (tempDispCounter2 >= 24) {  // every minute: 
//                //debugEmaWater();
//
//                if (changePerHourMinuteCounter < 59) {
//                    changePerHourMinuteCounter++;
//                }
//                tempDispCounter2 = 0;
//                
//                for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
//                    last59MedEMAs[k] = last59MedEMAs[k+1];
//                }
//                last59MedEMAs[58] = (air[0].currentEMA[1] + air[1].currentEMA[1]) / 2;
//            }
//
//            tempDispCounter2++;
//
//            readFloors();
//        }
//
//        String errorCheck() {
//            String err = "";
//            // check for strange readings (>20 from long avg) on two main air sensors
//            if (air[0].currentEMA[2] != 0) { // don't run the first time 
//                if (air[0].tempF > 20 + air[0].currentEMA[2] || air[0].tempF < -20 + air[0].currentEMA[2]) {
//                    // out of range, set to other sensor (hoping it is within range);
//                    err = "ERR MAIN " + String((air[0].currentEMA[2] - air[0].tempF));
//
//                    air[0].tempF = air[1].tempF;
//                }
//                if (air[1].tempF > 20 + air[1].currentEMA[2] || air[1].tempF < -20 + air[1].currentEMA[2]) {
//                    // out of range, set to other sensor (hoping it is within range);
//                    err = "ERR Upst " + String((air[1].currentEMA[2] - air[1].tempF));
//                        
//                    air[1].tempF = air[0].tempF;
//                }
//            }
//
//
//            if (isnan(air[0].humid) || isnan(air[0].tempF)) { // check for errors in two main sensors
//                
//                if (isnan(air[1].humid) || isnan(air[1].tempF)) {
//                    err = "NaN BOTH ";
//                } else {
//                    if (errorCounter1 == 30) {
//                        err = "NaN MAIN ";
//                        errorCounter1 = 0;
//                    }
//                    air[0].humid = air[1].humid;
//                    air[0].tempF = air[1].tempF;
//                }
//                errorCounter1++;
//            } else {
//                if (isnan(air[1].humid) || isnan(air[1].tempF)) {
//                    if (errorCounter2 == 30) {
//                        err = "NaN UPSTAIRS ";
//                        errorCounter2 = 0;
//                    }
//                    air[1].humid = air[0].humid;
//                    air[1].tempF = air[0].tempF;
//                    errorCounter2++;
//                }
//            }
//            return err;
//        }
//

} thermostat = Thermostat_C();
