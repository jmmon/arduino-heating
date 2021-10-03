#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
// #include <QuickPID.h>
#include <AutoPID.h>

const String VERSION_NUMBER = "4.1.2";
const bool DEBUG = false;

#define DHT1PIN 7 //main
#define DHT2PIN 8 //upstairs

#define DHT3PIN 4 //not yet used
#define DHT4PIN 12 //not yet used

const uint8_t WATER_FLOW_PIN = 3;   //flow sensor

const uint8_t FLOOR_SENSOR_COUNT = 2;
const uint8_t FLOOR_TEMP_PIN[2] = {A0, A1};

const uint8_t THERMOSTAT_BUTTONS_PIN = A2;
const uint8_t WATER_FLOAT_PIN = A3; //tank sensors

//{WATER_FLOW_PIN:3, DHT1PIN:4, PUMP_PIN:5, DHT2PIN:6, (DHT3PIN:7)}
//{FLOOR_TEMP_PIN[0]:A0, FLOOR_TEMP_PIN[1]:A1, THERMOSTAT__BUTTONS_PIN:A2, WATER_FLOAT_PIN:A3}

const uint8_t AIR_SENSOR_COUNT = 4;
DHT dht[4] = { 
    {DHT1PIN, DHT22}, 
    {DHT2PIN, DHT22}, 
    {DHT3PIN, DHT22}, //  outside, not yet installed // nan
    {DHT4PIN, DHT22},// greenhouse? // nan
};

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display  
//SDA = A4 pin; SCL = A5 pin


uint32_t currentTime = 0;

uint32_t last250ms = 0;
uint8_t ms1000ctr = 0;
uint8_t ms2500ctr = 0;

const uint8_t LCD_INTERVAL_SECONDS = 4;
int8_t lcdCounter = 0;

int8_t lastButtonStatus = 0; //thermostat buttons
uint16_t lastTankRead = 0;

const uint8_t LCD_PAGE_MAX = 5;
uint8_t lcdPage = LCD_PAGE_MAX;

const float EMA_MULT[3] = {
    //  2 / (1 + n)   =>  where (n * TEMPERATURE_READ_INTERVAL) = milliseconds defining EMA calculation
    2. / (1 + 12),    //12 * 2.5s = 30s   EMA =   0.5m
    2. / (1 + 600),   //600 * 2.5s = 1500s EMA =  25m
    2. / (1 + 1800),  //1800 * 2.5s = 4500s EMA = 75m
};

const float FLOOR_WARMUP_TEMPERATURE = 475; //0-1023, NOTE: insulation (cardboard, rugs) will require higher value
const float FLOOR_EMA_MULT = 2. / (1 + 10); //10 readings EMA (20s)
float floorEmaAvg;

int tempDispCounter = 5;
int tempDispCounter2 = 0;
uint8_t errorCounter1 = 5;
uint8_t errorCounter2 = 5;

float l_minute = 0;
float totalVolume = 0;
volatile uint32_t waterCounter = 0;

uint8_t changePerHourMinuteCounter = 0;
float last59MedEMAs[59];


bool set = false;



 // *******************************************************
// AutoPID

//pid settings and gains
#define outputMin 0
#define outputMax 255

/**
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

#define Kp 4
#define Ki 0.005
#define Kd 1

double Input, 
    Setpoint = 64,
    Output;

AutoPID myPID(&Input, &Setpoint, &Output, outputMin, outputMax, Kp, Ki, Kd);
 // *******************************************************





 // *******************************************************
 // QuickPID
// const uint32_t sampleTimeUs = 2500000; // 2500ms
// const int outputMax = 255; // 128 is start of on, 255 - (128/2) = 191
// const int outputMin = 0; // 127 is end of off

// bool printOrPlotter = 0;  // on(1) monitor, off(0) plotter
// float POn = 0.5;          // proportional on Error to Measurement ratio (0.0-1.0), default = 1.0
// float DOn = 0.0;          // derivative on Error to Measurement ratio (0.0-1.0), default = 0.0

// byte outputStep = 1;
// byte hysteresis = 1;
// int setpoint = 66;       // 1/3 of range for symetrical waveform
// int output = 85;          // 1/3 of range for symetrical waveform

// float Input, Output, Setpoint = 64;
// float Kp = 2, Ki = 0.05, Kd = 1;
// bool pidLoop = false;

// QuickPID _myPID = QuickPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, POn, DOn, QuickPID::DIRECT);
 // *******************************************************
