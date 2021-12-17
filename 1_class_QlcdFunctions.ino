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


const byte customCharSunAndHouse[8] = {
    B11000,
    B11000,
    B00010,
    B00111,
    B01111,
    B00101,
    B00111,
    B00111
};

const byte customCharDotInsideHouse[8] = {
    B00100,
    B01010,
    B11111,
    B10001,
    B10001,
    B10101,
    B10001,
    B10001
};


class Display_c {
    private:
        const uint8_t PIN = A2;

        const uint8_t LCD_INTERVAL_QTR_SECS = 14; // 3.5s
        const uint8_t LCD_PAGE_MAX = 6;
        uint8_t lcdPage = LCD_PAGE_MAX;
        int8_t lcdCounter = 0;
        bool holdSetPage = false;

    public:
        String tankPercent = "";

        Display_c() { // constructor
            pinMode(PIN, INPUT);
        }

        void initialize() {
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

        void updateSetPoint() {
            int8_t buttonIncDec = 0;
            uint16_t buttonRead = analogRead(PIN);
            if (buttonRead > 63) {
                if (DEBUG) Serial.println(buttonRead);

                if (buttonRead > 850) {
                    buttonIncDec = 1;
                    for (uint8_t k = 0; k < 2; k++) {  // reset record temps
                        air[k].lowest = air[k].tempF;
                    }
                } else {
                    buttonIncDec = -1;
                    for (uint8_t k = 0; k < 2; k++) {
                        air[k].highest = air[k].tempF;
                    }
                }
                
                Setpoint += double(0.5 * buttonIncDec);

                lcd.clear();
                pageSet();
                holdSetPage = true;
                pump.checkAfter(); // default 3 seconds
            }
        }

        String formatTime(uint32_t t) {
            // get base time
            uint16_t hours = t / 3600;
            t -= (hours * 3600);
            uint16_t minutes = t / 60;
            t -= (minutes * 60);
            uint16_t seconds = t;

            // rollover if necessary
            while (seconds >= 60) {
                minutes += 1;
                seconds -= 60;
            }
            while (minutes >= 60) {
                hours += 1;
                minutes -= 60;
            }

            // format output
            String output = "";
            output += ((hours < 10) ? "0" : "") + String(hours);
            output += ((minutes < 10) ? ":0" : ":") + (String(minutes));
            output += ((seconds < 10) ? ":0" : ":") + String(seconds);

            return output;
        }

        String formatHours(uint32_t t) { // hours with 1 decimal place
            float hours = t / 3600;   // get tenths of hours
            return String(hours, 1);  // turn into hours with tenths and limit to 1 decmial
        }



        void printPage() {
            switch(lcdPage) {   // display proper page
                case(0):
                        pageTemperature();
                    break;
                case(1):
                        pageGh();
                    break;
                case(2):
                        pagePumpCycleInfo();
                    break;
                    
                case(3):
                        pageTemperature();
                    break;
                case(4):
                        pageAccumTime();
                    break;
                case(5):
                        pageWaterFilling();
                    break;
            }
        }

        void update() { // every 0.25s
            updateSetPoint();
            if (pump.state != 3 && holdSetPage) {
                holdSetPage = false;
            }

            lcdCounter--;
            if (lcdCounter <= 0) {
                lcdCounter = LCD_INTERVAL_QTR_SECS;
        
                // switch page every 3.5 seconds
                lcdPage ++;
                if (lcdPage >= LCD_PAGE_MAX) {
                    lcdPage = 0;
                }
            }

            if (!holdSetPage) {  // if not locking to the set page
                if (lcdCounter == LCD_INTERVAL_QTR_SECS) { // every 3.5 sec
                    lcd.clear();
                }
                printPage(); // every sec

            } else { // display lcdPageSet
                pageSet();
            }

            if (waterTank.filling) { // WIP: Tank Is Being Filled indicator
                lcd.setCursor(15,0); // top right
                lcd.write(5);
            }
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Pages:
        void pageTemperature() {
            // first line
            lcd.setCursor(0,0);
            lcd.write(8); // inside icon //1
            lcd.print(Input, 1);
            lcd.print(F(" ")); // 6
            lcd.print(air[0].currentEMA[0],1); // 5
            lcd.print(F("/")); // 6
            lcd.print(air[1].currentEMA[0],1); // 10
            lcd.write(1); // degrees F // 11

            // second line
            lcd.setCursor(0,1);
            lcd.write(6); // thermometer
            lcd.print(Setpoint,1); // 6
            lcd.write(1); // degrees F // 7
            lcd.print(" ");
            lcd.write(126); // pointing right
            lcd.print(Output, 2);
            lcd.setCursor(13,1);
            lcd.write(5); // water droplet // 14
            lcd.print(waterTank.percent);
        }

        void pagePumpCycleInfo() { // heating cycle page
            // display cycle info, time/duration
            lcd.setCursor(0,0); // line 1
            lcd.print(pump.getStatus()); // 3 chars        
            lcd.print(F(" ")); // 4
            lcd.print(formatTime(pump.duration)); // 8 chars (12)

            lcd.setCursor(0,1); // top
            lcd.print(F("RunTtl")); // 6
            lcd.write(4); // 7
            lcd.print(formatHours(pump.time)); // 15
        }

        
        void pageAccumTime() { // Total time and time spent with pump on/off
            lcd.setCursor(0,0); // top
            lcd.write(6); // setpoint (thermometer)
            lcd.write(4); // 8
            int32_t dif = pump.accumAbove - (pump.time - pump.accumAbove);
            if (dif < 0) {
                lcd.print(F(" -"));
                dif *= -1;
            } else {
                lcd.print(F(" +"));
            }
            lcd.print(formatHours(dif)); // 16

            lcd.setCursor(0,1); //bot
            lcd.print(F("S")); // 7
            lcd.write(4); // 8
            lcd.print(pump.state); // 1

            dif = pump.accumOn - (pump.time - pump.accumOn);
            if (dif < 0) {
                lcd.print(F(" Off")); // 7
                dif *= -1;
            } else {
                
                lcd.print(F("  On")); // 7
            }
            lcd.write(4); // 8
            lcd.print(formatHours(dif)); // 16
        }

        void pageSet() { // Set Temperature page   
            lcd.setCursor(0,0); // top
            lcd.print(F("Temp")); // 6
            lcd.write(4); // sm colon  // 7
            lcd.print(Input, 1); // 11
            lcd.write(1); // Replaces "*F" // 12
            lcd.print(F("      ")); // 16

            lcd.setCursor(0,1); // bot
            lcd.print(F("   "));  // 3
            lcd.write(6);
            lcd.print(Setpoint,1); // 6
            lcd.write(1); // degrees F // 7
            lcd.print(" ");
            // lcd.write(127); //pointing left // 9 // 7
            lcd.write(126); // pointing right
            lcd.print(Output, 2);
        }

        void pageGh() { // Show greenhouse and exterior sensor readings
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

        void pageWaterFilling() { // Water Filling Temp Page
            lcd.setCursor(0,0); // top
            lcd.write(5);
            lcd.print(waterTank.ema); // 4
            lcd.print(F(":"));
            lcd.print(waterTank.slowEma); // 8
            lcd.print(F(" "));
            lcd.write(126); // pointing right
            if (waterTank.diffEma < 10) {
                lcd.print(F("  "));
            } else {
                lcd.print(F(" "));
            }
            lcd.print(waterTank.diffEma); // 13

            lcd.setCursor(0,1); // top
            lcd.print(F("Flr"));
            lcd.write(4); // sm colon // 4
            lcd.print(floorSensor[0].ema, 0); // +3
            lcd.print(F(":"));
            lcd.print(floorSensor[1].ema, 0);
        }

} display = Display_c();

// // note: lcd.write() {0~64 == ??}, {65~90 == A~Z}, {91~96 == ??}, {97~122 == a~z}, {123-127 == ??}
