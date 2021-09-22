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
                lcdPage3();
            break;
    }
}

void lcdUpdate() {
    if (currentTime >= (lastLcdUpdate + LCD_UPDATE_INTERVAL)) { // update LCD
        lastLcdUpdate += LCD_UPDATE_INTERVAL;

        lcdPage = (lcdPage == LCD_PAGE_MAX) ? 0: (lcdPage + 1); // cycle page

        lcdSwitchPage(); // display page
    }
}


void lcdPage1() { // main (indoor) temperature / water page

    lcd.setCursor(0,0);
    lcd.write(2); // inside icon // 1
    lcd.print(airSensor[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(airSensor[1].currentEMA[0],1); // 10
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
    lcd.setCursor(0,0);
    

    lcd.setCursor(0,1);
    
}


void lcdPage3() { // PID + PWM display for testing / adjustment
    lcd.clear();

    lcd.setCursor(0,0); //top
    lcd.print("P"); //
    lcd.write(4);
    lcd.print(KP);

    lcd.print(" I"); // 
    lcd.write(4);
    lcd.print(KI);


    

    lcd.setCursor(0,1); //bot
    lcd.print(" D");
    lcd.write(4);
    lcd.print(KD);

    lcd.print(" PWM"); // 
    lcd.write(4);
    lcd.print(outputVal); //0-255
}



void lcdPageSet() { // set temperature page
    lcd.setCursor(0,0);
    lcd.print("Temp"); // 6
    lcd.write(4); // sm colon  // 7
    lcd.print(airSensor[0].currentEMA[0],1); // 11
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