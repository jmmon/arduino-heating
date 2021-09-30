class Air_C {
    public:
        Air_C(DHT *s, String l, float w) {
            sensor = s;
            label = l;
            WEIGHT = w;
        }

        DHT *sensor;
        float tempC = 0;
        float tempF = 0;
        float WEIGHT = 1;
        float humid = 0;

        float highest = -1000;
        float lowest = 1000;
        float currentEMA[3]; // {short, medium, long} EMA
        float lastEMA[3];    // {short, medium, long}
        int16_t trendEMA = 0;
        
        bool working = true;
        String label;

        readTemp() {
            //read temp/humid
            tempC = sensor->readTemperature();
            tempF = sensor->readTemperature(true);
            humid = sensor->readHumidity();
        };

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

        updateEmaTrend() {
            if (currentEMA[0] < currentEMA[1]) { // if (now) temp < (medium avg) temp; temp is falling: 
                if (currentEMA[1] < weightedTemp) { // if medium 
                    if (trendEMA <= -DOWNTREND) {
                        trendEMA += DOWNTREND; // fast going back to 0
                    } else {
                        trendEMA = 0;
                    }
                } else {
                    //                if (currentEMA[1] - currentEMA[0] < 0.5) {  //if difference is small, this means temp is falling fast, so we'll up the trend quicker so pump turns on sooner.
                    //                    trendEMA += -UPTREND * 2;    //slow going down
                    //                } else {
                    //                    trendEMA += -UPTREND;    //slow going down     //if difference is large, this means temp is falling slowly, so we don't need the pump to come on as soon.
                    //                }
                    trendEMA += -UPTREND; //slow going down
                }
            }

            if (currentEMA[0] > currentEMA[1]) { // if rising
                if (currentEMA[1] > weightedTemp) {
                    if (trendEMA >= DOWNTREND) {
                        trendEMA += -DOWNTREND; // fast going back to 0
                    } else {
                        trendEMA = 0;
                    }
                } else {
                    //                if (currentEMA[0] - currentEMA[1] < 0.5) {  //if difference is small, this means temp is rising fast, so we'll up the trend quicker so pump shuts off sooner.
                    //                    trendEMA += UPTREND * 2;    //slow going up
                    //                } else {
                    //                    trendEMA += UPTREND;    //slow going up      //if difference is large, this means temp is rising slow, so we don't want the pump to shut off as soon.
                    //                }
                    trendEMA += UPTREND; //slow going up
                }
            }

            if (trendEMA > AIR_TEMP_TREND_MINMAX) {
                trendEMA = AIR_TEMP_TREND_MINMAX;
            }
            if (trendEMA < -AIR_TEMP_TREND_MINMAX) {
                trendEMA = -AIR_TEMP_TREND_MINMAX;
            }
        };


}
air[] = {
    Air_C(&dht[0], "MAIN", 1),
    Air_C(&dht[0], "UPSTAIRS", 2),
    Air_C(&dht[0], "OUTSIDE", 2),
    Air_C(&dht[0], "GREENHOUSE", 2),
};

















class FloorSensor_C {
    public:
        FloorSensor_C(uint8_t pin) { // constructor
            PIN = pin;
            pinMode(pin, INPUT);
        };
        uint8_t PIN;
        float currentEMA = 0;
        float lastEMA = 0;

        readTemp(int a = 10) {
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
    public:
        Pump_C (uint8_t pin) {
            PIN = pin;
            pinMode(pin, OUTPUT);
        }
        uint8_t PIN;
        uint8_t pwm = 0;
        uint8_t state = 0;
        uint16_t timer = 0;

        //methods:
        //update
        //or: [start, low, med, high, off, continue]

        start() {
            state = 1;
            timer = 180; // 3 minute minimum
            pwm = 255;
            analogWrite(PIN, pwm); // run immediately for hard start
        }


        stop() {
            state = 0;
            timer = 300; // 5 minute minimum
            pwm = 0;
            analogWrite(PIN, pwm);
        }


        update() { // called every second
            if (timer > 0) {
                timer --;
            }

            if (timer > 0) { // if restrained by timer
                if (state == 1) { // if on
                    // keep minimum pwm 
                    if (outputVal > 0) {
                        analogWrite(PIN, (198 + outputVal)); ;
                    } else {
                        analogWrite(PIN, 199) ; // minimum pwm
                    }
                } else {
                    // is off
                    if (outputVal == 57) { // if max, let's turn it on despite the timer
                        // special case
                        start();
                    } else {
                        analogWrite(PIN, 0);
                    }
                }

            } else { // if not restrained by timer
                if (state == 1) {
                    if (outputVal > 0) {
                        analogWrite(PIN, (198 + outputVal)) ;
                    } else {    //if outputVal drops below 0 turn it off
                        stop();
                    }
                } else { //from off, not restrained by timer
                    if (outputVal > 0) {
                        start();
                    } else {    // 
                        analogWrite(PIN, 0);
                    }
                }
            }

        }
} pump = Pump_C(PUMP_PIN);


















class Thermostat_C {
    public:
        Thermostat_C() {

        }
} thermostat = Thermostat_C();