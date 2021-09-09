// from (floor = 625, outside = snow), 360 trend gains ~0.9 degrees F (from 68 to 68.9)
  //  6:09 to 7:26: (floor = 568, outside = snow), trend fell to 264 and temp fell to 68.2 F


//NEW: add medium speed: if between 5 and <3 of set value, go medium speed
//  if (current < set -5) go warm up

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHT1PIN 7 //main
#define DHT2PIN 8 //upstairs //(not currently implemented)
#define DHT3PIN 12 //not yet used

DHT dht[] = {
    {DHT1PIN, DHT22},
    {DHT2PIN, DHT22},
    {DHT3PIN, DHT22}, //  outside, not yet installed
    // greenhouse?
};

const bool DEBUG = true;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display  
//SDA = A4 pin
//SCL = A5 pin

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

byte customCharDroplet[] = {
  B00100,
  B01110,
  B01010,
  B10111,
  B10111,
  B10111,
  B11111,
  B01110
};

byte customCharSmColon[] = {
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
const uint8_t WATER_LEVEL_PIN = A3; // float sensor
const uint8_t WATER_FLOW_PIN = 3;   // flow sensor not working yet

//{WATER_FLOW_PIN:3, DHT1PIN:4, PUMP_PIN:5, DHT2PIN:6, (DHT3PIN:7)}
//{FLOOR_TEMP_PIN[0]:A0, FLOOR_TEMP_PIN[1]:A1, THERMOSTAT__BUTTONS_PIN:A2, WATER_LEVEL_PIN:A3}

uint32_t lastPumpUpdate = 0;
uint32_t lastTemperatureRead = 0;
uint32_t lastButtonCheck = 0;
uint32_t cycleStartTime = 0;
uint32_t currentTime = 0;

int8_t lastButtonStatus = 0; //thermostat buttons
uint8_t lastButtonRead = 0;

const uint8_t BUTTON_CHECK_INTERVAL = 850; //adj thermostat every ()ms while one of the buttons is held
const uint32_t TEMPERATURE_READ_INTERVAL = 2500;
uint32_t pumpUpdateInterval = 60000;
uint32_t checkPump = 3000; //checks the pump state 2 seconds after start

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
    { "(continue) ", 60000, 0 },        // continue uses minTime
};

uint8_t currentPumpState = 0;
uint8_t nextPumpState = 0;

float temperatureSetPoint = 69; /*in F*/
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

struct airSensor_t
{
    DHT *sensor;
    float tempC;
    float tempF;
    float WEIGHT;
    float humid;

    float highest;
    float lowest;
    float currentEMA[3]; // {short, medium, long} EMA
    float lastEMA[3];    // {short, medium, long}
    int16_t trendEMA;
    
    bool working;
    String label;

} airSensor[] = {
    {&dht[0], 0, 0, 1, 0,     0, 999, {0, 0, 0}, {0, 0, 0}, 0,    true, "MAIN"},
    {&dht[1], 0, 0, 2, 0,     0, 999, {0, 0, 0}, {0, 0, 0}, 0,    true, "UPSTAIRS"},
};

struct floorSensor_t
{
    uint8_t PIN;
    float currentEMA;
    float lastEMA;

} floorSensor[] = {
    {FLOOR_TEMP_PIN[0], 0, 0},
    {FLOOR_TEMP_PIN[1], 0, 0},
};


uint8_t changePerHourMinuteCounter = 0;
float last59MedEMAs[59];

//********************************************************************************************************************************* FUNCTIONS
float ReadFloorTemperature(int pin, int a, int val = 0, int z = 0)
{
    for (z = 0; z < a; z++) {
        val = val + analogRead(pin);
    }
    return val / a;
}

