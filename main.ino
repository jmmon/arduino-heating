#include <Arduino.h>
#include <ArduPID.h>
#include <TimeLib.h>

void timeSetup() {

	// get and display time works!
	const String startTimeDate = __TIME__ " "__DATE__; // "hrs:mins:secs Jan 10 2022" example

	char *behind; // points to behind the first decimal number

	const uint8_t hrs = strtol(startTimeDate.c_str(), &behind, 10); // get first number
	const uint8_t mins = strtol(++behind, &behind, 10);							// start ++behind, and get second number, repoint behind to new behind
	const uint8_t secs = strtol(++behind, &behind, 10);							// etc

	String mon = ++behind; // save as string so we can use substring to get the 3 letter month
	mon = mon.substring(0, 3);
	const uint8_t dys = strtol(4 + behind, &behind, 10); // days jump 4 ahead of behind so we skip over the month (since it was not removed)
	const uint8_t yrs = strtol(++behind, &behind, 10);	 // last is the year

	uint8_t months;
	if (mon == "Jan")
		months = 1; // break down 3 letter month into int
	else if (mon == "Feb")
		months = 2;
	else if (mon == "Mar")
		months = 3;
	else if (mon == "Apr")
		months = 4;
	else if (mon == "May")
		months = 5;
	else if (mon == "Jun")
		months = 6;
	else if (mon == "Jul")
		months = 7;
	else if (mon == "Aug")
		months = 8;
	else if (mon == "Sep")
		months = 9;
	else if (mon == "Oct")
		months = 10;
	else if (mon == "Nov")
		months = 11;
	else if (mon == "Dec")
		months = 12;

	setTime(hrs, mins, secs, dys, months, yrs);
}

void setup()
{
	Serial.begin(9600);
	pump.stop(true);

	for (uint8_t k = 0; k < AIR_SENSOR_COUNT; k++)
	{
		dht[k].begin();
		//        dhtnew[k].setType(22);
	}

	timeSetup();
	Tstat.initialize();
	DEBUG_startup();
	waterTank.init();
	updateTEMP();

	pinMode(WATER_FLOW_PIN, INPUT);
	digitalWrite(WATER_FLOW_PIN, HIGH);

	WATER_FLOW_INTERRUPT = digitalPinToInterrupt(WATER_FLOW_PIN); // set in setup
																																//    attachInterrupt(WATER_FLOW_INTERRUPT, waterPulseCount, FALLING);

	// // AutoPID setup:
	// // if temperature is more than 4 degrees below or above Setpoint, OUTPUT will be set to min or max respectively
	// //myPID.setBangBang(4);
	// myPID.setBangBang(100); // so high that it disables the feature; setpoint +- 100
	// myPID.setTimeStep(1000);// set PID update interval

	// ArduPID setup:
	ArduPIDController.begin(&Input, &Output, &Setpoint, Kp, Ki, Kd);
	const unsigned int _minSamplePeriodMs = 1000;
	ArduPIDController.setSampleTime(_minSamplePeriodMs);
	ArduPIDController.setOutputLimits(outputMin, outputMax);
	const double windUpMin = -10;
	const double windUpMax = 10;
	ArduPIDController.setWindUpLimits(windUpMin, windUpMax);
	ArduPIDController.start();

	tone(TONE_PIN, NOTE_C5, 100); // quick beep at startup

	last250ms = millis();
}

void loop()
{
	currentTime = millis();

	// // AutoPID loop:
	// myPID.run(); // every second

	// ArduPID loop:
	ArduPIDController.compute();

	// Tstat.update(); // new spot

	// 250ms Loop:
	if (currentTime - last250ms >= 250)
	{ 
		last250ms += 250;
   // update Setpoint
		Tstat.update(); // old spot

		// 1000ms Loop:
		if (ms1000ctr >= 4)
		{
			ms1000ctr = 0;
      pump.update();
      
			// calc flowrate (not working)
			// calcFlow();
			// waterCalcFlow();
		}
		ms1000ctr++;

		// 2500ms Loop:
		if (ms2500ctr >= 10)
		{
			ms2500ctr = 0;

			waterTank.update();
			updateTEMP(); // read air temp
		}
		ms2500ctr++;
	}
}

/*
Insterrupt Service Routine ( not working )
 */
void waterPulseCounter()
{
	// Increment the pulse counter
	waterPulseCount++;
}

// not working
void waterCalcFlow()
{
	// Only process counters once per second
	if (waterPulseCount > 0)
	{
		// Disable the interrupt while calculating flow rate and sending the value to
		// the host
		detachInterrupt(WATER_FLOW_INTERRUPT);

		// Because this loop may not complete in exactly 1 second intervals we calculate
		// the number of milliseconds that have passed since the last execution and use
		// that to scale the output. We also apply the calibrationFactor to scale the output
		// based on the number of pulses per second per units of measure (litres/minute in
		// this case) coming from the sensor.
		flowRate = ((1000.0 / (millis() - oldTime)) * waterPulseCount) / calibrationFactor;

		// Note the time this processing pass was executed. Note that because we've
		// disabled interrupts the millis() function won't actually be incrementing right
		// at this point, but it will still return the value it was set to just before
		// interrupts went away.
		oldTime = millis();

		// Divide the flow rate in litres/minute by 60 to determine how many litres have
		// passed through the sensor in this 1 second interval, then multiply by 1000 to
		// convert to millilitres.
		flowMilliLitres = (flowRate / 60) * 1000;

		// Add the millilitres passed in this second to the cumulative total
		totalMilliLitres += flowMilliLitres;

		// // Print the flow rate for this second in litres / minute
		// Serial.print(int(flowRate));  // Print the integer part of the variable

		// // Print the cumulative total of litres flowed since starting
		// Serial.print(totalMilliLitres/1000);
		// Serial.print("L");

		// Reset the pulse counter so we can start incrementing again
		waterPulseCount = 0;

		// Enable the interrupt again now that we've finished sending output
		attachInterrupt(WATER_FLOW_INTERRUPT, waterPulseCount, FALLING);
	}
}
