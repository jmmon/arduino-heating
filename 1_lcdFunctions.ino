

const byte customCharInside[8] = { // "inside"
  B00000,
  B00000,
  B00000,
  B10100,
  B01111,
  B11100,
  B01000,
  B01000
};

const byte customCharOutside[8] = { // "outside"
  B01010,
  B00110,
  B01110,
  B00000,
  B00000,
  B01111,
  B01000,
  B01000
};

const byte customCharDegF[8] = { // "degrees F"
  B01000,
  B10100,
  B01000,
  B00111,
  B00100,
  B00110,
  B00100,
  B00100
};

const byte customCharSmColon[8] = { // small colon + space
  B00000,
  B00000,
  B01000,
  B00000,
  B00000,
  B01000,
  B00000,
  B00000
};

const byte customCharDroplet[8] = { //water droplet
  B00100,
  B01110,
  B01010,
  B10111,
  B10111,
  B10111,
  B11111,
  B01110
};

const byte customCharSetpoint[8] = { // thermometer with indicator
  B00000,
  B01000,
  B11001,
  B01011,
  B11001,
  B01000,
  B10100,
  B01000
};


byte customCharSunAndHouse[8] = {
    B11000,
    B11000,
    B00010,
    B00111,
    B01111,
    B00101,
    B00111,
    B00111
};

// byte customCharSunAndHouseInvert[8] = {
//     B00111,
//     B00111,
//     B11101,
//     B11000,
//     B10000,
//     B11010,
//     B11000,
//     B11000
// };

byte customCharDotInsideHouse[8] = {
    B00100,
    B01010,
    B11111,
    B10001,
    B10001,
    B10101,
    B10001,
    B10001
};


void lcdInitialize() {
    lcd.init();                      // initialize the lcd
    lcd.clear();
    lcd.backlight();  //open the backlight

    lcd.createChar(1, customCharDegF);
    lcd.createChar(2, customCharInside);
    lcd.createChar(3, customCharOutside);
    lcd.createChar(4, customCharSmColon);
    lcd.createChar(5, customCharDroplet);
    lcd.createChar(6, customCharSetpoint);
    lcd.createChar(7, customCharSunAndHouse);
    lcd.createChar(8, customCharDotInsideHouse);
    
}







//**************************************************************************
// helper functions
//**************************************************************************
String lcdTankPercent() {    
    uint16_t LOW_BOUND = 350;
    uint16_t HIGH_BOUND = 750; // 743 ~ 0.98%
    uint8_t tankPercent;

    if (waterTank.ema >= HIGH_BOUND) {
        return "FF"; // full

    } else if (waterTank.ema > LOW_BOUND) {
        tankPercent = (float) 100 * (waterTank.ema - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND);
        String output = (tankPercent < 10) ? " " + String(tankPercent) : String(tankPercent);
        return output;
    } else if (waterTank.ema < 250) {
        return "ER"; // error
    }
    return "EE"; // empty
}


String lcdTankRead() {
    String output = ((waterTank.ema < 10) ? "   " 
        : (waterTank.ema < 100) ? "  "
        : (waterTank.ema < 1000) ? " "
        : ""); // 0 - 1023 // 13-16
    output += waterTank.ema; // 0 - 1023 // 13-16

    return output;
}


String calcTime(uint32_t t) {
    String output = "";
    // if (int32_t < 0) {
    //     output = "-";
    // } else {
    //     output = "+";
    // }
    uint16_t hours = t / 3600;
    t -= (hours * 3600);
    uint16_t minutes = t / 60;
    t -= (minutes * 60);
    uint16_t seconds = t;

    while (seconds >= 60) {
        minutes += 1;
        seconds -= 60;
    }
    while (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }

    output += ((hours < 10) ? "0" : "") + String(hours);
    // output += (String(hours) + ":");

    output += ((minutes < 10) ? ":0" : ":") + (String(minutes));
    // output += (String(minutes) + ":");

    output += ((seconds < 10) ? ":0" : ":") + String(seconds);
    // output += String(seconds);

    return output;
}