void updateTEMP()
{
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        airSensor[i].tempF = airSensor[i].sensor->readTemperature(true);
        airSensor[i].humid = airSensor[i].sensor->readHumidity();
    }

    if (isnan(airSensor[0].humid) || isnan(airSensor[0].tempF)) { //check for errors in two sensors
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
            Serial.println(F("ERROR BOTH SENSORS "));
        } else {
            if (errorCounter1 == 30) {
                Serial.print(F("DHT.main error! "));
                errorCounter1 = 0;
            }
            airSensor[0].humid = airSensor[1].humid;
            airSensor[0].tempF = airSensor[1].tempF;
        }
        errorCounter1++;
        } else {
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
            if (errorCounter2 == 30) {
                Serial.print(F("DHT.upstairs error! "));
                errorCounter2 = 0;
            }
            airSensor[1].humid = airSensor[0].humid;
            airSensor[1].tempF = airSensor[0].tempF;
            errorCounter2++;
        }
    }

    //check for strange readings (>20 from long avg)
    if (airSensor[0].currentEMA[2] != 0) // don't run the first time
    {
        if (airSensor[0].tempF > 20 + airSensor[0].currentEMA[2] || airSensor[0].tempF < -20 + airSensor[0].currentEMA[2]) {
          //out of range, set to other sensor (hoping it is within range);
          Serial.print(F(" DHT.main outlier! ")); Serial.print(airSensor[0].tempF); Serial.print(F(" vs EMA_Long ")); Serial.print(airSensor[0].currentEMA[2]);
          airSensor[0].tempF = airSensor[1].tempF;
        }

        if (airSensor[1].tempF > 20 + airSensor[1].currentEMA[2] || airSensor[1].tempF < -20 + airSensor[1].currentEMA[2]) {
          //out of range, set to other sensor (hoping it is within range);
          Serial.print(F(" DHT.upstairs outlier! ")); Serial.print(airSensor[1].tempF); Serial.print(F(" vs EMA_Long ")); Serial.print(airSensor[1].currentEMA[2]);
          airSensor[1].tempF = airSensor[0].tempF;
        }
    }
    

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        if (airSensor[i].highest < airSensor[i].tempF) {
            airSensor[i].highest = airSensor[i].tempF;
        }

        if (airSensor[i].lowest > airSensor[i].tempF) {
            airSensor[i].lowest = airSensor[i].tempF;
        }

        for (uint8_t z = 0; z < 3; z++) {
            if (airSensor[i].lastEMA[z] == 0) {
                airSensor[i].lastEMA[z] = airSensor[i].tempF;
            }

            airSensor[i].currentEMA[z] = airSensor[i].tempF * EMA_MULT[z] + airSensor[i].lastEMA[z] * (1 - EMA_MULT[z]);
            airSensor[i].lastEMA[z] = airSensor[i].currentEMA[z];
        }
    }

    weightedTemp = (airSensor[0].WEIGHT * airSensor[0].currentEMA[0] + airSensor[1].WEIGHT * airSensor[1].currentEMA[0]) / (airSensor[0].WEIGHT + airSensor[1].WEIGHT);

    for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) {
        if (floorSensor[i].lastEMA == 0) {
            floorSensor[i].lastEMA = ReadFloorTemperature(floorSensor[i].PIN, 10);
        }
        floorSensor[i].currentEMA = (ReadFloorTemperature(floorSensor[i].PIN, 10) * FLOOR_EMA_MULT + floorSensor[i].lastEMA * (1 - FLOOR_EMA_MULT));
        floorSensor[i].lastEMA = floorSensor[i].currentEMA;
    }

    int difference = int(abs(floorSensor[0].currentEMA - floorSensor[1].currentEMA)); //error check
    if (difference > 80) {
        if (VERBOSE) {
            Serial.print(F("Floor Read Error. Difference: "));
            Serial.println(difference);
        }
        if (floorSensor[0].currentEMA > floorSensor[1].currentEMA) {
            floorSensor[1].currentEMA = floorSensor[0].currentEMA;
        } else {
            floorSensor[0].currentEMA = floorSensor[1].currentEMA;
        }
    }
    floorEmaAvg = (floorSensor[0].currentEMA + floorSensor[1].currentEMA) / FLOOR_SENSOR_COUNT; //avg two readings

    if (tempDispCounter >= 4) { //every 4 reads = 10 seconds calculate trend and print info
        for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
            if (airSensor[i].currentEMA[0] < airSensor[i].currentEMA[1]) { // if (now) temp < (medium avg) temp; temp is falling:
                if (airSensor[i].currentEMA[1] < weightedTemp) { //if medium
                    if (airSensor[i].trendEMA <= -DOWNTREND) {
                        airSensor[i].trendEMA += DOWNTREND; //fast going back to 0
                    } else{
                        airSensor[i].trendEMA = 0;
                    }
                } else {
                    //                if (airSensor[i].currentEMA[1] - airSensor[i].currentEMA[0] < 0.5) {  //if difference is small, this means temp is falling fast, so we'll up the trend quicker so pump turns on sooner.
                    //                    airSensor[i].trendEMA += -UPTREND * 2;    //slow going down
                    //                } else {
                    //                    airSensor[i].trendEMA += -UPTREND;    //slow going down     //if difference is large, this means temp is falling slowly, so we don't need the pump to come on as soon.
                    //                }
                    airSensor[i].trendEMA += -UPTREND; //slow going down
                }
            }
            if (airSensor[i].currentEMA[0] > airSensor[i].currentEMA[1]) { //if rising
                if (airSensor[i].currentEMA[1] > weightedTemp) {
                    if (airSensor[i].trendEMA >= DOWNTREND) {
                        airSensor[i].trendEMA += -DOWNTREND; //fast going back to 0
                    } else {
                        airSensor[i].trendEMA = 0;
                    }
                } else {
                    //                if (airSensor[i].currentEMA[0] - airSensor[i].currentEMA[1] < 0.5) {  //if difference is small, this means temp is rising fast, so we'll up the trend quicker so pump shuts off sooner.
                    //                    airSensor[i].trendEMA += UPTREND * 2;    //slow going up
                    //                } else {
                    //                    airSensor[i].trendEMA += UPTREND;    //slow going up      //if difference is large, this means temp is rising slow, so we don't want the pump to shut off as soon.
                    //                }
                    airSensor[i].trendEMA += UPTREND; //slow going up
                }
            }

            if (airSensor[i].trendEMA > AIR_TEMP_TREND_MINMAX) {airSensor[i].trendEMA = AIR_TEMP_TREND_MINMAX;}
            if (airSensor[i].trendEMA < -AIR_TEMP_TREND_MINMAX) {airSensor[i].trendEMA = -AIR_TEMP_TREND_MINMAX;}
        }
        avgTrend = (airSensor[0].trendEMA + airSensor[1].trendEMA) / 2.;

        if (tempDispCounter2 >= 6) { // every minute:
            Serial.println();
            Serial.print(F(" °F "));
            Serial.print(F("  EMA_Long.")); Serial.print(airSensor[0].currentEMA[2]);
            if (airSensor[0].currentEMA[2] != airSensor[1].currentEMA[2]) {
                Serial.print(F("/"));
                Serial.print(airSensor[1].currentEMA[2]);
            }
            Serial.print(F("  _Med.")); Serial.print(airSensor[0].currentEMA[1]);
            if (airSensor[0].currentEMA[1] != airSensor[1].currentEMA[1]) {
                Serial.print(F("/"));
                Serial.print(airSensor[1].currentEMA[1]);
            }
            Serial.print(F("  Trend: ")); Serial.print(airSensor[0].trendEMA);
            if (airSensor[0].trendEMA != airSensor[1].trendEMA) {
                Serial.print(F(" | "));
                Serial.print(airSensor[1].trendEMA);
            }

            Serial.print(F(" Water(Flow: "));
            Serial.print(l_minute);
            Serial.print(F(" l/m, totalVol: "));
            Serial.print(totalVolume);
            Serial.print(F(" liters, "));
            //check tank levels
            uint16_t tankReading = analogRead(WATER_LEVEL_PIN);
            Serial.print(F("Level: "));
            if (tankReading > 900) { //~1023
                Serial.print(F("< 25%"));
            }
            else if (tankReading > 685) { //~750
                Serial.print(F("25~50%"));
            }
            else if (tankReading > 32) { //~330
                Serial.print(F("50~75%"));
            } else { //~0
                Serial.print(F("> 75%"));
            }
            tempDispCounter2 = 0;
            Serial.print(F(") "));


            if (changePerHourMinuteCounter < 59) {
              changePerHourMinuteCounter++;
            } else {
              
            }
            
            for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
              last59MedEMAs[k] = last59MedEMAs[k+1];
            }
            last59MedEMAs[58] = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
        }
        Serial.print(airSensor[0].currentEMA[0]);
        if (airSensor[0].currentEMA[0] != airSensor[1].currentEMA[0]) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].currentEMA[0]);
        }
        Serial.print(F("  "));


        lcdPage1();
        
        
        tempDispCounter2++;
        tempDispCounter = 0;
    }
    tempDispCounter++;

    //Set next pump state

    if (weightedTemp > temperatureSetPoint || (avgTrend > 0 && weightedTemp > (temperatureSetPoint - AIR_TEMP_TREND_FACTOR * avgTrend))) { // check if should be off
    
        nextPumpState = 0;
    } else { //  else should be on:
        if (weightedTemp > temperatureSetPoint - 3) {  // if temp needs to move <3 degrees, turn on/start
            nextPumpState = 1;
        } else if (weightedTemp > temperatureSetPoint - 5) {  //if temp needs to move 3-5 degrees, go medium
            nextPumpState = 3; //  medium
        } else {                 // else, go full speed
            nextPumpState = 4; //  high
        }
    }
    if (currentPumpState == 0 && nextPumpState != 0) {   // Force update if switching to on from off (i.e. cut short the off cycle if turning on)
        checkPumpCycleState();
    }
}



