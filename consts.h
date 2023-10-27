const String VERSION_NUMBER = "4.1.2";
const bool DEBUG = true;

const uint8_t AIR_SENSOR_COUNT = 4;
const uint8_t FLOOR_SENSOR_COUNT = 2;

// PINS
// NANO PWM PINS: (digital)
// (3), (5), 6, 9, 10, 11
const uint8_t WATER_FLOAT_SENSOR_PIN = A3;
const uint8_t FLOOR_TEMP_PIN[2] = {A0, A1};
const uint8_t HEAT_PUMP_PIN = 5;
const uint8_t T_STAT_BUTTON_PIN = A2;

const uint8_t WATER_VALVE_PIN;

const uint8_t WATER_FLOW_PIN = 3; // flow sensor
uint8_t WATER_FLOW_INTERRUPT = 0; // set in setup as interrupt of above pin

const uint8_t TONE_PIN = 6;

const uint8_t DHT_PIN[4] = {
		7, // main
		8, // upstairs
		4, // one gh, one outside
		12,
};
DHT dht[4] = {
		{DHT_PIN[0], DHT22},
		{DHT_PIN[1], DHT22},
		{DHT_PIN[2], DHT22}, //  outside, not yet installed // nan
		{DHT_PIN[3], DHT22}, // greenhouse? // nan
};

//
//// set DHTNEW pins
// DHTNEW downstairs(DHT_PIN[0]);
// DHTNEW upstairs(DHT_PIN[1]);
// DHTNEW outside(DHT_PIN[2]);
// DHTNEW greenhouse(DHT_PIN[3]);
//// build array to access the sensors
// DHTNEW  dhtnew[4] = {downstairs, upstairs, outside, greenhouse};

uint32_t currentTime = 0; // timer
uint32_t prevLoopStartTime = 0;		// counters
const uint16_t MS_1000_INTERVAL = 1000;
const uint16_t MS_2500_INTERVAL = 2500;

// SDA = A4 pin; SCL = A5 pin
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

//  2 / (1 + n)   =>  where (n * TEMPERATURE_READ_INTERVAL) = milliseconds defining EMA calculation
const uint16_t EMA_MULT_PERIODS[3] = {
	12, 
  600, 
  1800
};
// 12 * 2.5s = 30s   EMA =   0.5m
// 600 * 2.5s = 1500s EMA =  25m
// 1800 * 2.5s = 4500s EMA = 75m
// so if x == EMA(600) - EMA(12) and y == EMA(1800) - EMA(600), we would expect y > x because x is over a smaller range.
// And so if x > y, we know the temp is increasing rapidly

const uint16_t FLOOR_WARMUP_TEMPERATURE = 550; // 0-1023, NOTE: insulation (cardboard, rugs) will require higher value
float floorEmaAvg = 0;
float floorEmaAvgSlow = 0;
bool coldFloor = false;

// for tweaking the setpoint based on the floor reading
const uint16_t MIN_FLOOR_READ = 525; // the 0 point for this calculation
const uint8_t MAX_FLOOR_OFFSET = 150; // added to above, the maximum for this calculation
const uint8_t MAX_TEMP_ADJUST_100X = 200; // degrees F * 100 to subtract from target temp, scaled by the floor read range
double floorOffset = 0;

// for the lcd? or for debug?
uint8_t tempDispCounter = 5;
uint8_t tempDispCounter2 = 0;
uint8_t errorCounter1 = 5;
uint8_t errorCounter2 = 5;

// water counter stuff, not working
float calibrationFactor = 4.5;

volatile byte waterPulseCount = 0;

float flowRate = 0;
float flowMilliLitres = 0;
float totalMilliLitres = 0;

unsigned long oldTime = 0;

float l_minute = 0;
float totalVolume = 0;

// Time:

time_t t = now();

// *******************************************************
/**
 * AutoPID
 * if output changes slowly compared to temp/input, start with higher gain(KP)[2-8] and lower resets(KI)[0.05-0.5]
 *
 * @brief KP // how fast system responds
 *  WAS: 0.012
 *  start with KP, adjust until temp oscillates, then cut in half.
 *
 *
 * @brief KI // "minutes per repeat" / "repeats per minute" in oscillation
 *  next is KI, reaction time. Adjust higher or lower as long as it increases stability, and change the target temperature each time to study reaction time.
 *
 *
 * @brief KD // seconds or minutes, requires clean input signal (minimal noise)
 *  finally KD monitors ramp time, and increase this to reduce overshoot, or reduce to increase stability.
 *  Plans for the future, so a higher number will make the pump shut off sooner *before* the setpoint is reached.
 *  I assume it will also turn *on* the pump *before* the temperature drops past the setpoint.
 */

// const double outputMin = -128;
// const double outputMax = 127;
// const double MIDPOINT = 0; // greater than this == on

// might want the pump to kick on a little sooner, perhaps depending on the floor temperature. So it should basically increase slightly as the time rolls on;
// Proportional
// double Kp = 8;
// Integral (testing 0.05 [was 0.005] starting jan 21 2022)
// double Ki = 0.05; // 0.005
// Derivative
// double Kd = 0.0; // 0.002

double weightedAirTemp;
double setPoint = 67;
// double Output;

// AutoPID myPID(&weightedAirTemp, &setPoint, &Output, outputMin, outputMax, Kp, Ki, Kd);

// ArduPID ArduPIDController;

// move this to functions?
// float calcEma(uint16_t reading, uint16_t lastEma, uint16_t _days)
// {
//   return (reading * (2. / (1 + _days)) + lastEma * (1 - (2. / (1 + _days))));
// }

