// from (floor = 625, outside = snow), 360 trend gains ~0.9 degrees F (from 68 to 68.9)
  //  6:09 to 7:26: (floor = 568, outside = snow), trend fell to 264 and temp fell to 68.2 F


//NEW: add medium speed: if between 5 and <3 of set value, go medium speed
//  if (current < set -5) go warm up

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <AutoPID.h>

#define DHT1PIN 7 //main
#define DHT2PIN 8 //upstairs

#define DHT3PIN 12 //not yet used

DHT dht[] = { 
    {DHT1PIN, DHT22}, 
    {DHT2PIN, DHT22}, 
    {DHT3PIN, DHT22}, //  outside, not yet installed
    // greenhouse?
};

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display  
//SDA = A4 pin
//SCL = A5 pin

bool DEBUG = false;

byte customCharInside[] = { // "inside"
  B00000,
  B00000,
  B00000,
  B10100,
  B01111,
  B11100,
  B01000,
  B01000
};

byte customCharOutside[] = { // "outside"
  B01010,
  B00110,
  B01110,
  B00000,
  B00000,
  B01111,
  B01000,
  B01000
};

byte customCharDegF[] = { // "degrees F"
  B01000,
  B10100,
  B01000,
  B00111,
  B00100,
  B00110,
  B00100,
  B00100
};

byte customCharDroplet[] = { //water droplet
  B00100,
  B01110,
  B01010,
  B10111,
  B10111,
  B10111,
  B11111,
  B01110
};

byte customCharSmColon[] = { // small colon + space
  B00000,
  B00000,
  B01000,
  B00000,
  B00000,
  B01000,
  B00000,
  B00000
};


const uint8_t AIR_SENSOR_COUNT = 2;
const uint8_t FLOOR_SENSOR_COUNT = 2;
const bool VERBOSE = false;
const String VERSION_NUMBER = "4.0.1";

const uint8_t PUMP_PIN = 5;
const uint8_t FLOOR_TEMP_PIN[2] = {A0, A1};
const uint8_t THERMOSTAT_BUTTONS_PIN = A2;
const uint8_t WATER_LEVEL_PIN = A3; //tank sensors
const uint8_t WATER_FLOW_PIN = 3;   //flow sensor

//{WATER_FLOW_PIN:3, DHT1PIN:4, PUMP_PIN:5, DHT2PIN:6, (DHT3PIN:7)}
//{FLOOR_TEMP_PIN[0]:A0, FLOOR_TEMP_PIN[1]:A1, THERMOSTAT__BUTTONS_PIN:A2, WATER_LEVEL_PIN:A3}

uint32_t lastPumpUpdate = 0;
uint32_t lastTemperatureRead = 0;
uint32_t lastButtonCheck = 0;
uint32_t cycleStartTime = 0;
uint32_t currentTime = 0;

int8_t lastButtonStatus = 0; //thermostat buttons
uint8_t lastButtonRead = 0;

const uint16_t BUTTON_CHECK_INTERVAL = 250; //adj thermostat every ()ms while one of the buttons is held
const uint16_t TEMPERATURE_READ_INTERVAL = 2500;
uint32_t pumpUpdateInterval = 60000;
uint32_t checkPump = 3000; //checks the pump state 2 seconds after start


// LCD update interval
const uint16_t LCD_UPDATE_INTERVAL = 5000; //5 seconds
uint32_t lastLcdUpdate = 0;
uint8_t LCD_PAGE_MAX = 1;
// uint8_t lcdPage = 0;
uint8_t lcdPage = LCD_PAGE_MAX;



struct cycle_t {
    String NAME;
    uint32_t MIN_TIME;
    uint8_t PWM;

} CYCLE[] = {
    { "   *OFF*   ", 1800000, 0 },      // Off cycle
    { "*LOW*      ", 600000, 199 },     // On cycle // low
    { "**STARTUP**", 3000, 223 },       // Motor Start
    { " *MEDIUM*  ", 300000, 223 },     // medium
    { "     *HIGH*", 300000, 255 },     // high
    { "(continue) ", 60000, 0 },        // continue uses minTime        Dont need a whole separate cycle profile for continue do I? Instead just keep it in the same mode number and add add a minute? Or, have it display the string, and for mode 6 at initialiation it sets the string to whatever the prev mode string was (so it remains on the screen for the full duration of the continue)
};


uint8_t currentPumpState = 0;
uint8_t nextPumpState = 0;

