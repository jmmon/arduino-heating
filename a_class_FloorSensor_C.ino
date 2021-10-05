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