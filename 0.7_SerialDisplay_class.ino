// const float SETPOINT_ADJ_PER_TICK = 1;		 // degrees change
// const uint8_t SETPOINT_TIMER_INTERVAL = 2; // quarter seconds
//
// const uint8_t LCD_INTERVAL_QTR_SECS = 14; // 3.5s
//
// const uint16_t TONE_DURATION = 180;			// seconds
// const uint16_t TONE_DURATION_FULL = 60; // when hits 100%, beep for this long
// const uint8_t TONE_TIMER_SPACING_SLOW = 16;
// const uint8_t TONE_TIMER_SPACING_FAST = 4;
// const uint8_t TONE_TIMER_SPACING_SOLID = 1;
//
// const uint8_t TONE_TIMER_TRIGGER_PERCENT_SLOW = 90;
// const uint8_t TONE_TIMER_TRIGGER_PERCENT_FAST = 95;
//
// const uint16_t VALVE_CLOSE_DURATION = 21600; // seconds (6 hours)

/* 
For "DEBUG" mode aka printing to Serial instead of to the LCD display
Should act very similar to the Tstat LCD display
*/

// class SerialDisplay_c
// {
// private:
// 	// consts 1
// 	uint8_t setpointTimer = SETPOINT_TIMER_INTERVAL;
//
// 	uint16_t lastButtonRead = 0;
// 	uint32_t lastPressTime = 0;
//
// 	uint8_t currentPage = -1;				 // + 1 will make it start at page 0,
// 	int8_t lcdPageSwitchCounter = 0; // counts down
// 	bool holdSetPage = false;
//
// 	uint8_t toneTimerSpacing = 0;
// 	uint32_t toneDuration = 0;
// 	uint32_t toneStartingTime = 0;
//
// 	bool isValveClosed = false;
// 	uint16_t valveCloseTimer = 0;
//   // constantly counts up, so it's okay for it to cycle back to 0
// 	uint8_t alarmCounter = 0; 
// 	uint16_t alarmCountdown = 0;
//
//   // holds the lines to print
// 	String currentLine1 = "";
// 	String currentLine2 = "";
// 	String previousLine1 = "";
// 	String previousLine2 = "";
//
// public:
// 	SerialDisplay_c() { 
// 		pinMode(T_STAT_BUTTON_PIN, INPUT);
// 		pinMode(WATER_VALVE_PIN, OUTPUT);
// 	}
//
// 	void initialize()
// 	{
//     DEBUG_startup();
// 		//
// 		// lcd.createChar(1, customCharDegF); // char(233)
// 		// lcd.createChar(2, customCharInside);
// 		// lcd.createChar(3, customCharOutside);
// 		// lcd.createChar(4, customCharSmColon); // char(58)
// 		// lcd.createChar(5, customCharDroplet); // char(218)
// 		// lcd.createChar(6, customCharsetPoint); // char(92)
// 		// lcd.createChar(7, customCharSunAndHouse); // char(235)
// 		// lcd.createChar(8, customCharDotInsideHouse); // char(252)
//       // 0x7e => // char(126)
// 	}
//
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 	// Utility functions:
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 	void detectButtons()
// 	{
// 		uint16_t buttonRead = analogRead(T_STAT_BUTTON_PIN);
//
// 		bool isButtonPressDetected = buttonRead > 63;
//
// 		if (!isButtonPressDetected)
// 		{
// 			// save for next time
// 			lastButtonRead = buttonRead;
// 			return;
// 		}
//
// 		if (DEBUG)
// 			Serial.println(buttonRead);
//
// 		// initialize variable
// 		int8_t changeTstatByAmount = 0;
//
// 		// button press will turn off alarm! Yay!
// 		bool alarmIsOn = alarmCountdown > 0 || alarmCounter > 1;
// 		if (alarmIsOn)
// 		{
// 			alarmCountdown = 0;
// 			alarmCounter = 1; // set to odd number
// 			checkBeep(2);			// silences tone if odd number
// 		}
//
// 		bool isButtonHeldForSecondTick = lastButtonRead > 63;
// 		bool isTopButton = buttonRead > 850;
//
//     // delay adjustment by 1 tick
// 		if (isButtonHeldForSecondTick) { 
// 			if (isTopButton) {
// 				changeTstatByAmount = 1;
// 				// reset record lows
// 				for (uint8_t k = 0; k < 2; k++)
// 					air[k].lowest = air[k].tempF;
// 			} else {
//         // bottom button
// 				changeTstatByAmount = -1;
// 				// reset record highs
// 				for (uint8_t k = 0; k < 2; k++)
// 					air[k].highest = air[k].tempF;
// 			}
// 		}
//
// 		// apply the changes to setpoint
// 		setPoint += double(SETPOINT_ADJ_PER_TICK * changeTstatByAmount);
//
// 		// show (and hold onto) our set page
// 		drawSetPage();
// 		holdSetPage = true;
// 		// check if needs to run
// 		pump.checkAfter(); // default 3 seconds
//
// 		// save for next time
// 		lastButtonRead = buttonRead;
// 	}
//
// 	// render current page every call, and rollover currentPage when needed
// 	void printCurrentPage()
// 	{
// 		previousLine1 = currentLine1;
// 		previousLine2 = currentLine2;
//
// 		switch (currentPage)
// 		{
// 		case (0):
// 			drawTemperaturePage();
// 			break;
//
// 		case (1):
// 			drawPumpCycleInfoPage();
// 			break;
//
// 		case (2):
// 			drawTemperaturePage();
// 			break;
//
// 		case (3):
// 			drawAccumTimePage();
// 			break;
//
// 		case (4):
// 			drawWaterFillingPage();
// 			break;
//
// 		// currentPage > highest ? rollover to 0 and rerun
// 		default:
// 			currentPage = 0;
// 			printCurrentPage();
// 			break;
// 		}
// 	}
//
// 	void incrementSwitchPageCounter()
// 	{
// 		// if holding, and pump moves out of "delayed" phase, turn off holdSetPage
// 		if (holdSetPage && pump.state != 3)
// 			holdSetPage = false;
//
// 		// increment currentPage every 3.5 seconds
// 		lcdPageSwitchCounter--;
// 		bool readyToSwitchPage = lcdPageSwitchCounter <= 0;
// 		if (readyToSwitchPage)
// 		{
// 			currentPage++; // is rolled over in the printCurrentPage function
// 			lcdPageSwitchCounter = LCD_INTERVAL_QTR_SECS;
// 		}
//
// 		// render lcdPageSet if locked to that page; else refresh the page
// 		if (holdSetPage) {
// 			drawSetPage();
//
// 			// String str = "H" + String(pump.state) + symbols[3] + String(timeRemaining);
// 			String str = "H" + String(pump.state) + ":" + String(timeRemaining);
// 			drawLine(str, 11, 0);
// 		} else {
// 			// if going into a new page, clear it
// 			bool isNewPageNext = lcdPageSwitchCounter == LCD_INTERVAL_QTR_SECS;
// 			// if (isNewPageNext)
// 			// 	lcd.clear();
//
// 			// refresh the page, or print the new page if it switched
// 			printCurrentPage();
// 		}
// 	}
//
// 	void regulateValve() {
// 		// should be closed during our timer (when tank gets full)
// 		bool shouldValveBeClosed = valveCloseTimer > 0;
//
// 		if (shouldValveBeClosed)
// 			valveCloseTimer--;
// 		else
// 			// after timer, open the valve
// 			isValveClosed = false;
//
// 		digitalWrite(WATER_VALVE_PIN, (isValveClosed) ? HIGH : LOW);
// 	}
//
//
//
//
//
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 	// Update function:
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 	// runs every 0.25s
// 	void update()
// 	{
// 		// setpoint delay
// 		setpointTimer--;
// 		bool shouldDetectButtons = setpointTimer == 0;
// 		if (shouldDetectButtons)
// 		{
// 			detectButtons();
// 			setpointTimer = SETPOINT_TIMER_INTERVAL;
// 		}
//
// 		incrementSwitchPageCounter();
//
// 		// Basically,
// 		// 	What I want is: if (tank is filling and) tank is between 90 and 95%,
// 		//		The beeper should beep every ~4 seconds (until tank is no longer filling)
// 		//		(Increment a counter, and if counter % 16 == 0, do a beep, else, stop the beep)
// 		// And, if (tank is filling and) tank is above 95%,
// 		//		The beeper should beep every ~1 seconds (until tank is no longer filling)
// 		//		(Increment a counter, and if counter % 4 == 0, do a beep, else, stop the beep)
// 		// And, if the tank is newly full,
// 		// 		The beeper should beep every ~0.5 seconds for a duration of 180 seconds.
// 		//		(Increment a counter, and if counter % 2 == 0, do a beep, else, stop the beep)
//
// 		if (waterTank.filling)
// 		{
// 			drawWaterLevel(); // draw waterLevel
//
// 			// do alarm stuff
// 			bool isWaterOver95 = waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_FAST;
// 			bool isWaterOver90 = waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_SLOW;
// 			bool isWaterOver85 = waterTank.displayPercent >= 85;
//
// 			if (isWaterOver85)
// 			{
// 				alarmCounter++; // count while above 85% (and filling)
//
// 				// beep when it should
// 				if (isWaterOver95)
// 					checkBeep(4); // fast beeping
// 				else if (isWaterOver90)
// 					checkBeep(8); // medium beeping
// 				else
// 					checkBeep(16); // slow beeping
// 			}
// 		}
// 		else if (waterTank.newlyFull)
// 		{
// 			// moment it hits 100%
// 			waterTank.newlyFull = false;
//
// 			alarmCountdown = 180 * 4; // seconds * 4
//
// 			isValveClosed = true;
// 			valveCloseTimer = VALVE_CLOSE_DURATION;
// 		}
//
// 		regulateValve();
//
// 		//
// 		if (alarmCountdown > 0)
// 			checkBeep(2, alarmCountdown);
// 	} // end update (every 0.25 seconds)
//
// 	void beepOnTime(uint8_t counter, uint16_t spacing)
// 	{
// 		if (counter % spacing == 0)
// 			tone(TONE_PIN, NOTE_C5, 250); // play our tone on pin for 250ms
// 		else
// 			noTone(TONE_PIN);
// 	}
//
// 	// takes spacing (n * 0.25s) and optional countdown timer for when hitting maximum
// 	void checkBeep(uint8_t spacing, uint16_t countdown = 0)
// 	{
// 		// if countdown, we count down the timer and beep when needed
// 		uint16_t counter = alarmCounter;
// 		if (countdown > 0)
// 		{
// 			countdown--;
// 			counter = countdown;
// 		}
//
// 		beepOnTime(counter, spacing);
// 	}
//
//
//
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 	// Pages:
// 	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 	void drawLines(String line1, String line2)
// 	{
// 		drawLine(line1 );
// 		drawLine(line2 );
// 	}
//
// 	void drawLine(String string, bool newLine = true)
// 	{
//     if (newLine) {
//       Serial.println(string);
//     } else {
//       Serial.print(string);
//     }
// 	}
//
// 	void drawLinesIfDifferent(String l1, String l2)
// 	{
// 		if (previousLine1 != l1 || previousLine2 != l2) // for after all pages are switched over...
// 			drawLines(currentLine1, currentLine2);
// 	}
//
// 	void drawWaterLevel(uint8_t lineNum = 1)
// 	{
// 		String extra = char(218) + waterTank.getDisplayString();
// 		drawLine(extra, false);
// 	}
//
// 	void drawTemperaturePage()
// 	{
// 		currentLine1 = char(252) + limitDecimals(weightedAirTemp, 1) 
//       + " " + limitDecimals(air[0].getTempEma(), 1) + "/" 
//       + limitDecimals(air[1].getTempEma(), 1) + char(233);
// 		currentLine2 = char(92) + limitDecimals(setPoint, 1) + char(233) + " " 
//         + limitDecimals(difference, 1) + "   ";
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 		drawWaterLevel(); // prints on bottom right
// 	}
//
// 	// display cycle info, time/cycleDuration
// 	void drawPumpCycleInfoPage()
// 	{
// 		currentLine1 = pump.getStatusString() + " " 
//         + formatTimeToString(pump.cycleDuration);
// 		currentLine2 = "RunTtl" + char(58) 
//         + formatHoursWithTenths(getTotalSeconds(currentTime));
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 	}
//
// 	void drawAccumTimePage()
// 	{ // Total time and time spent with pump on/off
// 		const uint32_t totalSeconds = getTotalSeconds(currentTime);
// 		const uint32_t accumBelow = (totalSeconds - pump.accumAbove);
// 		const int32_t netAccumAboveTarget = pump.accumAbove - accumBelow; // positive or negative
// 		const String hours = formatHoursWithTenths(netAccumAboveTarget);
//
// 		const uint32_t accumOff = (totalSeconds - pump.accumOn);
// 		const int32_t netAccumOn = pump.accumOn - accumOff;
//
// 		currentLine1 = char(92) + " " + char(58) + String((netAccumAboveTarget < 0) 
//       ? hours 
//       : "+" + hours
//     );
// 		currentLine2 = "State" + char(58) + String(pump.state) + 
//       (netAccumOn < 0 
//         ? " Off" 
//         : "  On"
//       ) + char(58) + formatHoursWithTenths(
//         (netAccumOn < 0) 
//         ? netAccumOn * -1 
//         : netAccumOn
//       );
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 	}
//
// 	void drawSetPage()
// 	{ // Set Temperature page
// 		currentLine1 = "Temp" + char(58) + limitDecimals(weightedAirTemp, 1) 
//       + char(233) + "      ";
// 		currentLine2 = "   " + char(92) + limitDecimals(setPoint, 1) + "        ";
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 	}
//
// 	// Show greenhouse and exterior sensor readings
// 	void drawGreenhouse()
// 	{
// 		currentLine1 = "GH" + char(58) + limitDecimals(air[3].currentEMA[0], 1) 
//       + char(233) + " " + limitDecimals(air[3].humid, 1) + "%  ";
// 		currentLine2 = " " + char(235) + " " + limitDecimals(air[2].currentEMA[0], 1) 
//       + char(233) + " " + limitDecimals(air[2].humid, 1) + "%  ";
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 	}
//
// 	void drawWaterFillingPage()
// 	{ // Water Filling Temp Page
// 		currentLine1 = char(218) + " " + String(waterTank.ema) + ":" 
//       + String(waterTank.slowEma) + " " + char(126) 
//       + (waterTank.diffEma < 10 ? "  " : " ") + String(waterTank.diffEma);
// 		currentLine2 = "Flr"+ char(58) + String(int(floorEmaAvg)) + ":" 
//       + String(int(floorEmaAvgSlow)) + "     ";
//
// 		drawLinesIfDifferent(currentLine1, currentLine2);
// 	}
//
// } SerialDisplay = Serial_c();