float tempSetPoint = 65; /*in F*/
float weightedTemp;
float avgTrend;
const int UPTREND = 1;                     // every 10 seconds trend is updated. if going towards the target temp, increase trend by this amount.
const int DOWNTREND = 10;                   // after overshooting and going back to baseline, reduce trend by this amount so it quickly resets.
const int AIR_TEMP_TREND_MINMAX = 360;     // 60min to reach max
const float AIR_TEMP_TREND_FACTOR = 0.003; // MINMAX * FACTOR = temperature trigger adjustment
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



// AutoPID

//pid settings and gains
#define OUTPUT_MIN -57
#define OUTPUT_MAX 57

#define KP .12      // proportional (more error/difference = more PWM)
#define KI .0003    // integral (integrate past errors; error over time)
#define KD 0        // derivative ("anticipatory control"; future estimate of trend of the error)

double temperature, 
    setPoint = tempSetPoint, 
    outputVal;



AutoPID myPID(&temperature, &setPoint, &outputVal, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);


class AirSensor_C {
    public:
        AirSensor_C(DHT *s, String l, float w) {
            sensor = s;
            label = l;
            WEIGHT = w;
        }

        DHT *sensor;
        float tempC = 0;
        float tempF = 0;
        float WEIGHT = 1;
        float humid = 0;

        float highest = -1000;
        float lowest = 1000;
        float currentEMA[3]; // {short, medium, long} EMA
        float lastEMA[3];    // {short, medium, long}
        int16_t trendEMA = 0;
        
        bool working = true;
        String label;

        readTemp() {
            //read temp/humid
            tempC = sensor->readTemperature();
            tempF = sensor->readTemperature(true);
            humid = sensor->readHumidity();
        };

        update() {
            //update highest / lowest
            if (highest < tempF) {
                highest = tempF;
            }
            if (lowest > tempF) {
                lowest = tempF;
            }

            //update EMAs (short, medium, long)
            for (uint8_t ema = 0; ema < 3; ema++) { // update EMA
                if (lastEMA[ema] == 0) {
                    lastEMA[ema] = tempF;
                }

                currentEMA[ema] = tempF * EMA_MULT[ema] + lastEMA[ema] * (1 - EMA_MULT[ema]);
                lastEMA[ema] = currentEMA[ema];
            }
        };

        updateEmaTrend() {
            if (currentEMA[0] < currentEMA[1]) { // if (now) temp < (medium avg) temp; temp is falling: 
                if (currentEMA[1] < weightedTemp) { // if medium 
                    if (trendEMA <= -DOWNTREND) {
                        trendEMA += DOWNTREND; // fast going back to 0
                    } else {
                        trendEMA = 0;
                    }
                } else {
                    //                if (currentEMA[1] - currentEMA[0] < 0.5) {  //if difference is small, this means temp is falling fast, so we'll up the trend quicker so pump turns on sooner.
                    //                    trendEMA += -UPTREND * 2;    //slow going down
                    //                } else {
                    //                    trendEMA += -UPTREND;    //slow going down     //if difference is large, this means temp is falling slowly, so we don't need the pump to come on as soon.
                    //                }
                    trendEMA += -UPTREND; //slow going down
                }
            }

            if (currentEMA[0] > currentEMA[1]) { // if rising
                if (currentEMA[1] > weightedTemp) {
                    if (trendEMA >= DOWNTREND) {
                        trendEMA += -DOWNTREND; // fast going back to 0
                    } else {
                        trendEMA = 0;
                    }
                } else {
                    //                if (currentEMA[0] - currentEMA[1] < 0.5) {  //if difference is small, this means temp is rising fast, so we'll up the trend quicker so pump shuts off sooner.
                    //                    trendEMA += UPTREND * 2;    //slow going up
                    //                } else {
                    //                    trendEMA += UPTREND;    //slow going up      //if difference is large, this means temp is rising slow, so we don't want the pump to shut off as soon.
                    //                }
                    trendEMA += UPTREND; //slow going up
                }
            }

            if (trendEMA > AIR_TEMP_TREND_MINMAX) {
                trendEMA = AIR_TEMP_TREND_MINMAX;
            }
            if (trendEMA < -AIR_TEMP_TREND_MINMAX) {
                trendEMA = -AIR_TEMP_TREND_MINMAX;
            }
        };


}
airSensor[] = {
    AirSensor_C(&dht[0], "MAIN", 1),
    AirSensor_C(&dht[1], "UPSTAIRS", 2),
};



class FloorSensor_C {
    public:
        FloorSensor_C(int pin) { // constructor
            PIN = pin;
        };
        uint8_t PIN;
        float currentEMA = 0;
        float lastEMA = 0;

        readTemp(int a = 10) {
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