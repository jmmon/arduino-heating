#include <Arduino.h>
#include <TimeLib.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// #include <AutoPID.h>
#include <DHT.h>
// #include <dhtnew.h>
#include <ArduPID.h>

#define TONE_USE_INT
#define TONE_PITCH 432
#include <TonePitch.h>


const String VERSION_NUMBER = "4.1.2";
const bool DEBUG = false;

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
uint32_t last250ms = 0;		// counters
uint8_t ms1000ctr = 0;		// or use %????
uint8_t ms2500ctr = 0;

// new timers counter, 20ms base for quick display-screen switching
// uint32_t last20ms = 0;

// SDA = A4 pin; SCL = A5 pin
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

const uint16_t EMA_MULT_PERIODS[3] = {
		12, 600, 1800};

const float EMA_MULT[3] = {
		//  2 / (1 + n)   =>  where (n * TEMPERATURE_READ_INTERVAL) = milliseconds defining EMA calculation
		2. / (1 + 12),	 // 12 * 2.5s = 30s   EMA =   0.5m
		2. / (1 + 600),	 // 600 * 2.5s = 1500s EMA =  25m
		2. / (1 + 1800), // 1800 * 2.5s = 4500s EMA = 75m
};

const float FLOOR_WARMUP_TEMPERATURE = 580; // 0-1023, NOTE: insulation (cardboard, rugs) will require higher value
float floorEmaAvg;
float floorEmaAvgSlow;
bool coldFloor = false;

int tempDispCounter = 5;
int tempDispCounter2 = 0;
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
uint8_t changePerHourMinuteCounter = 0;

// float last59MedEMAs[59];

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
 *  finally KD monitoris ramp time, and increase this to reduce overshoot, or reduce to increase stability.
 *
 */

const double outputMin = -128;
const double outputMax = 127;
const double MIDPOINT = 0; // greater than this == on

// Proportional
double Kp = 8;
// Integral (testing 0.05 [was 0.005] starting jan 21 2022)
double Ki = 0.05; // 0.005
// Derivative
double Kd = 0; // 0.002

double Input;
double Setpoint = 68;
double Output;

// AutoPID myPID(&Input, &Setpoint, &Output, outputMin, outputMax, Kp, Ki, Kd);

ArduPID ArduPIDController;

// move this to functions?
uint16_t calcEma(uint16_t reading, uint16_t lastEma, uint16_t days)
{
	// ema = (floatSensorReadValue * (2. / (1 + EMA_PERIODS_SHORT)) + lastEma * (1 - (2. / (1 + EMA_PERIODS_SHORT))));
	return (reading * (2. / (1 + days)) + lastEma * (1 - (2. / (1 + days))));
}