void checkPumpCycleState() {
    unsigned long lastCycleDuration = 0;
    Serial.println();

    if (nextPumpState == 1) {
        if (currentPumpState == 1) {                                                //  continue from on to on
            pumpUpdateInterval = CYCLE[5].MIN_TIME; //add a minute to continue same phase
            Serial.println(F("TESTING - On cycle continue phase"));
        } else if (currentPumpState != 0) { //  on from start or warmup
            currentPumpState = 1;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            Serial.println(F("TESTING - On cycle initialization (after Start)"));
            analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
        } else { /*if (currentPumpState == (0)*/   //  start from off or warm up (currentPumpState == (0 || 3))
            currentPumpState = 2;                                       //  set new current pump state to START
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME; //  add START interval
            lastCycleDuration = currentTime - cycleStartTime;           //  start new cycle..
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - Start cycle initialization"));
            analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
        }
    } else if (nextPumpState == 3) { // warm up changeto medium
        if (currentPumpState == 3) {                                                // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - medium cycle continue phase"));
        } else { // start as different phase
            currentPumpState = 3;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - medium cycle initialization"));
            analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
        }
    } else if (nextPumpState == 4) { // warm up
        if (currentPumpState == 4) {                                                // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - warm up cycle continue phase"));
        } else { // start as different phase
            currentPumpState = 4;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - warm up cycle initialization"));
            analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
        }
    } else if (nextPumpState == 0) { // off
        if (currentPumpState == 0) {                                                // continue same phase
            pumpUpdateInterval = CYCLE[5].MIN_TIME; // add a minute to continue same phase
            Serial.println(F("TESTING - Off cycle continue phase"));
        } else { //start as different phase
            currentPumpState = 0;
            pumpUpdateInterval = CYCLE[currentPumpState].MIN_TIME;
            lastCycleDuration = currentTime - cycleStartTime;
            cycleStartTime = currentTime;
            Serial.println(F("TESTING - Off cycle initialization"));
            analogWrite(PUMP_PIN, CYCLE[currentPumpState].PWM);
        }
    }
    

    if (lastCycleDuration != 0) {   // if end of cycle:
        uint16_t hours = lastCycleDuration / 3600000;
        lastCycleDuration -= (hours * 3600000);
        uint16_t minutes = lastCycleDuration / 60000;
        lastCycleDuration -= (minutes * 60000);
        uint16_t seconds = lastCycleDuration / 1000;
        lastCycleDuration -= (seconds * 1000);
        if (seconds >= 60) {
            minutes += 1;
            seconds -= 60;
        }
        if (minutes >= 60) {
            hours += 1;
            minutes -= 60;
        }
        if (lastCycleDuration >= 500 && (minutes > 0 || hours > 0)) {
            seconds += 1;
        }


        if (DEBUG) {
            Serial.print(F("                          Total cycle time: "));
            if (hours > 0) {
                Serial.print(hours);
                Serial.print(F("h"));
            }
            if (minutes > 0) {
                Serial.print(minutes);
                Serial.print(F("m"));
            }
            if (seconds > 0) {
                Serial.print(seconds);
                Serial.print(F("s"));
            }
            Serial.print(F(" ms:"));
            if (hours == 0 && minutes == 0) {
                Serial.print(lastCycleDuration);    
            }
            //Serial.println();
        }
    }


    if (DEBUG) {
        // print data
        Serial.println();
        Serial.print(F(" Set: "));
        Serial.print(temperatureSetPoint, 1);
        Serial.print(F("°F"));
        Serial.print(F("   Floor: "));
        Serial.print(floorSensor[0].currentEMA);
        if (floorSensor[0].currentEMA != floorSensor[1].currentEMA) {
            Serial.print(F(" / "));
            Serial.print(floorSensor[1].currentEMA);
        }
        Serial.print(F("   PWM "));
        if (CYCLE[currentPumpState].PWM == 0) {
            Serial.print(F("  "));
        }
        Serial.print(CYCLE[currentPumpState].PWM);
        Serial.print(F("                                       *Pump"));
        Serial.print(CYCLE[currentPumpState].NAME);
        Serial.print(F("(HIGHS "));
        Serial.print(airSensor[0].highest);
        if (airSensor[0].highest != airSensor[1].highest) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].highest);
        }
        Serial.print(F(" LOWS "));
        Serial.print(airSensor[0].lowest);
        if (airSensor[0].lowest != airSensor[1].lowest) {
            Serial.print(F("/"));
            Serial.print(airSensor[1].lowest);
        }
        Serial.print(F(")"));
        if (changePerHourMinuteCounter == 60) {
            Serial.print(F("Temp Change per Hr (Medium EMA): "));
            float avgNow = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
            float diff = last59MedEMAs[0] - avgNow;
            Serial.print(diff);
        }
        Serial.println();
    }
}

