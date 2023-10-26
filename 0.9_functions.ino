void airSensorNanCheck() {
	if (
			isnan(air[0].humid) ||
			isnan(air[0].tempF)
  ) {
		if (
				isnan(air[1].humid) ||
				isnan(air[1].tempF)
    ) {
			Serial.println(F("ERROR BOTH SENSORS "));
		} else {
			air[0].humid = air[1].humid;
			air[0].tempF = air[1].tempF;

			if (errorCounter1 == 30)
			{
				Serial.print(F("DHT.main error! "));
				errorCounter1 = 0;
			}
			errorCounter1++;
		}
	} else if (
			isnan(air[1].humid) ||
			isnan(air[1].tempF)
  ) {
		air[1].humid = air[0].humid;
		air[1].tempF = air[0].tempF;

		if (errorCounter2 == 30) {
			Serial.print(F("DHT.upstairs error! "));
			errorCounter2 = 0;
		}
		errorCounter2++;
	}
}

const uint8_t AIR_SENSOR_OUTLIER_DISTANCE = 20;
void airSensorOutlierCheck() {
  // don't run the first time
	if (air[0].currentEMA[2] == 0) { 
    return;
	}

  if (
      air[0].tempF > AIR_SENSOR_OUTLIER_DISTANCE + air[0].currentEMA[2] ||
      air[0].tempF < (0-AIR_SENSOR_OUTLIER_DISTANCE) + air[0].currentEMA[2]
  ) {
    // out of range, set to other sensor (hoping it is within range);
    if (DEBUG) {
      Serial.print(F(" DHT.main outlier! "));
      Serial.print(air[0].tempF);
      Serial.print(F(" vs EMA_Long "));
      Serial.print(air[0].currentEMA[2]);
    }

    air[0].tempF = air[1].tempF;
  } else if (
      air[1].tempF > 20 + air[1].currentEMA[2] ||
      air[1].tempF < -20 + air[1].currentEMA[2]
  ) {
    // out of range, set to other sensor (hoping it is within range);
    if (DEBUG) {
      Serial.print(F(" DHT.upstairs outlier! "));
      Serial.print(air[1].tempF);
      Serial.print(F(" vs EMA_Long "));
      Serial.print(air[1].currentEMA[2]);
    }

    air[1].tempF = air[0].tempF;
  }
}

double calcFloorBatteryCapacity() {
// calculate temperature offset to account for stored floor heat
	// taking into account the floor stored heat
	int16_t floorRange = floorEmaAvg - MIN_FLOOR_READ;

	// limit the range 0-150
	floorRange = max(0, min(MAX_FLOOR_OFFSET, floorRange));

	// range 0-150 * 100 / max offset of 150 == percentage of 0~100
	uint8_t floorRangePercent = uint8_t(floorRange * 100. / MAX_FLOOR_OFFSET);
	// multiply ratio by maxTempAdjust to get the total offset, subtract 1  for funsies, so cold floor feels like lower temp
	return (floorRangePercent * MAX_TEMP_ADJUST_100X / (100. * 100.)) - 1; 
}

void updateFloorTemps () {
	// update floor emas
	for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++)
		floorSensor[i].update();

	// floor thermistor difference check, for outlier errors
	uint16_t diff= uint16_t(abs(floorSensor[0].ema - floorSensor[1].ema));
	if (diff> 100) {
    DEBUG_floorReadError(diff);

    // use the other reading if it's faulty
		if (floorSensor[0].ema > floorSensor[1].ema)
			floorSensor[1].ema = floorSensor[0].ema;
		else
			floorSensor[0].ema = floorSensor[1].ema;
	}

	floorEmaAvg = (floorSensor[0].ema + floorSensor[1].ema) / FLOOR_SENSOR_COUNT;							// avg two readings
	floorEmaAvgSlow = (floorSensor[0].slowEma + floorSensor[1].slowEma) / FLOOR_SENSOR_COUNT; // avg two readings

  // used to offset setpoint.
  // As floor warms up this will be higher, so pump will cut off sooner or start later
  // As floor cools, this will be lower so pump will cut off later or start sooner
	// the warmer the floor, the higher this will be. setPoint - floorOffset == target temperature
	floorOffset = calcFloorBatteryCapacity();
}

