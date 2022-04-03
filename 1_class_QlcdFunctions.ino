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
        const uint8_t PIN = T_STAT_BUTTON_PIN;
				const uint8_t VALVE_PIN = WATER_VALVE_PIN;
        const float SETPOINT_ADJ_PER_TICK = 1;      // degrees change
        const uint8_t SETPOINT_TIMER_INTERVAL = 2;  // n * 0.25 seconds
        uint8_t setpointTimer = SETPOINT_TIMER_INTERVAL;

        uint16_t lastButtonRead = 0;

        const uint8_t LCD_INTERVAL_QTR_SECS = 14; // 3.5s
        const uint8_t LCD_PAGE_MAX = 7; // pages per cycle
        uint8_t lcdPage = -1; // + 1 will make it start at page 0
        int8_t lcdCounter = 0;
        bool holdSetPage = false;


        const uint16_t TONE_DURATION = 180; // seconds
				const uint16_t TONE_DURATION_FULL = 60;
				const uint8_t TONE_TIMER_SPACING_SLOW = 8;
				const uint8_t TONE_TIMER_SPACING_FAST = 4;
				const uint8_t TONE_TIMER_SPACING_SOLID = 1;
        uint16_t toneTimer = 0;
				bool toneTimerSpacing = 0;
				
				double * TONE_NOTE = NOTE_C5;


				bool valveClosed = false;
				uint16_t valveTimer = 0;
				const uint16_t VALVE_TIMER_DURATION = 21600; // seconds (6 hours)

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
						
						// either button also turns off water filling alarm!
            if (toneTimer > 0) toneTimer = 0;
