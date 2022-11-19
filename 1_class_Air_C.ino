class Air_C {
    private:
        DHT *sensor;
//         DHTNEW *sensor;

    public:
        float tempC = 0;
        float tempF = 0;
        float WEIGHT = 1;
        float humid = 0;

        float highest = -1000;
        float lowest = 1000;
        float currentEMA[3]; // {short, medium, long} EMA
        float lastEMA[3];    // {short, medium, long}
        
        bool working = true;
        
        Air_C(DHT *s, float w) {  //constructor
            sensor = s;
            WEIGHT = w;
        }

//        Air_C(DHTNEW *s, String l, float w) {  //constructor
//            sensor = s;
//            label = l;
//            WEIGHT = w;
//        }

        readTemp() {
            //read temp/humid
            tempC = sensor->readTemperature();
            tempF = sensor->readTemperature(true);
            humid = sensor->readHumidity();
        };

//        readTemp() {
////          uint32_t start = micros();
////          int chk = sensor.read();
////          uint32_t stop = micros();
//
//          sensor->read();
//          tempC = sensor->getTemperature();
//          tempF = (tempC * 9 / 5) + 32;
//          humid = sensor->getHumidity();
//        }

        float getTempEma(bool weighted = false) {
            if (weighted) return (WEIGHT * currentEMA[0]);
            return currentEMA[0];
        }

        updateRecords() {
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
    Air_C(&dht[0], 1), //"MAIN"
    Air_C(&dht[1], 2), //"UPSTAIRS"
    Air_C(&dht[2], 2), //"OUTSIDE"
    Air_C(&dht[3], 2), //"GREENHOUSE"
    
//    Air_C(&dhtnew[0], "MAIN", 1),
//    Air_C(&dhtnew[1], "UPSTAIRS", 2),
//    Air_C(&dhtnew[2], "OUTSIDE", 2),
//    Air_C(&dhtnew[3], "GREENHOUSE", 2),
};
