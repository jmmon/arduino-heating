//#include <Arduino.h>

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
//  making it easier to tell when filling
//  Too high takes longer to turn off filling icon;
//  oh wait, filling icon should just be turned off from detecting when it gets to FF and was filling; should not depend on this trigger alone

// increase this to allow wider sensitivity for FILL_TRIGGER
const uint8_t EMA_PERIODS_LONG = 160; //  * 2.5   400 sec

// This is the EMA for our difference between the above EMAs
//  should be ??
const uint8_t EMA_PERIODS_DIFF = 10; //  * 2.5 = 50 s

// higher makes it take longer to trigger ON filling
//  difference(EMA) must be > this to trigger filling, should be high
//  enough to not trigger randomly when not filling, but not too high
const uint8_t FILL_TRIGGER = 44; // average variation can reach over 22 so this must be higher by a margin

const uint8_t STOP_FILL_TRIGGER = 30; // lower makes it harder to trigger OFF filling

// bounds for reading from float sensor
// At ~0%, the tank seems to bounce between 232~197
// Can measure different fill points as I fill (once the hose thaws!)
// 780 - 250 = 530
// 530 / 100 == 5.3 units per percent
// so 50% should be 5.3 * 50 + 250 == 515
const uint16_t LOW_BOUND = 250;
const uint16_t HIGH_BOUND = 780;
const uint8_t ERROR_HIGH_BOUND = 190;

class WaterTank_C
{
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

	// Special values:
	// 0 == "EE" // empty
	// 100 == "FF" // full
	// 101 == "ER" // error
	// 255 || -1 == "--"; // initial state
	uint8_t percent, lastPercent, displayPercent = 255; // initialize as '--'

	WaterTank_C()
	{ // constructor
		pinMode(WATER_FLOAT_SENSOR_PIN, INPUT);
	}

	init()
	{ // run at setup
		ema = lastEma = slowEma = lastSlowEma = analogRead(WATER_FLOAT_SENSOR_PIN);
	}

	uint8_t calculateTankPercent()
	{
		return ((ema <= ERROR_HIGH_BOUND) ? 101 : // error (lowest)
								(ema <= LOW_BOUND) ? 0
																	 : // empty
								(ema <= HIGH_BOUND) ? (float)100 * (ema - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND)
																		: // normal range
								100);									// full
	}

	update()
	{																																			// every 2.5 seconds
		uint16_t floatSensorReadValue = analogRead(WATER_FLOAT_SENSOR_PIN); // float sensor

		// calculate water level EMAs
		lastEma = ema;
		lastSlowEma = slowEma;
		lastDiff = diff;
		lastDiffEma = diffEma;

		// EMA calcs
		ema = calcEma(floatSensorReadValue, lastEma, EMA_PERIODS_SHORT);
		slowEma = calcEma(floatSensorReadValue, lastSlowEma, EMA_PERIODS_LONG);
		diff = ema - slowEma;
		diffEma = calcEma(diff, lastDiffEma, EMA_PERIODS_DIFF);

		// calculate tank percent based on ema
		lastPercent = percent;
		percent = calculateTankPercent();

		// set to percent instead of '--' (initialization)
		bool isInitialization = displayPercent == 255;
		if (isInitialization)
			displayPercent = (displayPercent != percent) ? percent : displayPercent;

		// stabilize readings: stops wobble, i.e. 55 -> 54 -> 55 -> 54 -> 55
		bool isFillingAndGreater = filling && percent > displayPercent;
		bool isDrainingAndLesser = !filling && percent < displayPercent;
		// when not filling, allow it to decrease
		// when filling, allow it to increase
		if ((isFillingAndGreater) || (isDrainingAndLesser))
			displayPercent = percent;

		bool isTankRising = diffEma >= FILL_TRIGGER;
		bool isFull = percent == 100;

		// // water flow reset
		// if (
		// 	filling &&
		// 	isFull &&
		// 	isTankRising)
		// {
		// 	waterResetTotal();
		// }

		// Determine if tank is currently being filled:

		// while tank is rising, adjust filling boolean based on if tank is at 100%
		if (isTankRising)
		{
			filling = (percent != 100);
		}
		else
		{
			bool shouldStopRising = diffEma <= STOP_FILL_TRIGGER;
			if (shouldStopRising)
				filling = false;
		}
		// detect moment the tank fills to 100%
		if (filling && (percent == 100))
		{
			filling = false;
			newlyFull = true;

			// pin our tank reading to 100% for the moment
			ema = lastEma = slowEma = lastSlowEma = HIGH_BOUND; // set to max
			diffEma = lastDiffEma = 0;													// reset to 0
		}
	}

} waterTank = WaterTank_C();
