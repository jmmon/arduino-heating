// EMAs are calculated every 2.5 seconds!
const uint8_t FLOOR_EMA_DAYS = 12;				// 12 readings EMA (30s)
const uint16_t FLOOR_EMA_DAYS_SLOW = 60; // 60 readings EMA (150s == 2.5min)

/* ==========================================================================
 * FloorSensor class
 * ========================================================================== */
class FloorSensor_C {
private:
	uint8_t PIN;

public:
	uint16_t ema = 480; // approximate cold floor reading
	uint16_t slowEma = 480;

	uint16_t lastEma = 480;
	uint16_t lastSlowEma = 480;

	FloorSensor_C(uint8_t _pin) { // constructor
		PIN = _pin;
		pinMode(_pin, INPUT);
	}

  uint16_t readTemp(uint8_t loops = 5) {
		uint16_t val = 0;

		for (uint8_t z = 0; z < loops; z++)
			val += analogRead(PIN);

		return val / loops;
	}

	void update() {
		uint16_t floorRead = readTemp();

		// save old emas
		lastEma = ema;
		lastSlowEma = slowEma;

    float asFloat_floorRead = float(floorRead);

		// calc new emas
		// ema = uint16_t(calcEMAFromFloats(floorRead, lastEma, FLOOR_EMA_DAYS));
		// slowEma = uint16_t(calcEMAFromFloats(floorRead, lastSlowEma, FLOOR_EMA_DAYS_SLOW));
		ema = uint16_t(calcEma(floorRead, lastEma, FLOOR_EMA_DAYS));
		slowEma = uint16_t(calcEma(floorRead, lastSlowEma, FLOOR_EMA_DAYS_SLOW));
	}

} floorSensor[] = {
		FloorSensor_C(FLOOR_TEMP_PIN[0]),
		FloorSensor_C(FLOOR_TEMP_PIN[1])
};
