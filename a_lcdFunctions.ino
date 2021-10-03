

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



void lcdInitialize() {
    lcd.init();                      // initialize the lcd
    lcd.clear();
    lcd.backlight();  //open the backlight

    lcd.createChar(1, customCharDegF);
    lcd.createChar(2, customCharInside);
    lcd.createChar(3, customCharOutside);
    lcd.createChar(4, customCharSmColon);
    lcd.createChar(5, customCharDroplet);
    
}


void lcdUpdate() {
    lcdCounter--;
    if (lcdCounter <= 0) { // update LCD
        lcdCounter = LCD_INTERVAL_SECONDS;
        
        lcdPage ++;
        if (lcdPage >= LCD_PAGE_MAX) { // page 4 == page 0
            lcdPage = 0;
        }
        if (!set) {
            lcd.clear();
        }

    }
    if (!set) {
        lcdSwitchPage(); // display page
    }
}


void lcdSwitchPage() {
    lcd.clear();
    switch(lcdPage) {   // display proper page
        case(0):
                lcdPageTemperature();
            break;
        case(1):
                lcdPageGh();
            break;
        case(2):
                lcdPagePumpCycleInfo();
            break;

        case(3):
                lcdPageTemperature();
            break;
        case(4):
                lcdPageAccumTime();
            break;
    }
}



// helpers
String lcdTankPercent() {
    if (lastTankRead == 0) lastTankRead = analogRead(WATER_FLOAT_PIN);
    uint16_t tankRead = (analogRead(WATER_FLOAT_PIN) * (2. / (1 + 30)) + lastTankRead * (1 - (2. / (1 + 30))));
    lastTankRead = tankRead;
    uint16_t LOW_BOUND = 350;
    uint16_t HIGH_BOUND = 800;

    String output = "EE";
    if (tankRead >= HIGH_BOUND) {
        output = "FF";
    } else if (tankRead > LOW_BOUND) {
        float tankPercent = (tankRead - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND) * 100;
        
        output = (tankPercent < 10) ? " " : "";
        output += String(tankPercent);
    }
    return output;
}


String lcdTankRead() {
    if (lastTankRead == 0) lastTankRead = analogRead(WATER_FLOAT_PIN);
    uint16_t tankRead = (analogRead(WATER_FLOAT_PIN) * (2. / (1 + 30)) + lastTankRead * (1 - (2. / (1 + 30))));
    lastTankRead = tankRead;
    
    String output = ((tankRead < 10) ? "   " 
        : (tankRead < 100) ? "  "
        : (tankRead < 1000) ? " "
        : ""); // 0 - 1023 // 13-16
    output += tankRead; // 0 - 1023 // 13-16
    return output;
}



String calcTime(uint32_t t) {
    uint16_t hours = t / 3600;
    t -= (hours * 3600);
    uint16_t minutes = t / 60;
    t -= (minutes * 60);
    uint16_t seconds = t;

    if (seconds >= 60) {
        minutes += 1;
        seconds -= 60;
    }
    if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }

    String output;
    if (hours < 10) {
        output += '0';
    }
    output += (String(hours) + ':');
    if (minutes < 10) {
        output += '0';
    }
    output += (String(minutes) + ':');
    if (seconds < 10) {
        output += '0';
    }
    output += String(seconds);

    return output;
}







//**************************************************************************
// pages
//**************************************************************************
void lcdPageTemperature() { // main (indoor) Input / water page
    lcd.setCursor(0,0);
    lcd.write(2); // inside icon // 1
    lcd.print(Input, 1);
    lcd.print(" "); // 6
    lcd.print(air[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(air[1].currentEMA[0],1); // 10
    lcd.write(1); // degrees F // 11


    lcd.setCursor(0,1);
    lcd.write(126); // pointing right
    lcd.print(" "); // 2
    lcd.print(Setpoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");
    lcd.write(127); //pointing left // 9
    lcd.print("    "); // 13
    lcd.write(5); // water droplet // 14
    lcd.print(lcdTankPercent());
    // lcd.print("  "); // 11
    // lcd.write(5); // water droplet // 12
    // lcd.print(lcdTankRead());
}


void lcdPagePumpCycleInfo() { // heating cycle page
    // display cycle info, time/duration
    lcd.setCursor(0,0); // line 1
    lcd.print(pump.getStatus()); // 5 chars
    lcd.print(F(" ")); // 6
    lcd.print(Kp, 1); // 7
    lcd.print(F(":")); // 
    lcd.print(Ki, 3); // (5) 13
    lcd.print(F(":")); // 
    lcd.print(Kd, 1); // (2) 


    lcd.setCursor(0,1); // line 2
    // cycle time
    lcd.print(calcTime(pump.getDuration())); // 8 chars
    lcd.print(F(" ")); // 9
    lcd.write(126); // pointing right 10
    lcd.print(Output); // (6) 16
}


void lcdPageAccumTime() { // accumOn / Off
    lcd.setCursor(0,0); // top
    lcd.print("Time On"); // 7
    lcd.write(4); // 8
    lcd.print(calcTime(pump.getAccumOn())); // 16


    lcd.setCursor(0,1); //bot
    lcd.print("S"); // 7
    lcd.write(4); // 8
    lcd.print(pump.getState()); // 1
    lcd.print(" Off"); // 7
    lcd.write(4); // 8
    lcd.print(calcTime(pump.getAccumOff())); // 16
}



void lcdPageSet() { // set Input page    
    lcd.setCursor(0,0);
    lcd.print(F("Temp")); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(Input,1); // 11
    lcd.write(1); // Replaces "*F" // 12
    lcd.print(F("      ")); // 16


    lcd.setCursor(0,1);
    lcd.print(F("   "));  // 6
    lcd.write(126); // pointing right
    lcd.print(F(" ")); // 8
    lcd.print(Setpoint,1); // 12
    lcd.write(1); // degrees F // 13
    lcd.print(F(" ")); // 14
    lcd.write(127); //pointing left // 15
    lcd.print(F("    ")); // 16
}

void lcdPageErr(String err) {
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(err);


    lcd.setCursor(0,1);
    lcd.print(F("test")); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(err[3]);
}


void lcdPageGh() { // page for greenhouse and exterior sensors    
    lcd.setCursor(0,0);
    lcd.print(F("GH"));
    lcd.write(4);
    lcd.print(air[3].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[3].humid, 1); // 13
    lcd.print(F("%  ")); // 16

    
    lcd.setCursor(0,1);
    lcd.print(F(" "));
    lcd.write(3);
    lcd.print(F(" "));
    lcd.print(air[2].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[2].humid, 1); // 13
    lcd.print(F("%  ")); // 16
}




// lcd.write() {65~90 == A~Z}, {122~97 == z~a}
