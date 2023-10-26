// water level sensor input: 5v

// float sensor has ohm output: 240 ohm empty ~~~ 30 ohm full (++ wire resistance) (off of 5v)
//                              low return voltage ~~ high return voltage.
// approximately 22 steps on a 17" sensor; one step every 18-21mm
// 780 high bound - 350 low bound = 430 working range
// 430 / 22 steps =~ 19.54 units per step
// 100% / 22 = 4.5454 percent fill per step

// pin for float sensor, with 100 ohm pull-down resistor. // WATER_FLOAT_SENSOR_PIN

// 5v > wtrLvl (240~30 ohm) > (long wire with resistance) >  pin > pull-down to grnd with 100ohm resistor

// 5v > 30 ohm > long wire > pin (> pull-down)
// 5v > 240 ohm > long wire > pin (> pull-down)
// what happens if I change the pulldown? ??

// if wire resistance is reduced, that means total resistance would be reduced on high and low end, probably won't increase "resolution"

// 1/100 + 1/30 =   0.04333333 full resistance (not really!)
// 1/100 + 1/240 =  0.01416667 empty resistance

// This is the EMA which is displayed as the level
//  higher is smoother but slower response so less accurate
const uint8_t EMA_PERIODS_SHORT = 20; // * 2.5s per period = 50 s

// This is a slow EMA for comparison against the first one
//  to detect when tank is being filled. Higher means slow response,
//  making it easier to tell when isFilling
//  Too high takes longer to turn off isFilling icon;
//  oh wait, isFilling icon should just be turned off from detecting when it gets to FF and was isFilling; should not depend on this trigger alone

// increase this to allow wider sensitivity for FILL_TRIGGER
const uint8_t EMA_PERIODS_LONG = 160; //  * 2.5   400 sec

// This is the EMA for our difference between the above EMAs
//  should be ??
const uint8_t EMA_PERIODS_DIFF = 10; //  * 2.5 = 50 s

// higher makes it take longer to trigger ON isFilling
//  difference(EMA) must be > this to trigger isFilling, should be high
//  enough to not trigger randomly when not isFilling, but not too high
const uint8_t FILL_TRIGGER = 44; // average variation can reach over 22 so this must be higher by a margin

const uint8_t STOP_FILL_TRIGGER = 30; // lower makes it harder to trigger OFF isFilling

// bounds for reading from float sensor
// At ~0%, the tank seems to bounce between 232~197
// Can measure different fill points as I fill (once the hose thaws!)
// 780 - 250 = 530
// 530 / 100 == 5.3 units per percent
// so 50% should be 5.3 * 50 + 250 == 515
const uint16_t LOW_BOUND = 300;
const uint16_t HIGH_BOUND = 750;
const uint8_t ERROR_HIGH_BOUND = 190;

/* ==========================================================================
 * WaterTank class
 * ========================================================================== */
class WaterTank_C {
public:
	bool isFilling = false;
	bool isNewlyFull = false;

	uint16_t ema = 0;
	uint16_t lastEma = 0;

	uint16_t slowEma = 0;
	uint16_t lastSlowEma = 0;

	int16_t diff = 0;
	int16_t lastDiff = 0;

	int16_t diffEma = 0;
	int16_t lastDiffEma = 0;

	// Special values:
	// 0 == "EE" // empty
	// 100 == "FF" // full
	// 101 == "ER" // error
	// 255 || -1 == "--"; // initial state
	uint8_t percent, lastPercent, displayPercent = 255; // initialize as '--'

  /**
   * Constructor fn
   * */
	WaterTank_C() {
		pinMode(WATER_FLOAT_SENSOR_PIN, INPUT);
	}

  /**
   * Read tank sensor on init
   * */
	void init() {
		ema = lastEma = slowEma = lastSlowEma = analogRead(WATER_FLOAT_SENSOR_PIN);
	}

