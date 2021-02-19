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

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display  
//SDA = A4 pin
//SCL = A5 pin

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

const uint16_t BUTTON_CHECK_INTERVAL = 350; //adj thermostat every ()ms while one of the buttons is held
const uint16_t TEMPERATURE_READ_INTERVAL = 2500;
uint32_t pumpUpdateInterval = 60000;
uint32_t checkPump = 3000; //checks the pump state 2 seconds after start

const uint32_t MINIMUM_CYCLE_TIMES[6] = 
{
  1800000,        // Off cycle            30 m
  600000,         // On cycle             10 m
  3000,           // Motor start "cycle"  1 second
  300000,         // medium               5 m
  300000,         // Warm up cycle        5 m
  60000           // Continue last cycle  1 m
};
const String MOTOR_STATUS_STRING[5] = 
{
  "      *OFF*",
  "*ON*       ",
  "**STARTUP**",
  "*MED*      ",
  "**WARM UP**"
};
const uint8_t PWM[5] = 
{
  0,              // Off cycle
  199,            // On cycle // low
  223,            // Motor Start
  223,                        // medium
  255             // Warm up  // high
};
uint8_t currentPumpState = 0;
uint8_t nextPumpState = 0;

float tempSetPoint = 69; /*in F*/
float weightedTemp;
float avgTrend;
const uint8_t UPTREND = 1;                     // every 10 seconds trend is updated. if going towards the target temp, increase trend by this amount.
const uint8_t DOWNTREND = 10;                   // after overshooting and going back to baseline, reduce trend by this amount so it quickly resets.
const uint16_t AIR_TEMP_TREND_MINMAX = 360;     // 60min to reach max
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

uint8_t tempDispCounter = 5;
uint8_t tempDispCounter2 = 0;

float l_minute = 0;
float totalVolume = 0;
volatile uint32_t waterCounter = 0;

uint8_t lcdPage = 0;
uint8_t MAX_LCD_PAGES = 4;
uint8_t tStatDispCtr = 0;

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

uint32_t cycleCounter = 0;

//*********************************************************************************************************************** FUNCTIONS

float ReadFloorTemperature(uint8_t pin, uint8_t a, uint16_t val = 0) {
    for (uint8_t z = 0; z < a; z++) {
        val = val + analogRead(pin);
    }
    return val / a;
}

void countWater() {
    waterCounter++;
}

