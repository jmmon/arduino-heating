class WaterTank_C {
	private: 
		// input: 5v

		// float sensor has ohm output: 240 ohm empty ~~~ 30 ohm full
		//                              low return voltage ~~ high return voltage.
		// approximately 22 steps on a 17" sensor; one step every 18-21mm
		// 780 high bound - 350 low bound = 430 working range
		// 430 / 22 steps =~ 19.54 units per step
		// 100% / 22 = 4.5454 percent fill per step




		// pin for float sensor, with 100 ohm resistor to ground.
		const uint8_t PIN = WATER_FLOAT_SENSOR_PIN;

		// 5v > wtrLvl (240~30 ohm) > (long wire with resistance) >  pin > pull-down to grnd with 100ohm resistor

		// 5v > 30 ohm > long wire > pin (> pull-down)
		// 5v > 240 ohm > long wire > pin (> pull-down)


		// 1/100 + 1/30 =   0.04333333 full resistance
		// 1/100 + 1/240 =  0.01416667 empty resistance

		// This is the EMA which is displayed as the level
		//  higher is smoother but slower response so less accurate
		const uint8_t EMA_PERIODS_SHORT = 20; // * 2.5s per period = 50 s

		// This is a slow EMA for comparison against the first one
		//  to detect when tank is being filled. Higher means slow response,
		//  making it easier to tell when filling
		//  Too high takes longer to turn off filling icon;
		//  oh wait, filling icon should just be turned off from detecting when it gets to FF and was filling; should not depend on this trigger alone

		// increase this to allow wider sensitivity for FILL_TRIGGER
		const uint8_t EMA_PERIODS_LONG = 160; //  * 2.5   400 sec

		// This is the EMA for our difference between the above EMAs
		//  should be ??
		const uint8_t EMA_PERIODS_DIFF = 20; //  * 2.5 = 50 s

		// higher makes it take longer to trigger ON filling
		//  difference(EMA) must be > this to trigger filling, should be high
		//  enough to not trigger randomly when not filling, but not too high
		const uint8_t FILL_TRIGGER = 44; // average variation can reach over 22 so this must be higher by a margin

		const uint8_t STOP_FILL_TRIGGER = 30; // lower makes it harder to trigger OFF filling

		

		// bounds for reading from float sensor
		const uint16_t LOW_BOUND = 350;
		const uint16_t HIGH_BOUND = 780;
		const uint8_t ERROR_HIGH_BOUND = 250;
			
	public:
		bool filling = false;
		bool newlyFull = false;

		uint16_t ema = 0;
		uint16_t lastEma = 0;

		uint16_t slowEma = 0;
		uint16_t lastSlowEma = 0;

		int16_t diff = 0;
		int16_t lastDiff = 0;

		int16_t diffEma = 0;
		int16_t lastDiffEma = 0;

		// struct tankLevel_s {
		// 	uint16_t ema = 0;
		// 	uint16_t lastEma = 0;

		// 	uint16_t slowEma = 0;
		// 	uint16_t lastSlowEma = 0;

		// 	uint16_t diff = 0;
		// 	uint16_t lastDiff = 0;

		// 	uint16_t diffEma = 0;
		// 	uint16_t lastDiffEma = 0;
		// } tankLevel ;

		// Special values:
		// 0 == "EE"
		// 100 == "FF"
		// 101 == "ER"
		// 255 || -1 == "--";
		uint8_t percent = 255; // initialize as '--'
		uint8_t lastPercent = 255;
		uint8_t displayPercent = 255;

		WaterTank_C() {
			pinMode(PIN, INPUT);
		}

		init() {
			ema = lastEma = slowEma = lastSlowEma = analogRead(PIN);
			diffEma = lastDiffEma = 0;
		}
		
		uint8_t calculateTankPercent() {    
			if (ema <= ERROR_HIGH_BOUND) { // error (lowest)
				return 101;
			} else if (ema <= LOW_BOUND) { // empty
				return 0;
			} else if (ema <= HIGH_BOUND) { // normal range
				return (float) 100 * (ema - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND);
			}
			return 100; // full
		}

		
		update() { // every 2.5 seconds
			uint16_t readPin = analogRead(PIN); // float sensor

			// calculate water level EMAs
			lastEma = ema;
			ema = (readPin * (2. / (1 + EMA_PERIODS_SHORT )) + lastEma * (1 - (2. / (1 + EMA_PERIODS_SHORT ))));

			lastSlowEma = slowEma;
			slowEma = (readPin * (2. / (1 + EMA_PERIODS_LONG )) + lastSlowEma * (1 - (2. / (1 + EMA_PERIODS_LONG ))));

			// calculate difference and ema
			lastDiff = diff;
			diff = ema - slowEma;
			
			lastDiffEma = diffEma;
			diffEma = (diff * (2. / (1 + EMA_PERIODS_DIFF )) + lastDiffEma * (1 - (2. / (1 + EMA_PERIODS_DIFF ))));
			

			// calculate tank percent
			lastPercent = percent;
			percent = calculateTankPercent();

			if ( displayPercent == 255 ) {  // on init ('--')
				displayPercent = (displayPercent != percent) ? percent : displayPercent;
			}

			// stabilize percent reading; only allow increase when increasing; only allow decrease when decreasing;
			// ( stops wobble readings i.e. 55 -> 54 -> 55 -> 54 -> 55 )
			if (
				( filling && percent > displayPercent ) || 
				( !filling && percent < displayPercent )
			) {
				displayPercent = percent;
			}



			// water flow reset
			if (
				filling && 
				percent == 100 && 
				diffEma >= FILL_TRIGGER
			) {
				waterResetTotal();
			}
			
			
			// Determine if tank is currently being filled:

			// while tank is rising, adjust filling boolean based on if tank is at 100%
			if (diffEma >= FILL_TRIGGER) {
				filling = (percent == 100) ? false : true;
			}

			// while filling and hits 100%, turn off our filling indicator
			if (filling && (percent == 100)) {
				filling = false;
				newlyFull = true;

				// pin our tank reading to 100% for the moment
				ema = lastEma = slowEma = lastSlowEma = HIGH_BOUND; // set to max
				diffEma = lastDiffEma = 0; // reset to 0
			}

			// if current read is around longerEMAread, not filling. 
			if (diffEma <= STOP_FILL_TRIGGER) filling = false;
		}

} waterTank = WaterTank_C();
