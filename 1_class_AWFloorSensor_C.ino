class FloorSensor_C {
    private:
        uint8_t PIN;
        const float FLOOR_EMA_MULT = 2. / (1 + 10); // 10 readings EMA (20s)
        const float FLOOR_EMA_MULT_SLOW = 2. / (1 + 300); // 300 readings EMA (600s)

    public:
        float ema = 0;
        float lastEma = 0;

        float slowEma = 0;
        float lastSlowEma = 0;

        FloorSensor_C(uint8_t pin) { // constructor
            PIN = pin;
            pinMode(pin, INPUT);
        };

        uint16_t readTemp(int a = 10) {
            int val = 0;
            for (int z = 0; z < a; z++) {
                val += analogRead(PIN);
            }
            return val / a;
        };

        update() {
            uint16_t floorRead = readTemp();
            if (lastEma == 0) { // initialize
                lastEma = floorRead;
                lastSlowEma = lastEma;
            }
            ema = (floorRead * FLOOR_EMA_MULT + lastEma * (1 - FLOOR_EMA_MULT));
            lastEma = ema;

            slowEma = (floorRead * FLOOR_EMA_MULT_SLOW + lastSlowEma * (1 - FLOOR_EMA_MULT_SLOW));
            lastSlowEma = slowEma;

        };
} 
floorSensor[] = { 
    FloorSensor_C(FLOOR_TEMP_PIN[0]),
    FloorSensor_C(FLOOR_TEMP_PIN[1])
};