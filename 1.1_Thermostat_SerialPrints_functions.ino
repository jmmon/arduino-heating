void DEBUG_startup2()
{
	if (!DEBUG) return;

  Serial.println();
  Serial.print(F("Version "));
  Serial.println(VERSION_NUMBER);
  Serial.println(F("DHTxx test!"));
}

void DEBUG_emaWater2()
{
	if (!DEBUG) return;

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

void DEBUG_airEmas2()
{
	if (tempDispCounter >= 4)
	{ // every 4 reads = 10 seconds, calculate trend and print info
		tempDispCounter = 0;

    if (!DEBUG) return;

    Serial.print(air[0].currentEMA[0]);
    if (air[0].currentEMA[0] != air[1].currentEMA[0])
    {
      Serial.print(F("/"));
      Serial.print(air[1].currentEMA[0]);
    }
    Serial.print(F("  "));
	}
	tempDispCounter++;
}

void DEBUG_cycleTimes2(uint32_t seconds) {
  if (!DEBUG) return;

  Serial.print(F("                          Total cycle time: "));
  Serial.println(formatTimeToString(seconds));
}

void DEBUG_highsLowsFloor2()
{
  if (!DEBUG) return;

  Serial.println();
  Serial.print(F(" Set: "));
  Serial.print(setPoint, 1);
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
