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