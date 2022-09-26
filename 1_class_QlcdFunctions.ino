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

const byte customCharSetpoint[8] = { // thermometer with indicator
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

class Display_c
{
private:
	// consts 1
	uint8_t setpointTimer = SETPOINT_TIMER_INTERVAL;

	uint16_t lastButtonRead = 0;
	uint32_t lastPressTime = 0;

	uint8_t currentLcdPage = -1;		 // + 1 will make it start at page 0,
	int8_t lcdPageSwitchCounter = 0; // counts down
	bool holdSetPage = false;

	// uint16_t toneTimer = 0;
	uint8_t toneTimerSpacing = 0;

	// uint32_t toneEndingTime = 0;

	uint32_t toneDuration = 0;
	uint32_t toneStartingTime = 0;

	bool isValveClosed = false;
	uint16_t valveCloseTimer = 0;

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
		lcd.createChar(6, customCharSetpoint);
		lcd.createChar(7, customCharSunAndHouse);
		lcd.createChar(8, customCharDotInsideHouse);
	}

	void startAlarmTimer(uint32_t _duration, uint16_t _spacing)
	{
		toneStartingTime = currentTime;
		toneTimerSpacing = _spacing;
		toneDuration = _duration * 1000; // in ms
	}

	void stopAlarmTimer()
	{
		toneStartingTime = 0;
		toneDuration = 0;
	}

	void soundTheAlarm()
	{
		if (currentTime - toneStartingTime % (toneTimerSpacing * 25) == 0)
		//based off currentTime, this will actually trigger based off ms % timerSpacing, which is not what is wanted.
		// I want it to trigger based on a starting time counter which will count up ....
		// every 0.25s, if (ms time passed since start) % (toneSpacingQtrSeconds * 25ms) == 0, should do what I want?
		{
			tone(TONE_PIN, NOTE_C5, 250); // play our tone on pin for 250ms
		}
		else
		{
			noTone(TONE_PIN);
		}
	}

	// void beepOnIntervalByType(uint8_t type) {
	// 	uint16_t DURATION = 500;
	// 	uint16_t interval = type * 1000 + 1000; // some formula for the beeping intervals

	// 	//if firing at the correct timer spacing, start the tone
	// 	// else no tone
	// 	if (currentTime - toneStartingTime % tomeTimer)
	// }

	void detectButtons()
	{
		uint16_t buttonRead = analogRead(T_STAT_BUTTON_PIN);

		if (buttonRead > 63)
		{
			int8_t thermostatChangeDirection = 0;

			if (DEBUG)
			{
				Serial.println(buttonRead);
			}

			// buttons also turn off water filling alarm early! yay!
			if (toneStartingTime > 0)
			{
				stopAlarmTimer();
			}

			if (lastButtonRead > 63)
			{ // delays adjustment by 1 tick
				if (buttonRead > 850)
				{
					thermostatChangeDirection = 1;
					for (uint8_t k = 0; k < 2; k++)
					{ // reset record temps
						air[k].lowest = air[k].tempF;
					}
				}
				else
				{
					thermostatChangeDirection = -1;
					for (uint8_t k = 0; k < 2; k++)
					{
						air[k].highest = air[k].tempF;
					}
				}
			}

			Setpoint += double(SETPOINT_ADJ_PER_TICK * thermostatChangeDirection);

			lcd.clear();
			showPage_Set();
			holdSetPage = true;
			pump.checkAfter(); // default 3 seconds
		}
		lastButtonRead = buttonRead;
	}

	String formatTimeToString(uint32_t _t)
	{ // takes seconds
		// get base time from seconds count
		uint16_t hours = _t / 3600;
		_t -= (hours * 3600);
		uint16_t minutes = _t / 60;
		_t -= (minutes * 60);
		uint16_t seconds = _t;

		// rollover if necessary
		while (seconds >= 60)
		{
			minutes += 1;
			seconds -= 60;
		}
		while (minutes >= 60)
		{
			hours += 1;
			minutes -= 60;
		}

		return (((hours < 10) ? "0" : "") + String(hours) +
						((minutes < 10) ? ":0" : ":") + (String(minutes)) +
						((seconds < 10) ? ":0" : ":") + String(seconds));
	}

	String formatHoursWithTenths(int32_t _t)
  {
		return String((_t / 3600), 1);
	}

	void printCurrentLcdPage()
	{
		switch (currentLcdPage)
		{ // display proper page, with auto rollover/recall
		case (0):
			showPage_Time(); // new!
			break;
		case (1):
			showPage_Temperature();
			break;
		case (2):
			showPage_Greenhouse();
			break;
		case (3):
			showPage_PumpCycleInfo();
			break;

		case (4):
			showPage_Time(); // new!
			break;
		case (5):
			showPage_Temperature();
			break;
		case (6):
			showPage_AccumTime();
			break;
		case (7):
			showPage_WaterFilling();
			break;
		case (8):
			showPage_WaterFlowCounter();
			break;
		default: // currentLcdPage > highest ? rollover to 0 and reprint immediately
			currentLcdPage = 0;
			printCurrentLcdPage();
		}
	}

	void updatePage()
	{
		// reset holdSetPage when ready
		if (
				pump.state != 3 &&
				holdSetPage)
		{
			holdSetPage = false;
		}

		// switch page every 3.5 seconds
		lcdPageSwitchCounter--;
		if (lcdPageSwitchCounter <= 0)
		{
			lcdPageSwitchCounter = LCD_INTERVAL_QTR_SECS;
			currentLcdPage++; // is rolled over in the printCurrentLcdPage function
		}

		// display lcdPageSet if locked to that page; else refresh the page
		if (holdSetPage)
		{
			showPage_Set();
		}
		else
		{
			if (lcdPageSwitchCounter == LCD_INTERVAL_QTR_SECS)
			{ // every 3.5 sec
				lcd.clear();
			}
			printCurrentLcdPage(); // every sec
		}
	}

	void regulateValve()
	{
		if (valveCloseTimer > 0)
		{
			valveCloseTimer--;
		}
		else
		{
			isValveClosed = false;
		}

		digitalWrite(WATER_VALVE_PIN, (isValveClosed) ? HIGH : LOW);
	}

	void update()
	{ // every 0.25s
		// if update every occurrence, the timer could happen here, or in loop:

		// setpoint delay
		setpointTimer--;
		if (setpointTimer <= 0)
		{
			detectButtons();
			setpointTimer = SETPOINT_TIMER_INTERVAL;
		}

		updatePage();

		if (waterTank.filling)
		{												// WIP: Tank Is Being Filled indicator shows on all pages
			lcd.setCursor(15, 0); // top right
			lcd.write(5);
		}

		if (waterTank.filling)
		{
			if (waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_FAST)
			{
				startAlarmTimer(TONE_DURATION, TONE_TIMER_SPACING_FAST);
			}
			else if (waterTank.displayPercent >= TONE_TIMER_TRIGGER_PERCENT_SLOW)
			{
				startAlarmTimer(TONE_DURATION, TONE_TIMER_SPACING_SLOW);
			}
		}
		else if (waterTank.newlyFull)
		{
			// moment it hits 100%
			waterTank.newlyFull = false;

			startAlarmTimer(TONE_DURATION_FULL, TONE_TIMER_SPACING_SOLID);

			isValveClosed = true;
			valveCloseTimer = VALVE_CLOSE_DURATION;
		}

		if (toneStartingTime > 0 &&
				currentTime - toneDuration < toneStartingTime)
		{
			soundTheAlarm();
		}
		else
		{
			stopAlarmTimer();
		}

		regulateValve();

	} // end update (every 0.25 seconds)

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Pages:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	void showPage_Temperature()
	{
		// first line
		lcd.setCursor(0, 0);
		lcd.write(8); // inside icon //1
		lcd.print(Input, 1);
		lcd.print(F(" "));									// 6
		lcd.print(air[0].currentEMA[0], 1); // 5
		lcd.print(F("/"));									// 6
		lcd.print(air[1].currentEMA[0], 1); // 10
		lcd.write(1);												// degrees F // 11

		// second line
		lcd.setCursor(0, 1);
		lcd.write(6);						// thermometer
		lcd.print(Setpoint, 1); // 6
		lcd.write(1);						// degrees F // 7
		lcd.print(" ");
		lcd.write(126); // pointing right
		lcd.print(Output, 2);
		lcd.setCursor(13, 1);
		lcd.write(5); // water droplet // 14
									// new
		// handle conversion / formatting:
		switch (waterTank.displayPercent)
		{
		case (100):
			lcd.print("FF");
			break;
		case (0):
			lcd.print("EE");
			break;
		case (101):
			lcd.print("ER");
			break;
		case (255):
			lcd.print("--");
			break;
		default:
			lcd.print((waterTank.displayPercent < 10) ? " " + String(waterTank.displayPercent) : String(waterTank.displayPercent));
		}
	}

	void showPage_PumpCycleInfo()
	{ // heating cycle page
		// display cycle info, time/cycleDuration
		lcd.setCursor(0, 0);											 // line 1
		lcd.print(pump.getStatus());							 // 3 chars
		lcd.print(F(" "));												 // 4
		lcd.print(formatTimeToString(pump.cycleDuration)); // 8 chars (12)

		lcd.setCursor(0, 1);															// bot
		lcd.print(F("RunTtl"));														// 6
		lcd.write(4);																			// small colon // 7
		lcd.print(formatHoursWithTenths(currentTime / 1000)); // 15
	}

	void pagePidInfo()
	{
		lcd.setCursor(0, 0);
		lcd.print(F("Kp"));
		lcd.write(4);
		lcd.print(Kp, 0);
		lcd.print(F(" Ki"));
		lcd.write(4);
		lcd.print(Ki, 4); // space for 8

		lcd.setCursor(0, 1);
		lcd.print(F("Kd"));
		lcd.write(4);
		lcd.print(Kd, 0);
		lcd.print(F(" Outpt"));
		lcd.write(4);
		lcd.print(Output);
	}

	void showPage_AccumTime()
	{											 // Total time and time spent with pump on/off
		lcd.setCursor(0, 0); // top
		lcd.write(6);				 // setpoint (thermometer)
		lcd.write(4);				 // 8
		const uint32_t totalSeconds = currentTime / 1000;
		int32_t difference = pump.accumAbove - (totalSeconds - pump.accumAbove); // positive or negative
		String hours = formatHoursWithTenths(difference);
		lcd.print((" ") + ((difference < 0) ? hours : ("+") + hours));

		lcd.setCursor(0, 1);	 // bot
		lcd.print(F("State")); // 7
		lcd.write(4);					 // 8
		lcd.print(pump.state); // 1
		difference = pump.accumOn - (totalSeconds - pump.accumOn);
		lcd.print((difference < 0) ? " Off" : "  On");
		lcd.write(4);																																	 // 8
		lcd.print(formatHoursWithTenths((difference < 0) ? difference * -1 : difference)); // 16
	}

	void showPage_Set()
	{													// Set Temperature page
		lcd.setCursor(0, 0);		// top
		lcd.print(F("Temp"));		// 6
		lcd.write(4);						// sm colon  // 7
		lcd.print(Input, 1);		// 11
		lcd.write(1);						// Replaces "*F" // 12
		lcd.print(F("      ")); // 16

		lcd.setCursor(0, 1); // bot
		lcd.print(F("   ")); // 3
		lcd.write(6);
		lcd.print(Setpoint, 1); // 6
		lcd.write(1);						// degrees F // 7
		lcd.print(" ");
		// lcd.write(127); //pointing left // 9 // 7
		lcd.write(126); // pointing right
		lcd.print(Output, 2);
	}

	void showPage_Greenhouse()
	{ // Show greenhouse and exterior sensor readings
		lcd.setCursor(0, 0);
		lcd.print(F("GH"));
		lcd.write(4);												// sm colon
		lcd.print(air[3].currentEMA[0], 1); // 7
		lcd.write(1);
		lcd.print(F(" "));					// 9
		lcd.print(air[3].humid, 1); // 13
		lcd.print(F("%  "));				// 16

		lcd.setCursor(0, 1);
		lcd.print(F(" "));
		lcd.write(7); // outside
		lcd.print(F(" "));
		lcd.print(air[2].currentEMA[0], 1); // 7
		lcd.write(1);
		lcd.print(F(" "));					// 9
		lcd.print(air[2].humid, 1); // 13
		lcd.print(F("%  "));				// 16
	}

	void showPage_WaterFilling()
	{											 // Water Filling Temp Page
		lcd.setCursor(0, 0); // top
		lcd.write(5);
		lcd.print(waterTank.ema); // 4
		lcd.print(F(":"));
		lcd.print(waterTank.slowEma); // 8
		lcd.print(F(" "));
		lcd.write(126); // pointing right
		lcd.print((waterTank.diffEma < 10) ? F("  ") : F(" "));
		// if (waterTank.diffEma < 10) {
		//     lcd.print(F("  "));
		// } else {
		//     lcd.print(F(" "));
		// }
		lcd.print(waterTank.diffEma); // 13

		lcd.setCursor(0, 1); // top
		lcd.print(F("Flr"));
		lcd.write(4);											// sm colon // 4
		lcd.print(floorSensor[0].ema, 0); // +3
		lcd.print(F(":"));
		lcd.print(floorSensor[1].ema, 0);
	}

	void showPage_WaterFlowCounter()
	{
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

	void showPage_Time()
	{
		t = now(); // update time when this is called!!
		lcd.setCursor(0, 0);
		lcd.print((int(hour(t)) < 10) ? F("0") : F(""));
		lcd.print(hour(t));
		lcd.print((int(minute(t)) < 10) ? F(":0") : F(":"));
		lcd.print(minute(t));
		lcd.print((int(second(t)) < 10) ? F(":0") : F(":"));
		lcd.print(second(t)); // takes 8 chars
		lcd.print(F(" "));
		const uint8_t dayOfWeek = weekday(t); // number!
		lcd.print(
				(dayOfWeek == 1) ? "Sun" : (dayOfWeek == 2) ? "Mon"
															 : (dayOfWeek == 3)		? "Tue"
															 : (dayOfWeek == 4)		? "Wed"
															 : (dayOfWeek == 5)		? "Thu"
															 : (dayOfWeek == 6)		? "Fri"
																										: "Sat");

		lcd.setCursor(0, 1);
		lcd.print(day(t));
		lcd.print(F(" "));
		lcd.print(month(t));
		lcd.print(F(" "));
		lcd.print(year(t));
	}

} display = Display_c();

// // note: lcd.write() {0~64 == ??}, {65~90 == A~Z}, {91~96 == ??}, {97~122 == a~z}, {123-127 == ??}