void updateTEMP() {
    cycleCounter++;
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        airSensor[i].tempF = airSensor[i].sensor->readTemperature(true);
        airSensor[i].humid = airSensor[i].sensor->readHumidity();
    }

    if (isnan(airSensor[0].humid) || isnan(airSensor[0].tempF)) { //check for errors in two sensors
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
            //Serial.println(F("ERROR BOTH SENSORS "));
        } else {
                //Serial.print(F("DHT.main error! "));

            airSensor[0].humid = airSensor[1].humid;
            airSensor[0].tempF = airSensor[1].tempF;
        }
    } else {
        if (isnan(airSensor[1].humid) || isnan(airSensor[1].tempF)) {
                //Serial.print(F("DHT.upstairs error! "));

            airSensor[1].humid = airSensor[0].humid;
            airSensor[1].tempF = airSensor[0].tempF;
        }
    }

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {    //set new records, calculate EMAs
        if (airSensor[i].highest < airSensor[i].tempF) {
            airSensor[i].highest = airSensor[i].tempF;
        }
        if (airSensor[i].lowest > airSensor[i].tempF) {
            airSensor[i].lowest = airSensor[i].tempF;
        }
            //calculate EMAs
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
        if (floorSensor[i].lastEMA == 0)
            floorSensor[i].lastEMA = ReadFloorTemperature(floorSensor[i].PIN, 10);
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
        }
        else {
            floorSensor[0].currentEMA = floorSensor[1].currentEMA;
        }
    }
    floorEmaAvg = (floorSensor[0].currentEMA + floorSensor[1].currentEMA) / FLOOR_SENSOR_COUNT; //avg two readings

    if (tempDispCounter >= 4) { //every 4 reads = 10 seconds calculate trend and print info
        tempDispCounter = 0;

        for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {    //compute trend
            if (airSensor[i].currentEMA[0] < airSensor[i].currentEMA[1]) {  // if (now) temp < (medium avg) temp; temp is falling:
                if (airSensor[i].currentEMA[1] < weightedTemp) {  //if medium
                    if (airSensor[i].trendEMA <= -DOWNTREND) {
                        airSensor[i].trendEMA += DOWNTREND; //fast going back to 0
                    } else {
                        airSensor[i].trendEMA = 0;
                    }
                } else {
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
                    airSensor[i].trendEMA += UPTREND; //slow going up
                }
            }

            if (airSensor[i].trendEMA > AIR_TEMP_TREND_MINMAX) {
                airSensor[i].trendEMA = AIR_TEMP_TREND_MINMAX;
            }
            if (airSensor[i].trendEMA < -AIR_TEMP_TREND_MINMAX) {
                airSensor[i].trendEMA = -AIR_TEMP_TREND_MINMAX;
            }
        }
        
        avgTrend = (airSensor[0].trendEMA + airSensor[1].trendEMA) / 2.;

        if (tempDispCounter2 >= 6) {  // every minute:
            tempDispCounter2 = 0;
            
            if (changePerHourMinuteCounter < 59) {
              changePerHourMinuteCounter++;
            }
            
            for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
              last59MedEMAs[k] = last59MedEMAs[k+1];
            }
            last59MedEMAs[58] = (airSensor[0].currentEMA[1] + airSensor[1].currentEMA[1]) / 2;
        }
        
        tempDispCounter2++;
    }
    tempDispCounter++;

    //Set next pump state
    if (weightedTemp > tempSetPoint || (avgTrend > 0 && weightedTemp > (tempSetPoint - AIR_TEMP_TREND_FACTOR * avgTrend))) { //  check if should be off
        nextPumpState = 0;
    } else { //  else should be on:

        if (weightedTemp > tempSetPoint - 3 && currentPumpState <= 1) {  // if temp needs to move <3 degrees, turn on/start
            nextPumpState = 1;
        }else if (weightedTemp > tempSetPoint - 5 && currentPumpState <= 3) {  //if temp needs to move 3-5 degrees, go medium
            nextPumpState = 3; //  medium
        } else if (weightedTemp <= tempSetPoint - 5 && currentPumpState <= 4){                 // else, temp < set-5
            nextPumpState = 4; //  high
        }    
    }

    if (currentPumpState == 0 && nextPumpState != 0) {   // Force update if switching to on from off
        checkPumpCycleState();
    }
    
    if (tStatDispCtr >= 2) {
        tStatDispCtr = 0;

        uint16_t tankReading = analogRead(WATER_LEVEL_PIN);
        String waterDisplay = "";
        if (tankReading > 896) { //~1023
            waterDisplay = "<25%";
        } else if (tankReading > 657) { //~750
            waterDisplay = "<50%";
        } else if (tankReading > 267) { //~330
            waterDisplay = "<75%";
        } else { //~0
            waterDisplay = ">75%";
        }

        if (lcdPage == 0 || lcdPage == 2) {
            lcd.setCursor(0,0);
            lcd.print(airSensor[0].currentEMA[0],1);
            lcd.print(" ");         //added
            lcd.print(airSensor[1].currentEMA[0],1);
            lcd.print((char)223);
            lcd.print("F  c#");    //adjusted
            lcd.print(nextPumpState);  //cycle number, need to pull info in... restructure...
            lcd.setCursor(0,1);
            lcd.print("Set: ");
            lcd.print(tempSetPoint,1);
            lcd.print((char)223);
            lcd.print("F ");

            uint16_t mins = cycleCounter/24;
            if (mins > 999) {
                mins = 999;     //16.65hrs
            } else if (mins < 10) {
                lcd.print("  ");
            } else if (mins < 100) {
                lcd.print(" ");
            }
            lcd.print(mins);
            lcd.print("m");

        } else if (lcdPage == 1) { 
            lcd.setCursor(0,0);
            lcd.print("GH");
            lcd.print((char)223);
            lcd.print("F ");
            lcd.print("XX.X");
                //9 chars
            lcd.print("   Wtr ");

            lcd.setCursor(0,1);
            lcd.print("HUM% XX.X"); //9 chars
            lcd.print("   ");
            lcd.print(waterDisplay);

        } else if (lcdPage == 3) {
            lcd.setCursor(0,0);
            lcd.print("Ex");
            lcd.print((char)223);
            lcd.print("F ");
            lcd.print("XX.X");
                //9 chars
            lcd.print("       ");

            lcd.setCursor(0,1);
            lcd.print("HUM% XX.X"); //9 chars
            lcd.print("       ");
        
        }
        lcd.setCursor(0,1);
        lcd.print(lcdPage);     //temporary display of page # in bottom left

        lcdPage++;
        if (lcdPage >= 4) lcdPage = 0;

    }

    tStatDispCtr++;
}


