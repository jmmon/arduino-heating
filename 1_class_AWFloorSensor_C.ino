
const float FLOOR_EMA_MULT = 2. / (1 + 10); // 10 readings EMA (20s)
const float FLOOR_EMA_MULT_SLOW = 2. / (1 + 300); // 300 readings EMA (600s)

class FloorSensor_C {
	private:
		uint8_t PIN;

	public:
		float ema = 0;
		float lastEma = 0;

		float slowEma = 0;
		float lastSlowEma = 0;

		FloorSensor_C(uint8_t _pin) { // constructor
			PIN = _pin;
			pinMode(_pin, INPUT);
		}

		uint16_t readTemp(uint8_t loops = 10) {
			uint32_t val = 0;
			for (uint8_t z = 0; z < loops; z++) {
				val += analogRead(PIN);
			}
			return val / loops;
		}

		update() {
			uint16_t floorRead = readTemp();
			if (lastEma == 0) { // initialize
				lastEma = floorRead;
				lastSlowEma = lastEma;
			}
			// save old emas
			lastEma = ema;
			lastSlowEma = slowEma;
			// calc new emas
			ema = (floorRead * FLOOR_EMA_MULT + lastEma * (1 - FLOOR_EMA_MULT));
			slowEma = (floorRead * FLOOR_EMA_MULT_SLOW + lastSlowEma * (1 - FLOOR_EMA_MULT_SLOW));
		}

} 
floorSensor[] = { 
    FloorSensor_C(FLOOR_TEMP_PIN[0]),
    FloorSensor_C(FLOOR_TEMP_PIN[1])
};