void countWater()
{
    waterCounter++;
}

void setup()
{ //************************************************************************************************ SETUP
    Serial.begin(9600);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FLOOR_TEMP_PIN[0], INPUT);
    pinMode(FLOOR_TEMP_PIN[1], INPUT);
    pinMode(THERMOSTAT_BUTTONS_PIN, INPUT);
    pinMode(WATER_LEVEL_PIN, INPUT);
    pinMode(WATER_FLOW_PIN, INPUT);

    lcd.createChar(0, customCharDegF);

    lcd.createChar(1, customCharInside);
    lcd.createChar(2, customCharOutside);

    lcd.createChar(3, customCharSmColon);

    lcd.createChar(4, customCharDroplet);

    for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++) {
        dht[k].begin();
    }
        
    lcd.init();                      // initialize the lcd
    lcd.clear();
    lcd.backlight();  //open the backlight
    //lcd.createChar(0, customChar); // create a new custom character

    //TESTING DISPLAY
    lcd.setCursor(0,0);
    lcd.print("TESTING DISPLAY");
    lcd.setCursor(2,1);
    lcd.print("Hello World");
    //TESTING DISPLAY
    
    analogWrite(PUMP_PIN, 0);
    attachInterrupt(digitalPinToInterrupt(WATER_FLOW_PIN), countWater, FALLING);
    Serial.println();
    Serial.print(F("Version "));
    Serial.println(VERSION_NUMBER);
    Serial.println(F("DHTxx test!"));

    delay(1200);
    lastButtonCheck = millis();
    lastPumpUpdate = lastButtonCheck;
    lastTemperatureRead = lastButtonCheck;
    cycleStartTime = lastButtonCheck;
    updateTEMP();
} //************************************************************************************************ END SETUP

