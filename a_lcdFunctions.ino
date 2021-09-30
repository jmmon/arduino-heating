void lcdInitialize() {
    lcd.init();                      // initialize the lcd
    lcd.clear();
    lcd.backlight();  //open the backlight
    
    //TESTING DISPLAY
    lcd.setCursor(0,0);
    lcd.print("TESTING DISPLAY");
    lcd.setCursor(2,1);
    lcd.print("Hello World");
    //TESTING DISPLAY

    lcd.createChar(1, customCharDegF);
    lcd.createChar(2, customCharInside);
    lcd.createChar(3, customCharOutside);
    lcd.createChar(4, customCharSmColon);
    lcd.createChar(5, customCharDroplet);
    
}


void lcdSwitchPage() {
    switch(lcdPage) {   // display proper page
        case(0):
                lcdPage1();
            break;

        case(1):
                lcdPage4();
            break;
        case(2):
                lcdPage1();
            break;

        case(3):
                lcdPage2();
            break;
        case(4):
                lcdPage1();
            break;
        case(5):
                lcdPage3();
            break;
    }
}

void lcdUpdate() {
    lcdCounter++;
    if (lcdCounter >= LCD_INTERVAL_SECONDS) { // update LCD
        lcdCounter = 0;

        lcdPage = (lcdPage == LCD_PAGE_MAX) ? 
            0: 
            (lcdPage + 1); // cycle page

        lcdSwitchPage(); // display page
    }
}



// pages
void lcdPage1() { // main (indoor) temperature / water page

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.write(2); // inside icon // 1
    lcd.print(air[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(air[1].currentEMA[0],1); // 10
    lcd.write(1); // degrees F // 11

    
    // lcd.write() {65~90 == A~Z}, {122~97 == z~a}

    lcd.print("     "); // 16

    lcd.setCursor(0,1);
    lcd.write(126); // pointing right
    lcd.print(" "); // 2
    lcd.print(tempSetPoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");
    lcd.write(127); //pointing left // 9

    // lcd.print("    "); // 13
    // lcdPrintTankPercent();

    
    lcd.print("  "); // 11
    lcdPrintTankRead();
}


void lcdPage2() { // heating cycle page
    //display cycle info, time/duration
    lcd.clear();
    lcd.setCursor(0,0); // line 1
    lcd.print(CYCLE[currentPumpState].NAME); //11 chars

    lcd.print(F(" ")); //12
    lcd.print(F("PWM")); // 15
    lcd.write(4); // 16



    lcd.setCursor(0,1); // line 2
    //cycle time
    uint32_t t = cycleDuration;
    uint16_t hours = t / 3600000;
    t -= (hours * 3600000);
    uint16_t minutes = t / 60000;
    t -= (minutes * 60000);
    uint16_t seconds = t / 1000;
    t -= (seconds * 1000);
    if (seconds >= 60) {
        minutes += 1;
        seconds -= 60;
    }
    if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }
    if (t >= 500 && (minutes > 0 || hours > 0)) {
        seconds += 1;
    }

    lcd.print(F("t"));
    lcd.write(4); //2 chars

    if (hours > 0) {
        if (hours < 10) {
            lcd.print(F("0"));
        }
        lcd.print(hours);
        lcd.print(F(":")); // max 8 chars
    }

    if (minutes > 0 || hours > 0) { 
        if (minutes < 10) {
            lcd.print(F("0"));
        }
        lcd.print(minutes);
        lcd.print(F(":")); // max 5 chars
    }

    if (minutes > 0 || hours > 0 || seconds > 0) { 
        if (seconds < 10) {
            lcd.print(F("0"));
        }   
        lcd.print(seconds); //max 8 chars
    }
    
    if (hours == 0 && minutes == 0) {
        if (seconds == 0) {
            lcd.print(F("0"));
        }
        lcd.print(F(","));
        lcd.print(t); // min 5 chars
    }
// so 7 - 10, leaves 6


    lcd.print(F("   ")); //3
    
    lcd.print((CYCLE[currentPumpState].PWM < 10)?"  ":""); //pwm is either >198 or 0
    lcd.print(CYCLE[currentPumpState].PWM); // 3 chars
}


void lcdPage3() { // PID + PWM display for testing / adjustment
    lcd.clear();

    lcd.setCursor(0,0); //top
    lcd.print("p"); //
    lcd.write(4);
    lcd.print(KP, 3);

    lcd.print(" i"); // 
    lcd.write(4);
    lcd.print(KI, 3);


    

    lcd.setCursor(0,1); //bot
    lcd.print(" d");
    lcd.write(4);
    lcd.print(KD);

    lcd.print("  => "); // 
    lcd.print(outputVal); //0-255
}



void lcdPageSet() { // set temperature page
    lcd.clear();    
    lcd.setCursor(0,0);
    lcd.print("Temp"); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(air[0].currentEMA[0],1); // 11
    lcd.write(1); // Replaces "*F" // 12
    lcd.print("    "); // 16

    lcd.setCursor(0,1);
    lcd.print("   ");  // 6
/*     lcd.write(4); // Replaces ": " // 7 */
    lcd.write(126); // pointing right
    lcd.print(" "); // 8
    lcd.print(tempSetPoint,1); // 12
    lcd.write(1); // degrees F // 13
    lcd.print(" "); // 14
    lcd.write(127); //pointing left // 15
    lcd.print("    "); // 16
}




void lcdPage4() { // page for greenhouse and exterior sensors
    lcd.clear();    
    lcd.setCursor(0,0);
    lcd.print(F("GH"));
    lcd.write(4);
    lcd.print(air[2].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[2].humid, 1); // 13
    lcd.print(F("%  ")); // 16


    
    lcd.setCursor(0,1);
    lcd.print(F(" "));
    lcd.write(3);
    lcd.print(F(" "));
    lcd.print(air[3].currentEMA[0], 1); // 7
    lcd.write(1);
    lcd.print(F(" ")); // 9
    lcd.print(air[3].humid, 1); // 13
    lcd.print(F("%  ")); // 16

}



// helpers
void lcdPrintTankPercent() {
    uint16_t tankRead = analogRead(WATER_LEVEL_PIN);
    lcd.write(5); // water droplet // 14
    if (tankRead <= 350) {
        lcd.print("EE");
    } else if (tankRead >= 1015) {
        lcd.print("FF");
    } else {
        uint16_t LOW_BOUND = 350;
        float tankPercent = (tankRead - LOW_BOUND) / (1023 - LOW_BOUND) * 100;
        // lcd.print((tankPercent < 10) ? " " + tankPercent : tankPercent); // 0 - 99 // 15-16
        lcd.print((tankPercent < 10) ? " " : ""); // 0 - 99 // 15-16
        lcd.print(tankPercent); // 0 - 99 // 15-16
    }
    // lcd.print(tankRead);
}


void lcdPrintTankRead() {
    uint16_t tankRead = analogRead(WATER_LEVEL_PIN);
    lcd.write(5); // water droplet // 14

    lcd.print((tankRead < 10) ? "   " 
        : (tankRead < 100) ? "  "
        : (tankRead < 1000) ? " "
        : ""); // 0 - 1023 // 13-16
    lcd.print(tankRead); // 0 - 1023 // 13-16
}