//**************************************************************************
// pages
//**************************************************************************


// ******************************************************************************************************************************
// main (indoor) Input / water page
void lcdPageTemperature() {
    lcd.setCursor(0,0);
    lcd.write(8); // inside icon //1
    lcd.print(Input, 1);
    lcd.print(" "); // 6
    lcd.print(air[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(air[1].currentEMA[0],1); // 10
    lcd.write(1); // degrees F // 11


    lcd.setCursor(0,1);

    lcd.write(6); // thermometer
    lcd.print(Setpoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");

    lcd.write(126); // pointing right
    lcd.print(Output, 2);

    lcd.setCursor(13,1);
    lcd.write(5); // water droplet // 14
    lcd.print(lcdTankPercent());
    
//     lcd.setCursor(11,1);
//     lcd.write(5); // water droplet // 12
//     lcd.print(lcdTankRead());
}


// ******************************************************************************************************************************
// PID info, cycle info
void lcdPagePumpCycleInfo() { // heating cycle page
    // display cycle info, time/duration
    lcd.setCursor(0,0); // line 1
    lcd.print(pump.getStatus()); // 5 chars
    lcd.print(F(" ")); // 6
    lcd.print(Kp, 1); // 7
    lcd.print(F(":")); // 
    lcd.print(Ki, 3); // (5) 13
    lcd.print(F(":")); // 
    lcd.print(Kd, 3); // (2) 


    lcd.setCursor(0,1); // line 2
    // cycle time
    lcd.print(calcTime(pump.getDuration())); // 8 chars
    lcd.print(F("")); // 9
    lcd.write(126); // pointing right 10
    lcd.print(Output); // (6) 16
}


// ******************************************************************************************************************************
// Total time and time spent with pump on/off
void lcdPageAccumTime() { // accumOn / Off
    lcd.setCursor(0,0); // top
    lcd.print("AccumT"); // 6
    lcd.write(4); // 7
    lcd.print(calcTime(pump.getAccumOn() + pump.getAccumOff())); // 16


    lcd.setCursor(0,1); //bot
    lcd.print("S"); // 7
    lcd.write(4); // 8
    lcd.print(pump.getState()); // 1
    lcd.print(" On"); // 7
    lcd.write(4); // 8

    int32_t dif = pump.getAccumOn() - pump.getAccumOff();
    if (dif < 0) {
        lcd.print(F("-"));
        dif *= -1;
    } else {
        lcd.print(F("+"));
    }
    lcd.print(calcTime(dif)); // 16
}


// ******************************************************************************************************************************
// Set Temperature page
void lcdPageSet(uint16_t btnRead = 0) { // set Input page    
    lcd.setCursor(0,0);
    lcd.print(F("Temp")); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(Input, 1); // 11
    lcd.write(1); // Replaces "*F" // 12
    lcd.print(F("      ")); // 16
    // lcd.print(pump.getTimeRemaining()); // countdown for screen hold


    lcd.setCursor(0,1);
    lcd.print(F("   "));  // 3
    lcd.write(6);
    lcd.print(Setpoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");
    // lcd.write(127); //pointing left // 9 // 7
    lcd.write(126); // pointing right
    lcd.print(Output, 2);
}


// ******************************************************************************************************************************
// Error page not currently used
void lcdPageErr(String err) {
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(err);


    lcd.setCursor(0,1);
    lcd.print(F("test")); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(err[3]);
}


// ******************************************************************************************************************************
// Show greenhouse and exterior sensor readings
void lcdPageGh() { 
    lcd.setCursor(0,0);
    lcd.print(F("GH"));
    lcd.write(4); //sm colon
    lcd.print(air[3].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[3].humid, 1); // 13
    lcd.print(F("%  ")); // 16

    
    lcd.setCursor(0,1);
    lcd.print(F(" "));
    lcd.write(7); // outside
    lcd.print(F(" "));
    lcd.print(air[2].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[2].humid, 1); // 13
    lcd.print(F("%  ")); // 16
}


// ******************************************************************************************************************************
// Total time and time spent above/below setpoint
void lcdPageAccumTarget() { // accumAbove/below
    lcd.setCursor(0,0); // top
    lcd.print(F("time")); // 7
    lcd.write(4); // 8
    lcd.print(calcTime(pump.getAccumAbove() + pump.getAccumBelow())); // 16


    lcd.setCursor(0,1); //bot
    lcd.write(6);
    lcd.write(4); // 8
    int32_t dif = pump.getAccumAbove() - pump.getAccumBelow();
    if (dif < 0) {
        lcd.print(F(" -"));
        dif *= -1;
    } else {
        lcd.print(F(" +"));
    }
    lcd.print(calcTime(dif)); // 16
}


// ******************************************************************************************************************************
// Water Filling Temp Page
void lcdPageWaterFilling() {
    lcd.setCursor(0,0); // top
    lcd.write(5);
    lcd.print(F("cur"));
    lcd.write(4);
    lcd.print(waterTank.ema);

    
    lcd.print(F(" dif"));
    lcd.write(4);
    lcd.print(waterTank.diff);

    
    lcd.setCursor(0,1); // bot
    //lcd.write(5);
    lcd.print(F(" slo"));
    lcd.write(4);
    lcd.print(waterTank.slowEma);

    
    lcd.print(F(" ema"));
    lcd.write(4);
    lcd.print(waterTank.diffEma);
}


// ******************************************************************************************************************************
// Floor reading display temp page
void lcdPageFloor() {
    lcd.setCursor(0,0); // top
    lcd.print(F("Floor1"));
    lcd.write(4); // sm colon // 7
    lcd.print(floorSensor[0].ema, 0); // +4

    lcd.setCursor(0,1); // bot
    lcd.print(F("     2"));
    lcd.write(4); // sm colon
    lcd.print(floorSensor[1].ema, 0);
}



// ***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************

// main functions


// note: lcd.write() {0~64 == ??}, {65~90 == A~Z}, {91~96 == ??}, {97~122 == a~z}, {123-127 == ??}
const uint8_t LCD_PAGE_MAX = 9;
uint8_t lcdPage = LCD_PAGE_MAX;


void lcdChangePage() {
    switch(lcdPage) {   // display proper page
        case(0):
                lcdPageTemperature();
            break;
        case(1):
                lcdPageWaterFilling();
            break;
        case(2):
                lcdPageGh();
            break;
        case(3):
                lcdPagePumpCycleInfo();
            break;
            
        case(4):
                lcdPageTemperature();
            break;
        case(5):
                lcdPageWaterFilling();
                
            break;
        case(6):
                lcdPageAccumTime();
            break;
        case(7):
                lcdPageAccumTarget();
            break;
        case(8):
                lcdPageFloor();
            break;
    }
}



// update LCD screen
void lcdUpdate() { // every 0.25s
    lcdCounter--;
    if (lcdCounter <= 0) {
        lcdCounter = LCD_INTERVAL_QTR_SECS;
 
        // switch page every 4 seconds
        lcdPage ++;
        if (lcdPage >= LCD_PAGE_MAX) {
            lcdPage = 0;
        }
    }
    

    if (!holdSetPage) {  // if not locking to the set page
        if (lcdCounter == LCD_INTERVAL_QTR_SECS) { // every 3.5 sec
            lcd.clear();
        }

        lcdChangePage(); // every sec
    } else { // if holding set page, set the page to 0
        lcdPageSet(0);
    }


    // WIP: Tank Is Being Filled indicator
    if (waterTank.filling) {
        lcd.setCursor(15,0); // top right
        lcd.write(5);
    }
}