void loop() { //************************************************************************************************* LOOP
    currentTime = millis();
    if (currentTime >= (lastPumpUpdate + pumpUpdateInterval)) { //pump update timer
        checkPumpCycleState();
        lastPumpUpdate += pumpUpdateInterval;
    }

    if (checkPump != 0 && currentTime >= checkPump) { //special case pump check i.e. initialization, after thermostat changes
        checkPumpCycleState();
        checkPump = 0;
        //switch display back to temperature?
    }

    if (currentTime >= (lastTemperatureRead + TEMPERATURE_READ_INTERVAL)) { //Read temperature

        if (waterCounter != 0) {                                                    //not working
            float thisDuration = (currentTime - lastTemperatureRead) / 1000;
            //this cycle's duration in seconds
            // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.    (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
            l_minute = (waterCounter / 7.5) / thisDuration; // divide by 2.5 because this is on a 2.5s timer  //returns vol per minute extrapolated from 2.5s
            totalVolume += (l_minute * thisDuration / 60);  //adds vol from the last 2.5s to totalVol
            waterCounter = 0;
        }

        updateTEMP();
        lastTemperatureRead += TEMPERATURE_READ_INTERVAL;
    }

    if (currentTime >= (lastButtonCheck + BUTTON_CHECK_INTERVAL)) { // read thermostat buttons
        lastButtonCheck += BUTTON_CHECK_INTERVAL;
        int8_t buttonStatus = 0;
        uint16_t buttonRead = analogRead(THERMOSTAT_BUTTONS_PIN);
        if (buttonRead > 63) {
            Serial.println(buttonRead);
            if (buttonRead > 831) {
                buttonStatus = 1;
                for (uint8_t k = 0; k < 2; k++) {  //reset record temps
                   airSensor[k].lowest = airSensor[k].tempF;
                }
            } else {
               buttonStatus = -1;
               for (uint8_t k = 0; k < 2; k++) {
                   airSensor[k].highest = airSensor[k].tempF;
                }
            }
            
            temperatureSetPoint += (0.5 * buttonStatus);
            lcdPageSet();
            
            checkPump = currentTime + 3000; // wait 3 seconds before accepting new tempoerature in case button will be pressed more than once
        }
    }

} //************************************************************************************************* END LOOP