void updateTemps() {
	// floor temp stuff
  updateFloorTemps();

	// air temp stuff
	for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++)
		air[i].readTemp();

	// check for errors in two main sensors
	airSensorNanCheck();

	// check for strange readings (>20 from long avg) on two main air sensors
	airSensorOutlierCheck();

	// update highs/lows records (for all air sensors)
	for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++)
		air[i].updateRecords();

	// calculate weighted air temp
	weightedAirTemp = (double)(air[0].getTempEma(true) + air[1].getTempEma(true)) / (air[0].WEIGHT + air[1].WEIGHT);

	DEBUG_airEmas();

	// counter for what? other than DEBUG_emaWater
	if (tempDispCounter2 >= 24) { // every minute:
		//        DEBUG_emaWater();

		tempDispCounter2 = 0;
	}
	tempDispCounter2++;

}

void countWater() {
	waterPulseCount++;
}

void waterResetTotal() {
	totalMilliLitres = 0;
}

// void calcFlow() {
//     if (waterPulseCounter != 0) {
//         float thisDuration = (currentTime - (prevLoopStartTime + 750)) / 1000;
//         // this cycle's cycleDuration in seconds
//         // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.    (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
//         l_minute = (waterPulseCounter / 7.5) / thisDuration; // divide by 2.5 because this is on a 2.5s timer  // returns vol per minute extrapolated from 2.5s
//         totalVolume += (l_minute * thisDuration / 60);  // adds vol from the last 2.5s to totalVol
//         waterPulseCounter = 0;
//     }
// }

// float avg(int inputVal) {
//   static int arrDat[16];
//   static int pos;
//   static long sum;
//   pos++;
//   if (pos >= 16) pos = 0;
//   sum = sum - arrDat[pos] + inputVal;
//   arrDat[pos] = inputVal;
//   return (float)sum / 16.0;
// }

// void pidAutoUpdate() {
//     if (_myPID.autoTune) // Avoid dereferencing nullptr after _myPID.clearAutoTune()
//     {
//       switch (_myPID.autoTune->autoTuneLoop()) {
//         case _myPID.autoTune->AUTOTUNE:
//           weightedAirTemp = weightedAirTemp;
//           analogWrite(5, Output);
//           break;

//         case _myPID.autoTune->TUNINGS:
//           _myPID.autoTune->setAutoTuneConstants(&Kp, &Ki, &Kd); // set new tunings
//           _myPID.SetMode(QuickPID::AUTOMATIC); // setup PID
//           _myPID.SetSampleTimeUs(sampleTimeUs);
//           _myPID.SetTunings(Kp, Ki, Kd, POn, DOn); // apply new tunings to PID
//           setPoint = 500;
//           break;

//         case _myPID.autoTune->CLR:
//           if (!pidLoop) {
//             _myPID.clearAutoTune(); // releases memory used by AutoTune object
//             pidLoop = true;
//           }
//           break;
//       }
//     }
// }

// void pidLoopUpdate() {
//     if (pidLoop) {
//         _myPID.Compute();
//         analogWrite(5, Output);
//     }
// }


/*
Check if floor is cold
@return {bool}
*/
bool isFloorCold() {
  return (
    floorEmaAvg < FLOOR_WARMUP_TEMPERATURE || 
    floorEmaAvgSlow < FLOOR_WARMUP_TEMPERATURE
  );
}

/*
Compares long vs med EMAs for given air sensor, and limits the result
Used to detect trend in air temp so we can offset the setpoint (stop earlier etc)
@param {uint8_t} airSensorIndex - index of air sensor
@param {double} limit - temperature limit
*/
double updateAirLimitedTempDifference(uint8_t airSensorIndex = 0, double limit = 2) {
  float prev = air[airSensorIndex].currentEMA[2];
  float current = air[airSensorIndex].currentEMA[1];
  // get absolute difference
  double diff = double(prev - current);
  // limit the diff to the limit
  // save difference to global var
  difference = diff >= 0 ? min(diff, limit) : max(diff, -limit);
}

/*
Checks air temp difference/trend, then applies that to setpoint to
determine if we're warm enough

// take the air temp medium ema compared to current ema
// medium ema - current === difference,
// (or slow ema - medium ema === difference)
//    this way a sudden drop in temp (from opening the door) will
//      not trigger the pump quickly
// if difference is positive this means we have extra capacity of heat
// if difference is negative this means we don't have extra capacity of heat

// NOTE: Only uses Air[0]!!

@return {bool}
*/
bool isAboveAdjustedSetPoint() {
  updateAirLimitedTempDifference(0);
  return weightedAirTemp >= setPoint - difference;
}
