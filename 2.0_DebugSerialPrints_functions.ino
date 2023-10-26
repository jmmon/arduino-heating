void DEBUG_startup() {
	if (!DEBUG) {
    return;
	}

  Serial.println();
  Serial.print(F("Version "));
  Serial.println(VERSION_NUMBER);
  Serial.println(F("DHTxx test!"));
}

void DEBUG_dhtOutlier(uint8_t index) {
    if (!DEBUG) {
      return;
    }

    Serial.print(F(" DHT."));
    Serial.print(index == 0 ? F("main") : F("upstairs"));
    Serial.print(F(" outlier! "));

    Serial.print(air[index].tempF);
    Serial.print(F(" vs EMA_Long "));
    Serial.print(air[index].currentEMA[2]);
}

void DEBUG_buttonRead(uint16_t _buttonRead) {
  if (!DEBUG) {
    return;
  }
  Serial.println(_buttonRead);
}


void DEBUG_dhtNaN(uint8_t index = 2) {
  if (!DEBUG) {
    return;
  }

  Serial.print(F("DHT"));
  Serial.print(index > 1 
    ? F(" -- BOTH SENSORS") 
    : index == 0 
      ? F(".main") 
      : F(".upstairs")
  );
  Serial.print(F(" error! "));
}

uint8_t floorReadErrorCounter = 0;
void DEBUG_floorReadError(uint16_t diff) {
  if (!DEBUG) {
    return;
  }
  // every 4 reads = 10 seconds, calculate trend and print info
	if (floorReadErrorCounter < 4) { 
    return;
  }
  floorReadErrorCounter = 0;

  Serial.print(F("Floor Read Error. Difference: "));
  Serial.println(diff);

	floorReadErrorCounter++;
}

void DEBUG_printHighsAndLows() {
  Serial.print(F("(HIGHS "));
  Serial.print(air[0].highest);
  if (air[0].highest != air[1].highest) {
    Serial.print(F("/"));
    Serial.print(air[1].highest);
  }
  Serial.print(F(" LOWS "));
  Serial.print(air[0].lowest);
  if (air[0].lowest != air[1].lowest) {
    Serial.print(F("/"));
    Serial.print(air[1].lowest);
  }
  Serial.print(F(") "));
}

void DEBUG_emaWater() {
	if (!DEBUG) {
    return;
	}

  Serial.println();
  Serial.print(F(" °F "));
  Serial.print(F("  EMA_Long."));
  Serial.print(air[0].currentEMA[2]);
  if (air[0].currentEMA[2] != air[1].currentEMA[2]) {
    Serial.print(F("/"));
    Serial.print(air[1].currentEMA[2]);
  }
  Serial.print(F("  _Med."));
  Serial.print(air[0].currentEMA[1]);
  if (air[0].currentEMA[1] != air[1].currentEMA[1]) {
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

void DEBUG_airEmas() {

  if (!DEBUG) {
    return;
  }
   // every 4 reads = 10 seconds, calculate trend and print info
	if (tempDispCounter < 4) {
    return;
	}

  tempDispCounter = 0;

  Serial.print(F("Current Temp: "));
  Serial.print(air[0].currentEMA[0]);
  if (air[0].currentEMA[0] != air[1].currentEMA[0]) {
    Serial.print(F("/"));
    Serial.print(air[1].currentEMA[0]);
  }
  Serial.print(F("                                                "));
  DEBUG_printHighsAndLows();

	tempDispCounter++;
}

void DEBUG_heartbeat() {
  if (!DEBUG) {
    return;
  }

  Serial.println();
  Serial.print(F("Heartbeat ratio: "));
  Serial.print(heartbeatOnOffRatioEMA, 2);
  Serial.print(F(" / "));
  Serial.print(HEARTBEAT_RATIO_THRESHOLD, 2);
  Serial.print(F(" --- is ABOVE threshold: "));
  Serial.print(isHeartbeatRatioAboveThreshold);
}

uint8_t highsLowsFloorDisplayCounter = 0;
void DEBUG_highsLowsFloor() {
  if (!DEBUG) {
    return;
  }

  // every 4 reads = 10 seconds, calculate trend and print info
  if (highsLowsFloorDisplayCounter < 4) {
    return;
  }

  highsLowsFloorDisplayCounter = 0;

  Serial.println();
  Serial.print(F(" Set: "));
  Serial.print(setPoint, 1);
  Serial.print(F("°F"));
  Serial.print(F("   Floor: "));
  Serial.print(floorSensor[0].ema);
  if (floorSensor[0].ema != floorSensor[1].ema) {
    Serial.print(F(" / ")); // +3
    Serial.print(floorSensor[1].ema); // +3
  } else {
    Serial.print(F("      "));
  }

  Serial.print(F("                                 *PWM: "));
  Serial.print(pump.getStatusString());


  // print cycle time info
  if (timeRemaining > 0) {
    Serial.print(F(" t: "));
    uint32_t t = timeRemaining;
    uint16_t hours = t / 3600;
    t -= (hours * 3600);
    uint16_t minutes = t / 60;
    t -= (minutes * 60);
    uint16_t seconds = t;
    if (seconds >= 60) {
        minutes += 1;
        seconds -= 60;
    }
    if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }

    if (hours > 0) {
        if (hours < 10) Serial.print(F("0"));
        Serial.print(hours);
        Serial.print(F(":"));
    }
    if (minutes > 0) {
        if (minutes < 10) Serial.print(F("0"));
        Serial.print(minutes);
        Serial.print(F(":"));
    }
    if (seconds > 0) {
        if (seconds < 10) Serial.print(F("0"));
        Serial.print(seconds);
    }
  }
  Serial.println();

  highsLowsFloorDisplayCounter ++;
}