void lcdPageSet() {
    lcd.setCursor(0,0);
    lcd.print("Temp: "); // 6
    lcd.print(airSensor[0].currentEMA[0],1); //* 10
    lcd.write(0); // degrees F //* 11
    lcd.print("     "); //* 16

    lcd.setCursor(0,1); //x, y
    lcd.print(" Set"); //* 4
    lcd.write(3); // sm colon //* 5
    lcd.print(temperatureSetPoint,1); //* 9
    lcd.write(0); // degrees F //* 10
    lcd.print("      "); //* 16
}

void lcdPage1() {
    uint16_t tankPercent = analogRead(WATER_LEVEL_PIN) / 1023 * 99;
    
    lcd.setCursor(0,0);
    lcd.write(1); // inside icon // 1
    lcd.print(airSensor[0].currentEMA[0],1);  // 5
    lcd.print(" ");          // 6
    lcd.print(airSensor[1].currentEMA[0],1); // 10
    lcd.write(0); // degrees F // 11
    lcd.print("     "); // 16

    lcd.setCursor(0,1);
    lcd.print("Set"); // 3
    lcd.write(3); // 4
    lcd.print(temperatureSetPoint,1); // 8
    lcd.write(0); // degrees F // 11
    lcd.print("  "); // 13
    
    lcd.write(4); // water droplet // 14
    if (tankPercent < 10) {
        lcd.print(" "); // 15
    }
    lcd.print(tankPercent); // 16
}

void lcdPage2() {

}
