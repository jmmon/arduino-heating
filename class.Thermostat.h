const byte customCharInside[8] = { // "inside"
		B00000,
		B00000,
		B00000,
		B10100,
		B01111,
		B11100,
		B01000,
		B01000};

const byte customCharOutside[8] = { // "outside"
		B01010,
		B00110,
		B01110,
		B00000,
		B00000,
		B01111,
		B01000,
		B01000};

const byte customCharDegF[8] = { // "degrees F"
		B01000,
		B10100,
		B01000,
		B00111,
		B00100,
		B00110,
		B00100,
		B00100};

const byte customCharSmColon[8] = { // small colon + space
		B00000,
		B00000,
		B01000,
		B00000,
		B00000,
		B01000,
		B00000,
		B00000};

const byte customCharDroplet[8] = { // water droplet
		B00100,
		B01110,
		B01010,
		B10111,
		B10111,
		B10111,
		B11111,
		B01110};

const byte customCharsetPoint[8] = { // thermometer with indicator
		B00000,
		B01000,
		B11001,
		B01011,
		B11001,
		B01000,
		B10100,
		B01000};

const byte customCharSunAndHouse[8] = {
		B11000,
		B11000,
		B00010,
		B00111,
		B01111,
		B00101,
		B00111,
		B00111};

const byte customCharDotInsideHouse[8] = {
		B00100,
		B01010,
		B11111,
		B10001,
		B10001,
		B10101,
		B10001,
		B10001};

/* struct consts:
Sketch uses 26416 bytes (85%) of program storage space. Maximum is 30720 bytes.
Global variables use 1389 bytes (67%) of dynamic memory, leaving 659 bytes for local variables. Maximum is 2048 bytes.
*/

/* global consts:
Sketch uses 26176 bytes (85%) of program storage space. Maximum is 30720 bytes.
Global variables use 1370 bytes (66%) of dynamic memory, leaving 678 bytes for local variables. Maximum is 2048 bytes.
*/

/* diff: bytes saved with globals
26416 - 26176 = 24
1389 - 1370 = 19
*/

/*
	9x uint8_t
	1x float
	3x uint16_t
*/

/* small optimizations: reducing temporary variables:
Sketch uses 26154 bytes (85%) of program storage space. Maximum is 30720 bytes.		(another 22 bytes!)
Global variables use 1370 bytes (66%) of dynamic memory, leaving 678 bytes for local variables. Maximum is 2048 bytes. ( same)

reducing extra (copy) memoryvariables:
Sketch uses 24636 bytes (80%) of program storage space. Maximum is 30720 bytes.  (BOOM! ANOTHER 1518 bytes saved!)
Global variables use 1370 bytes (66%) of dynamic memory, leaving 678 bytes for local variables. Maximum is 2048 bytes.

*/

const float SETPOINT_ADJ_PER_TICK = 1;		 // degrees change
const uint8_t SETPOINT_TIMER_INTERVAL = 2; // quarter seconds

const uint8_t LCD_INTERVAL_QTR_SECS = 14; // 3.5s

const uint16_t TONE_DURATION = 180;			// seconds
const uint16_t TONE_DURATION_FULL = 60; // when hits 100%, beep for this long
const uint8_t TONE_TIMER_SPACING_SLOW = 16;
const uint8_t TONE_TIMER_SPACING_FAST = 4;
const uint8_t TONE_TIMER_SPACING_SOLID = 1;

const uint8_t TONE_TIMER_TRIGGER_PERCENT_SLOW = 90;
const uint8_t TONE_TIMER_TRIGGER_PERCENT_FAST = 95;

const uint16_t VALVE_CLOSE_DURATION = 21600; // seconds (6 hours)

/* ==========================================================================
 * Display class
 * ========================================================================== */
class Display_c
{
private:
	// consts 1
	uint8_t setpointTimer = SETPOINT_TIMER_INTERVAL;

	uint16_t lastButtonRead = 0;
	uint32_t lastPressTime = 0;

	uint8_t currentPage = -1;				 // + 1 will make it start at page 0,
	int8_t lcdPageSwitchCounter = 0; // counts down
	bool holdSetPage = false;

	uint8_t toneTimerSpacing = 0;
	uint32_t toneDuration = 0;
	uint32_t toneStartingTime = 0;

	bool isValveClosed = false;
	uint16_t valveCloseTimer = 0;
	uint8_t alarmCounter = 0; // constantly counts up, so it's okay for it to cycle back to 0
	uint16_t alarmCountdown = 0;