  /**
   * helper to translate percent into display string
   * @return {String} - 2 character string
   * */
	String getDisplayString() {
		// handle conversion / formatting:
		switch (displayPercent) {
		case (100):
			return F("FF");
		case (0):
		  return F("EE");
		case (101):
			return F("ER");
		case (255):
			return F("--");
		default:
			return (displayPercent < 10) ? (" ") + String(displayPercent) : String(displayPercent);
		}
	}

  /**
   * Check EMA against our bounds and calculate the tank percent
   * @return {uint8_t} - percent as number
   * */
	uint8_t getCalculatedTankPercent() {
		return ((ema <= ERROR_HIGH_BOUND) 
      ? 101 // error (lowest)
      : (ema <= LOW_BOUND) 
        ? 0 // empty
        : (ema <= HIGH_BOUND) 
          ? (float)100 * (ema - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND) // normal range
          : 100 // full
    );
	}

  /**
   * calculate EMAs (tank percent), update isFilling and isNewlyFull
   * runs every 2.5s
   * */
	update() {
		uint16_t floatSensorReadValue = analogRead(WATER_FLOAT_SENSOR_PIN); // float sensor

		// calculate water level EMAs
		lastEma = ema;
		lastSlowEma = slowEma;
		lastDiff = diff;
		lastDiffEma = diffEma;

    float asFloat_FloatSensorReadValue = float(floatSensorReadValue);

		// EMA calcs
		// ema = calcEMAFromFloats(floatSensorReadValue, lastEma, EMA_PERIODS_SHORT);
		// slowEma = calcEMAFromFloats(floatSensorReadValue, lastSlowEma, EMA_PERIODS_LONG);
		ema = calcEma(floatSensorReadValue, lastEma, EMA_PERIODS_SHORT);
		slowEma = calcEma(floatSensorReadValue, lastSlowEma, EMA_PERIODS_LONG);

		diff = ema - slowEma;
		// diffEma = calcEMAFromFloats(diff, lastDiffEma, EMA_PERIODS_DIFF);
		diffEma = calcEma(diff, lastDiffEma, EMA_PERIODS_DIFF);

		// calculate tank percent based on ema
		lastPercent = percent;
		percent = getCalculatedTankPercent();

		// set to percent instead of '--' (initialization)
		bool isInitialization = displayPercent == 255;
		if (isInitialization) {
      displayPercent = (displayPercent != percent) ? percent : displayPercent;
    }

		// stabilize readings: stops wobble, i.e. 55 -> 54 -> 55 -> 54 -> 55
		bool isFillingAndGreater = isFilling && percent > displayPercent;
		bool isDrainingAndLesser = !isFilling && percent < displayPercent;
		// when not isFilling, allow it to decrease
		// when isFilling, allow it to increase
		if ((isFillingAndGreater) || (isDrainingAndLesser)) {
      displayPercent = percent;
    }

		bool isTankRising = diffEma >= FILL_TRIGGER;
		bool isFull = percent == 100;

		// // water flow reset
		// if (
		// 	isFilling &&
		// 	isFull &&
		// 	isTankRising)
		// {
		// 	waterResetTotal();
		// }

		// Determine if tank is currently being filled:

		// while tank is rising, adjust isFilling boolean based on if tank is at 100%
		if (isTankRising) {
			isFilling = (percent != 100);
		}	else {
			bool shouldStopRising = diffEma <= STOP_FILL_TRIGGER;
      // if should stop rising
			if (shouldStopRising) {
        isFilling = false;
      }
		}

		// detect moment the tank fills to 100%
		if (isFilling && (percent == 100)) {
			isFilling = false;
			isNewlyFull = true;

			// pin our tank reading to 100% for the moment
			ema = lastEma = slowEma = lastSlowEma = HIGH_BOUND; // set to max
			diffEma = lastDiffEma = 0;													// reset to 0
		}
	}

} waterTank = WaterTank_C();
