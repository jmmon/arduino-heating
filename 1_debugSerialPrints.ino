//#include <Arduino.h>

void DEBUG_startup()
{
	if (DEBUG)
	{
		Serial.println();
		Serial.print(F("Version "));
		Serial.println(VERSION_NUMBER);
		Serial.println(F("DHTxx test!"));
	}
}

void DEBUG_emaWater()
{
	if (DEBUG)
	{
		Serial.println();
		Serial.print(F(" °F "));
		Serial.print(F("  EMA_Long."));
		Serial.print(air[0].currentEMA[2]);
		if (air[0].currentEMA[2] != air[1].currentEMA[2])
		{
			Serial.print(F("/"));
			Serial.print(air[1].currentEMA[2]);
		}
		Serial.print(F("  _Med."));
		Serial.print(air[0].currentEMA[1]);
		if (air[0].currentEMA[1] != air[1].currentEMA[1])
		{
			Serial.print(F("/"));
			Serial.print(air[1].currentEMA[1]);
		}

		Serial.print(F(" Water(Flow: "));
		Serial.print(l_minute);
		Serial.print(F(" l/m, totalVol: "));
		Serial.print(totalVolume);
		Serial.print(F(" liters, "));
		// check tank levels
		Serial.print(F("Level: "));
		Serial.print(waterTank.percent);
		Serial.print(F(" "));
	}
}

void DEBUG_airEmas()
{
	if (tempDispCounter >= 4)
	{ // every 4 reads = 10 seconds, calculate trend and print info
		tempDispCounter = 0;

		if (DEBUG)
		{
			Serial.print(air[0].currentEMA[0]);
			if (air[0].currentEMA[0] != air[1].currentEMA[0])
			{
				Serial.print(F("/"));
				Serial.print(air[1].currentEMA[0]);
			}
			Serial.print(F("  "));
		}
	}
	tempDispCounter++;
}

void DEBUG_highsLowsFloor()
{
	if (DEBUG)
	{
		// if (lastCycleDuration != 0) {  // if end of cycle:
		//     Serial.print(F(" ms:"));

		//     uint32_t t = lastCycleDuration;
		//     Serial.print(t);
		//     uint16_t hours = t / 3600000;
		//     t -= (hours * 3600000);
		//     uint16_t minutes = t / 60000;
		//     t -= (minutes * 60000);
		//     uint16_t seconds = t / 1000;
		//     t -= (seconds * 1000);
		//     if (seconds >= 60) {
		//         minutes += 1;
		//         seconds -= 60;
		//     }
		//     if (minutes >= 60) {
		//         hours += 1;
		//         minutes -= 60;
		//     }
		//     if (t >= 500 && (minutes > 0 || hours > 0)) {
		//         seconds += 1;
		//     }

		//     Serial.print(F("                          Total cycle time: "));

		//     if (hours > 0) {
		//         Serial.print(hours);
		//         Serial.print(F("h"));
		//     }
		//     if (minutes > 0) {
		//         Serial.print(minutes);
		//         Serial.print(F("m"));
		//     }
		//     if (seconds > 0) {
		//         Serial.print(seconds);
		//         Serial.print(F("s"));
		//     }
		//     if (hours == 0 && minutes == 0) {
		//         Serial.print(t);
		//     }
		//     //Serial.println();
		// }

		Serial.println();
		Serial.print(F(" Set: "));
		Serial.print(Setpoint, 1);
		Serial.print(F("°F"));
		Serial.print(F("   Floor: "));
		Serial.print(floorSensor[0].ema);
		if (floorSensor[0].ema != floorSensor[1].ema)
		{
			Serial.print(F(" / "));
			Serial.print(floorSensor[1].ema);
		}
		Serial.print(F("                                       *Pump"));
		Serial.print(pump.getStatusString());
		Serial.print(F("(HIGHS "));
		Serial.print(air[0].highest);
		if (air[0].highest != air[1].highest)
		{
			Serial.print(F("/"));
			Serial.print(air[1].highest);
		}
		Serial.print(F(" LOWS "));
		Serial.print(air[0].lowest);
		if (air[0].lowest != air[1].lowest)
		{
			Serial.print(F("/"));
			Serial.print(air[1].lowest);
		}
		Serial.print(F(")"));
		// if (changePerHourMinuteCounter == 60) {
		//     Serial.print(F("Temp Change per Hr (Medium EMA): "));
		//     float avgNow = (air[0].currentEMA[1] + air[1].currentEMA[1]) / 2;
		//     float diff = last59MedEMAs[0] - avgNow;
		//     Serial.print(diff);
		// }
		Serial.println();
	}
}
