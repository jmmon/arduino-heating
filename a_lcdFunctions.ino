
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
    lcd.print(temperatureSetPoint,1); // 12
    lcd.write(1); // degrees F // 13
    lcd.print(" "); // 14
    lcd.write(127); //pointing left // 15
    lcd.print("    "); // 16
}

void lcdPage1() { // main (indoor) temperature / water page

    lcd.setCursor(0,0);
    lcd.write(2); // inside icon // 1
    lcd.print(airSensor[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(airSensor[1].currentEMA[0],1); // 10
    lcd.write(1); // degrees F // 11

    
//  {65~90 == A~Z}, {122~97 == z~a}
    
//    lcd.write(65);
//    
//    lcd.write(64);
//    
//    lcd.write(63);
//    
//    lcd.write(61); //
//    
//    lcd.write(62); //
    lcd.print("     "); // 16


    lcd.setCursor(0,1);
/*     lcd.print("Set"); // 3
    lcd.write(4); // sm colon // 4 */
    // lcd.write(126); // pointing right
    // lcd.print(" "); // 2
    lcd.print(temperatureSetPoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");
    // lcd.write(127); //pointing left // 9

    // lcd.print("    "); // 13
    lcd.print(" "); // 13
    lcdPrintTankPercent();


    // lcd.print("  "); // 11
    // lcd.write(5); // water droplet // 12
    // lcd.print((tankRead < 10) ? "   " : (tankRead < 100) ? "  " : (tankRead < 1000) ? " " : "");
    // lcd.print(tankRead); // 0 - 1023 // 16
}

void lcdPrintTankPercent() {
    uint16_t tankRead = analogRead(WATER_LEVEL_PIN);
    lcd.write(5); // water droplet // 14
    if (tankRead <= 350) {
        lcd.print("EE");
    } else if (tankPercent >= 1015) {
        lcd.print("FF");
    } else {
        uint16_t tankPercent = (tankRead - 350) / (1023 - 350) * 100;
        lcd.print((tankPercent < 10) ? " " + tankPercent : tankPercent); // 0 - 99 // 15-16
    }
}


void lcdPage2() { // heating alternate page
    // 5 and 7 spots to work with if I keep the first main stuff
    // display duration in this mode, floor reading? (probably not necessary)

    // uint16_t tankPercent = ((analogRead(WATER_LEVEL_PIN) - 350) / (1023 - 350)) * 100;
    uint16_t tankRead = analogRead(WATER_LEVEL_PIN);

    lcd.setCursor(0,0);
    lcd.write(2); // inside icon // 1
    lcd.print(airSensor[0].currentEMA[0],1); // 5
    lcd.print("/"); // 6
    lcd.print(airSensor[1].currentEMA[0],1); // 10
    lcd.write(1); // degrees F // 11
    lcd.print("     "); // 16

    lcd.setCursor(0,1);
    /*     lcd.print("Set"); // 3
    lcd.write(4); // sm colon // 4 */
    lcd.write(126); // pointing right
    lcd.print(" "); // 2
    lcd.print(temperatureSetPoint,1); // 6
    lcd.write(1); // degrees F // 7
    lcd.print(" ");
    lcd.write(127); //pointing left // 9

    lcd.print("    "); // 13
    lcdPrintTankPercent(); // 14-16

    //  lcd.print("  "); // 11
    //  lcd.write(5); // water droplet // 12
    //  lcd.print((tankRead < 10) ? "   " : (tankRead < 100) ? "  " : (tankRead < 1000) ? " " : "");
    //  lcd.print(tankRead); // 0 - 1023 // 16
}

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
