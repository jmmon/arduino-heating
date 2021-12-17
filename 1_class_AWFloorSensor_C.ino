class FloorSensor_C {
    private:
        uint8_t PIN;

    public:
        float ema = 0;
        float lastEma = 0;

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
            if (lastEma == 0) {
                lastEma = readTemp();
            }
            ema = (readTemp() * FLOOR_EMA_MULT + lastEma * (1 - FLOOR_EMA_MULT));
            lastEma = ema;

        };
} 
floorSensor[] = { 
    FloorSensor_C(FLOOR_TEMP_PIN[0]),
    FloorSensor_C(FLOOR_TEMP_PIN[1])
};