void checkPumpCycleState() {

    // if (nextPumpState == 1) {
    //     if (currentPumpState == 1) {                                                //  continue from on to on
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[5]; //add a minute to continue same phase
    //     } else if (currentPumpState != 0) { //  on from start or warmup
    //         currentPumpState = 1;
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
    //         analogWrite(PUMP_PIN, 199);
    //     } else {        /*if (currentPumpState == (0)*/ //  start from off or warm up (currentPumpState == (0 || 3))
    //         currentPumpState = 2;                                       //  set new current pump state to START
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState]; //  add START interval
    //         cycleStartTime = currentTime;
    //         analogWrite(PUMP_PIN, 223);
    //         cycleCounter = 0;
    //     }

    // } else if (nextPumpState == 3) { // warm up changeto medium
    //     if (currentPumpState == 3) {                                                // continue same phase
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[5]; // add a minute to continue same phase
    //     } else { // start as different phase
    //         currentPumpState = 3;
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
    //         cycleStartTime = currentTime;
    //         analogWrite(PUMP_PIN, 223);
    //         cycleCounter = 0;
    //     }

    // } else if (nextPumpState == 4) { // warm up
    //     if (currentPumpState == 4) {                                                // continue same phase
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[5]; // add a minute to continue same phase
    //     } else { // start as different phase
    //         currentPumpState = 4;
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
    //         cycleStartTime = currentTime;
    //         analogWrite(PUMP_PIN, 255);
    //         cycleCounter = 0;
    //     }

    // } else if (nextPumpState == 0) { // off
    //     if (currentPumpState == 0) {                                                // continue same phase
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[5]; // add a minute to continue same phase
    //     } else { //start as different phase
    //         currentPumpState = 0;
    //         pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
    //         cycleStartTime = currentTime;
    //         analogWrite(PUMP_PIN, 0);
    //         cycleCounter = 0;
    //     }
    // }

    if (nextPumpState == currentPumpState) {
        pumpUpdateInterval = MINIMUM_CYCLE_TIMES[5]; // add a minute to continue same phase
    } else {
        if (nextPumpState == 1) {
            if (currentPumpState != 0) {    //  on from start or warmup
                currentPumpState = 1;
                pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
                analogWrite(PUMP_PIN, 199);

            } else {    //if (currentPumpState == (0)  start from off or warm up (currentPumpState == (0 || 3))
                currentPumpState = 2;                                       //  set new current pump state to START
                pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState]; //  add START interval
                cycleStartTime = currentTime;
                analogWrite(PUMP_PIN, 223);
                cycleCounter = 0;

            }
        } else if (nextPumpState == 3) {    // start as different phase
            currentPumpState = 3;
            pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
            cycleStartTime = currentTime;
            analogWrite(PUMP_PIN, 223);
            cycleCounter = 0;

        } else if (nextPumpState == 4) {    // start as different phase
            currentPumpState = 4;
            pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
            cycleStartTime = currentTime;
            analogWrite(PUMP_PIN, 255);
            cycleCounter = 0;

        } else if (nextPumpState == 0) {    //start as different phase
            currentPumpState = 0;
            pumpUpdateInterval = MINIMUM_CYCLE_TIMES[currentPumpState];
            cycleStartTime = currentTime;
            analogWrite(PUMP_PIN, 0);
            cycleCounter = 0;

        }
    }
}

void setup() { //************************************************************************************************ SETUP
    Serial.begin(9600);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FLOOR_TEMP_PIN[0], INPUT);
    pinMode(FLOOR_TEMP_PIN[1], INPUT);
    pinMode(THERMOSTAT_BUTTONS_PIN, INPUT);

    for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++) {
        dht[k].begin();
    }

    lcd.init();                      // initialize the lcd
    lcd.clear();
    lcd.backlight();  //open the backlight
    lcd.setCursor(0,0);
    lcd.print("Radiant Heat");
    
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
                for (uint8_t k = 0; k < 2; k++) { //reset record temps
                    airSensor[k].lowest = airSensor[k].tempF;
                }
            } else {
                buttonStatus = -1;
                for (uint8_t k = 0; k < 2; k++) {
                    airSensor[k].highest = airSensor[k].tempF;
                }
            }
            tempSetPoint += (0.5 * buttonStatus);
            
            //lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temp: ");
            lcd.print(airSensor[0].currentEMA[0],1);
            lcd.print((char)223);
            lcd.print("F");
            lcd.setCursor(0,1); //x, y
            lcd.print(" Set: ");
            lcd.print(tempSetPoint,1);
            lcd.print((char)223);
            lcd.print("F");

            lcdPage = 0;
            
            checkPump = currentTime + 3000; // wait 3 seconds before accepting new tempoerature in case button will be pressed more than once
        }
    }
} //************************************************************************************************* END LOOP