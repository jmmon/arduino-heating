#include <Arduino.h>

const uint16_t FLOOR_EMA_DAYS = 10; // 10 readings EMA (20s)
const uint16_t FLOOR_EMA_DAYS_SLOW = 300; // 300 readings EMA (600s)

class FloorSensor_C {
	private:
		uint8_t PIN;

	public:
		uint16_t ema = 0;
		uint16_t lastEma = 0;

		uint16_t slowEma = 0;
		uint16_t lastSlowEma = 0;

		FloorSensor_C(uint8_t _pin) { // constructor
			PIN = _pin;
			pinMode(_pin, INPUT);
		}

		uint16_t readTemp(uint8_t loops = 10) {
			uint32_t val = 0;
			
			for (uint8_t z = 0; z < loops; z++) val += analogRead(PIN);
			
			return val / loops;
		}

		update() {
			uint16_t floorRead = readTemp();
			bool isInitialization = lastEma == 0;
			if (isInitialization) { // initialize
				lastEma = floorRead;
				lastSlowEma = lastEma;
			}
			// save old emas
			lastEma = ema;
			lastSlowEma = slowEma;
			// calc new emas
			ema = calcEma(floorRead, lastEma, FLOOR_EMA_DAYS);
			slowEma = calcEma(floorRead, lastSlowEma, FLOOR_EMA_DAYS_SLOW);
		}

} 
floorSensor[] = { 
    FloorSensor_C(FLOOR_TEMP_PIN[0]),
    FloorSensor_C(FLOOR_TEMP_PIN[1])
};