	String currentLine1 = "";
	String currentLine2 = "";
	String previousLine1 = "";
	String previousLine2 = "";

public:
	Display_c()
	{ // constructor
		pinMode(T_STAT_BUTTON_PIN, INPUT);
	}

	void initialize()
	{
		lcd.init(); // initialize the lcd
		lcd.clear();
		lcd.backlight(); // open the backlight

		lcd.createChar(1, customCharDegF);
		lcd.createChar(2, customCharInside);
		lcd.createChar(3, customCharOutside);
		lcd.createChar(4, customCharSmColon);
		lcd.createChar(5, customCharDroplet);
		lcd.createChar(6, customCharsetPoint);
		lcd.createChar(7, customCharSunAndHouse);
		lcd.createChar(8, customCharDotInsideHouse);
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Utility functions:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	void detectButtons()
	{
		uint16_t buttonRead = analogRead(T_STAT_BUTTON_PIN);

		bool isButtonPressDetected = buttonRead > 63;

		if (!isButtonPressDetected)
		{
			// save for next time
			lastButtonRead = buttonRead;
			return;
		}

		if (DEBUG)
			Serial.println(buttonRead);

		// initialize variable
		int8_t changeTstatByAmount = 0;

		// button press will turn off alarm! Yay!
		bool alarmIsOn = alarmCountdown > 0 || alarmCounter > 1;
		if (alarmIsOn)
		{
			alarmCountdown = 0;
			alarmCounter = 1; // set to odd number
			checkBeep(2);			// silences tone if odd number
		}

		bool isButtonHeldForSecondTick = lastButtonRead > 63;
		bool isTopButton = buttonRead > 850;
		if (isButtonHeldForSecondTick)
		{ // delays adjustment by 1 tick
			if (isTopButton)
			{
				changeTstatByAmount = 1;
				// reset record lows
				for (uint8_t k = 0; k < 2; k++)
					air[k].lowest = air[k].tempF;
			}
			else // bottom button
			{
				changeTstatByAmount = -1;
				// reset record highs
				for (uint8_t k = 0; k < 2; k++)
					air[k].highest = air[k].tempF;
			}
		}

		// apply the changes to setpoint
		setPoint += double(SETPOINT_ADJ_PER_TICK * changeTstatByAmount);

		// show (and hold onto) our set page
		lcd.clear();
		drawSetPage();
		holdSetPage = true;
		// check if needs to run
		pump.checkAfter(); // default 3 seconds

		// save for next time
		lastButtonRead = buttonRead;
	}

	// render current page every call, and rollover currentPage when needed
	void printCurrentPage()
	{
		previousLine1 = currentLine1;
		previousLine2 = currentLine2;

		switch (currentPage)
		{
		case (0):
			drawTemperaturePage();
			break;

		case (1):
			drawPumpCycleInfoPage();
			break;

		case (2):
			drawTemperaturePage();
			break;

		case (3):
			drawAccumTimePage();
			break;

		case (4):
			drawWaterFillingPage();
			break;

		// currentPage > highest ? rollover to 0 and rerun
		default:
			currentPage = 0;
			printCurrentPage();
			break;
		}
	}

	void incrementSwitchPageCounter()
	{
		// if holding, and pump moves out of "delayed" phase, turn off holdSetPage
		if (holdSetPage && pump.state != 3)
			holdSetPage = false;

		// increment currentPage every 3.5 seconds
		lcdPageSwitchCounter--;
		bool readyToSwitchPage = lcdPageSwitchCounter <= 0;
		if (readyToSwitchPage)
		{
			currentPage++; // is rolled over in the printCurrentPage function
			lcdPageSwitchCounter = LCD_INTERVAL_QTR_SECS;
		}

		// render lcdPageSet if locked to that page; else refresh the page
		if (holdSetPage)
		{
			drawSetPage();

			// String str = "H" + String(pump.state) + symbols[3] + String(timeRemaining);
			String str = "H" + String(pump.state) + ":" + String(timeRemaining);
			drawLine(str, 11, 0);
		}
		else
		{
			// if going into a new page, clear it
			bool isNewPageNext = lcdPageSwitchCounter == LCD_INTERVAL_QTR_SECS;
			if (isNewPageNext)
				lcd.clear();
			// refresh the page, or print the new page if it switched
			printCurrentPage();
		}
		// printCurrentPage();
	}

	void regulateValve()
	{
		// should be closed during our timer (when tank gets full)
		bool shouldValveBeClosed = valveCloseTimer > 0;

		if (shouldValveBeClosed)
			valveCloseTimer--;
		else
			// after timer, open the valve
			isValveClosed = false;

		digitalWrite(WATER_VALVE_PIN, (isValveClosed) ? HIGH : LOW);
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Update function:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// runs every 0.25s
	void update()
	{
		// setpoint delay
		setpointTimer--;
		bool shouldDetectButtons = setpointTimer == 0;
		if (shouldDetectButtons)
		{
			detectButtons();
			setpointTimer = SETPOINT_TIMER_INTERVAL;
		}

		incrementSwitchPageCounter();

		// Basically,
		// 	What I want is: if (tank is filling and) tank is between 90 and 95%,
		//		The beeper should beep every ~4 seconds (until tank is no longer filling)
		//		(Increment a counter, and if counter % 16 == 0, do a beep, else, stop the beep)
		// And, if (tank is filling and) tank is above 95%,
		//		The beeper should beep every ~1 seconds (until tank is no longer filling)
		//		(Increment a counter, and if counter % 4 == 0, do a beep, else, stop the beep)
		// And, if the tank is newly full,
		// 		The beeper should beep every ~0.5 seconds for a duration of 180 seconds.
		//		(Increment a counter, and if counter % 2 == 0, do a beep, else, stop the beep)

		if (waterTank.filling)
		{
			drawWaterLevel(); // draw waterLevel

			// do alarm stuff
			bool isWaterOver95 = waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_FAST;
			bool isWaterOver90 = waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_SLOW;
			bool isWaterOver85 = waterTank.displayPercent >= 85;

			if (isWaterOver85)
			{
				alarmCounter++; // count while above 85% (and filling)

				// beep when it should
				if (isWaterOver95)
					checkBeep(4); // fast beeping
				else if (isWaterOver90)
					checkBeep(8); // medium beeping
				else
					checkBeep(16); // slow beeping
			}
		}
		else if (waterTank.newlyFull)
		{
			// moment it hits 100%
			waterTank.newlyFull = false;

			alarmCountdown = 180 * 4; // seconds * 4

			isValveClosed = true;
			valveCloseTimer = VALVE_CLOSE_DURATION;
		}

		regulateValve();

		//
		if (alarmCountdown > 0)
			checkBeep(2, alarmCountdown);
	} // end update (every 0.25 seconds)

	void beepOnTime(uint8_t counter, uint16_t spacing)
	{
		if (counter % spacing == 0)
			tone(TONE_PIN, NOTE_C5, 250); // play our tone on pin for 250ms
		else
			noTone(TONE_PIN);
	}

	// takes spacing (n * 0.25s) and optional countdown timer for when hitting maximum
	void checkBeep(uint8_t spacing, uint16_t countdown = 0)
	{
		// if countdown, we count down the timer and beep when needed
		uint16_t counter = alarmCounter;
		if (countdown > 0)
		{
			countdown--;
			counter = countdown;
		}

		beepOnTime(counter, spacing);
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Pages:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	void drawLines(String line1, String line2)
	{
		drawLine(line1, 0, 0);
		drawLine(line2, 0, 1);
	}

	void drawLine(String string, uint8_t slot, uint8_t lineNum)
	{
		lcd.setCursor(slot, lineNum);
		lcd.print(string);
	}

	void drawLinesIfDifferent(String l1, String l2)
	{
		if (previousLine1 != l1 || previousLine2 != l2) // for after all pages are switched over...
			drawLines(currentLine1, currentLine2);
	}

	void drawWaterLevel(uint8_t lineNum = 1)
	{
		String extra = "\005" + waterTank.getDisplayString();
		drawLine(extra, 13, lineNum);
	}

	void drawTemperaturePage()
	{
		currentLine1 = "\008" + limitDecimals(weightedAirTemp, 1) + " " + limitDecimals(air[0].getTempEma(), 1) + "/" + limitDecimals(air[1].getTempEma(), 1) + "\001";
		currentLine2 = "\006" + limitDecimals(setPoint, 1) + "\001 " + limitDecimals(difference, 1) + "   ";

		drawLinesIfDifferent(currentLine1, currentLine2);
		drawWaterLevel(); // prints on bottom right
	}

	// display cycle info, time/cycleDuration
	void drawPumpCycleInfoPage()
	{
		currentLine1 = pump.getStatusString() + " " + formatTimeToString(pump.cycleDuration);
		currentLine2 = "RunTtl\004" + formatHoursWithTenths(getTotalSeconds(currentTime));

		drawLinesIfDifferent(currentLine1, currentLine2);
	}

	void drawAccumTimePage()
	{ // Total time and time spent with pump on/off
		const uint32_t totalSeconds = getTotalSeconds(currentTime);
		const uint32_t accumBelow = (totalSeconds - pump.accumAbove);
		const int32_t netAccumAboveTarget = pump.accumAbove - accumBelow; // positive or negative
		const String hours = formatHoursWithTenths(netAccumAboveTarget);

		const uint32_t accumOff = (totalSeconds - pump.accumOn);
		const int32_t netAccumOn = pump.accumOn - accumOff;

		currentLine1 = "\006\004 " + String((netAccumAboveTarget < 0) ? hours : "+" + hours);
		currentLine2 = "State\004" + String(pump.state) + (netAccumOn < 0 ? " Off" : "  On") + "\004" + formatHoursWithTenths((netAccumOn < 0) ? netAccumOn * -1 : netAccumOn);

		drawLinesIfDifferent(currentLine1, currentLine2);
	}

	void drawSetPage()
	{ // Set Temperature page
		currentLine1 = "Temp\004" + limitDecimals(weightedAirTemp, 1) + "\001      ";
		currentLine2 = "   \006" + limitDecimals(setPoint, 1) + "        ";

		drawLinesIfDifferent(currentLine1, currentLine2);
	}

	// Show greenhouse and exterior sensor readings
	void drawGreenhouse()
	{
		currentLine1 = "GH\004" + limitDecimals(air[3].currentEMA[0], 1) + "\001 " + limitDecimals(air[3].humid, 1) + "%  ";
		currentLine2 = " \007 " + limitDecimals(air[2].currentEMA[0], 1) + "\001 " + limitDecimals(air[2].humid, 1) + "%  ";

		drawLinesIfDifferent(currentLine1, currentLine2);
	}

	void drawWaterFillingPage()
	{ // Water Filling Temp Page
		currentLine1 = "\005 " + String(waterTank.ema) + ":" + String(waterTank.slowEma) + " \x7e" + (waterTank.diffEma < 10 ? "  " : " ") + String(waterTank.diffEma);
		currentLine2 = "Flr\004" + String(int(floorEmaAvg)) + ":" + String(int(floorEmaAvgSlow)) + "     ";

		drawLinesIfDifferent(currentLine1, currentLine2);
	}

	// void show_WaterFlowCounter()
	// {
	// 	// flow rate
	// currentLine1 = "WtrRate\004" + String(flowRate) + " l/m";
	// currentLine2 = "WtrTtl\004" + String(totalMillilitres / 1000.) + " L";

	// if (previousLine1 != currentLine1 || previousLine2 != currentLine2) // for after all pages are switched over...
	// 	drawLines(currentLine1, currentLine2);

	// // drawWaterLevel(); // prints on bottom right
	// }

	// void show_Time()
	// {
	// t = now(); // update time when this is called!!
	// const uint8_t dayOfWeek = weekday(t); // number!

	// String theDay = String((dayOfWeek == 1) ? ("Sun") : (dayOfWeek == 2) ? ("Mon")
	// 															: (dayOfWeek == 3)	 ? ("Tue")
	// 															: (dayOfWeek == 4)	 ? ("Wed")
	// 															: (dayOfWeek == 5)	 ? ("Thu")
	// 															: (dayOfWeek == 6)	 ? ("Fri")
	// 																									 : ("Sat"));

	// currentLine1 = String(int(hour(t)) < 10 ? F("0") : F("")) + String(hour(t)) + String(int(minute(t)) < 10 ? F("0") : F("")) + String(minute(t)) + String(int(second(t)) < 10 ? F("0") : F("")) + String(second(t)) + " " + theDay;
	// currentLine2 = String(day(t)) + " " + String(month(t)) + " " + String(year(t));

	// if (previousLine1 != currentLine1 || previousLine2 != currentLine2) // for after all pages are switched over...
	// 	drawLines(currentLine1, currentLine2);

	// }

	// void pagePidInfo()
	// {
	// 	currentLine1 = "Kp\004" + int(Kp) + " Ki\004" + Ki
	// 	currentLine2 = "Kd\004" + int(Kd) + " Outpt\004" + Output;

	// 	if (previousLine1 != currentLine1 || previousLine2 != currentLine2) // for after all pages are switched over...
	// 		drawLines(currentLine1, currentLine2);

	// }

} Tstat = Display_c();

// // note: lcd.write() {0~64 == ??}, {65~90 == A~Z}, {91~96 == ??}, {97~122 == a~z}, {123-127 == ??}