//						toneTimer = 0;
						
						if (lastButtonRead > 63) { // delays adjustment by 1 tick
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
						}
						
						
						Setpoint += double(SETPOINT_ADJ_PER_TICK * buttonIncDec);

						lcd.clear();
						pageSet();
						holdSetPage = true;
						pump.checkAfter(); // default 3 seconds
					}
					lastButtonRead = buttonRead;
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

            // uint8_t pages[] = {
            //     0,
            //     1,
            //     2,
            //     0, 
            //     3, 
            //     4, 
            //     5
            // };
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

                case(6):
                        pageWaterFlowCounter();
                    break;
            }
        }

        void update() { // every 0.25s
            // setpoint delay
            setpointTimer --;
            if (setpointTimer <= 0) {
                updateSetPoint();
                setpointTimer = SETPOINT_TIMER_INTERVAL;
            }
            
            if (pump.state != 3 && holdSetPage) {
                holdSetPage = false;
            }

            lcdCounter --;
            if (lcdCounter <= 0) {
                lcdCounter = LCD_INTERVAL_QTR_SECS;
        
                // switch page every 3.5 seconds
                lcdPage ++;
                if (lcdPage >= LCD_PAGE_MAX) {
                    lcdPage = 0;
                }
            }
            
            // if not locking on set-temp page, clear if needed, and refresh/print the page
            if (!holdSetPage) {
                if (lcdCounter == LCD_INTERVAL_QTR_SECS) { // every 3.5 sec
                    lcd.clear();
                }
                printPage(); // every sec

            } else { // display lcdPageSet if locked to that page
                pageSet();
            }

            if (waterTank.filling) { // WIP: Tank Is Being Filled indicator
                lcd.setCursor(15,0); // top right
                lcd.write(5);
            }



						// WATER TANK FILLING ALARM:
						// notes: 95%+ beeping works; but once tank hits full we turn off 'filling' so the 100% filled beeping never triggers. Instead, once the tank hits full, the beeping stops. Then, as the tank values level, they might drop to 99%, which starts again the slow beeping, because 'filling' now turns back on now that the difference between water tank levels is still over the threshold. So until the water tank level difference reduces below the threshold, the slow beeping will turon on and off as the water tank level dances between 99% and 100%.

						// also, tank filled mark still could increase another bit more. Tank hits ~745 then dances down to ~700, since the tank actually is not quite to the max level.


						if (waterTank.filling) {
							if (waterTank.displayPercent >= 98) { // fast beeping above 98%
								toneTimer = TONE_DURATION * 4; // since it triggers every quarter second;
								toneTimerSpacing = TONE_TIMER_SPACING_FAST;
							} else if (waterTank.displayPercent >= 95) { // slow beeping above 95%
								toneTimer = TONE_DURATION * 4; // since it triggers every quarter second;
								toneTimerSpacing = TONE_TIMER_SPACING_SLOW;
							}
						} else if (waterTank.newlyFull) { // moment it hits 100%
							waterTank.newlyFull = false;
							toneTimer = TONE_DURATION_FULL * 4; // since it triggers every quarter second;
							toneTimerSpacing = TONE_TIMER_SPACING_SOLID;


							// close our valve
							valveClosed = true;
							valveTimer = VALVE_TIMER_DURATION;
						}

            // sound our tone when our timer is set
            if (toneTimer > 0) {
							if (toneTimer % toneTimerSpacing == 0) {
								tone(TONE_PIN, TONE_NOTE, 250); // play tone
							}
							toneTimer--;
            }


						if (valveTimer > 0) {
							valveTimer--;
						} else {
							valveClosed = false;
						}

						if (valveClosed) {
							// send signal to close valve
							// digitalWrite(WATER_VALVE_PIN, HIGH)

						} else {
							// digitalWrite(WATER_VALVE_PIN, LOW)
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
    // old
            // lcd.print(waterTank.displayPercent);
    // new
            // handle conversion / formatting:
            String displayPercent = "--";
            switch(waterTank.displayPercent) {
                case(100) : displayPercent = "FF";
                break;
                case(0) : displayPercent = "EE";
                break;
                case(101) : displayPercent = "ER";
                break;
                case(255) : displayPercent = "--";
                break;
                default: displayPercent = (waterTank.displayPercent < 10) ? " " + String(waterTank.displayPercent) : String(waterTank.displayPercent);
            }
            lcd.print(displayPercent);
        }

        void pagePumpCycleInfo() { // heating cycle page
            // display cycle info, time/cycleDuration
            lcd.setCursor(0,0); // line 1
            lcd.print(pump.getStatus()); // 3 chars        
            lcd.print(F(" ")); // 4
            lcd.print(formatTime(pump.cycleDuration)); // 8 chars (12)

            lcd.setCursor(0,1); // bot
            lcd.print(F("RunTtl")); // 6
            lcd.write(4); //small colon // 7
            uint32_t totalTime = currentTime / 1000;
            lcd.print(formatHours(totalTime)); // 15
        }

        void pagePidInfo() {
            lcd.setCursor(0, 0);
            lcd.print(F("Kp")); 
            lcd.write(4); 
            lcd.print(Kp, 0);
            lcd.print(F(" Ki")); 
            lcd.write(4); 
            lcd.print(Ki, 4); //space for 8

            lcd.setCursor(0, 1);
            lcd.print(F("Kd")); 
            lcd.write(4); 
            lcd.print(Kd, 0);
            lcd.print(F(" Outpt")); 
            lcd.write(4); 
            lcd.print(Output);
        }

        
        void pageAccumTime() { // Total time and time spent with pump on/off
            lcd.setCursor(0,0); // top
            lcd.write(6); // setpoint (thermometer)
            lcd.write(4); // 8
            uint32_t totalSeconds = currentTime / 1000;
            int32_t dif = pump.accumAbove - (totalSeconds - pump.accumAbove);
            if (dif < 0) {
                lcd.print(F(" -"));
                dif *= -1;
            } else {
                lcd.print(F(" +"));
            }
            lcd.print(formatHours(dif)); // 16


            lcd.setCursor(0,1); //bot
            lcd.print(F("State")); // 7
            lcd.write(4); // 8
            lcd.print(pump.state); // 1

            dif = pump.accumOn - (totalSeconds - pump.accumOn);
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

        void pageWaterFlowCounter() {
            // flow rate
            lcd.setCursor(0, 0);
            lcd.print(F("WtrRate"));
            lcd.write(4);
            lcd.print(flowRate, 2);
            lcd.print(F(" l/m"));

            // total water use
            lcd.setCursor(0, 1);
            lcd.print(F("WtrTtl"));
            lcd.write(4);
            lcd.print(totalMilliLitres / 1000, 3);
            lcd.print(F(" l"));
        }

} display = Display_c();

// // note: lcd.write() {0~64 == ??}, {65~90 == A~Z}, {91~96 == ??}, {97~122 == a~z}, {123-127 == ??}
