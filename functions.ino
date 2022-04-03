void updateTEMP() {
    // air temp stuff
    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
        air[i].readTemp();
    }

    // check for errors in two main sensors
    if (isnan(air[0].humid) || isnan(air[0].tempF)) {
        if (isnan(air[1].humid) || isnan(air[1].tempF)) {
            Serial.println(F("ERROR BOTH SENSORS "));
        } else {
            air[0].humid = air[1].humid;
            air[0].tempF = air[1].tempF;
            
            if (errorCounter1 == 30) {
                Serial.print(F("DHT.main error! "));
                errorCounter1 = 0;
            }
            errorCounter1++;
        }
    } else {
        if (isnan(air[1].humid) || isnan(air[1].tempF)) {
            air[1].humid = air[0].humid;
            air[1].tempF = air[0].tempF;
            
            if (errorCounter2 == 30) {
                Serial.print(F("DHT.upstairs error! "));
                errorCounter2 = 0;
            }
            errorCounter2++;
        }
    }

    // check for strange readings (>20 from long avg) on two main air sensors
    if (air[0].currentEMA[2] != 0) { // don't run the first time 
        if (air[0].tempF > 20 + air[0].currentEMA[2] || air[0].tempF < -20 + air[0].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            if (DEBUG) {
                Serial.print(F(" DHT.main outlier! ")); Serial.print(air[0].tempF); 
                Serial.print(F(" vs EMA_Long ")); Serial.print(air[0].currentEMA[2]);
            }

            air[0].tempF = air[1].tempF;
        }
        if (air[1].tempF > 20 + air[1].currentEMA[2] || air[1].tempF < -20 + air[1].currentEMA[2]) {
            // out of range, set to other sensor (hoping it is within range);
            if (DEBUG) {
                Serial.print(F(" DHT.upstairs outlier! ")); Serial.print(air[1].tempF); 
                Serial.print(F(" vs EMA_Long ")); Serial.print(air[1].currentEMA[2]);
            }

            air[1].tempF = air[0].tempF;
        }
    }

    for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records (for all air sensors)
        air[i].updateRecords();
    }

    Input = (double)(air[0].getTempEma() * air[0].WEIGHT + air[1].getTempEma() * air[1].WEIGHT) / (air[0].WEIGHT + air[1].WEIGHT);

    //debugAirEmas();

    // counter for what? other than debugEmaWater
    if (tempDispCounter2 >= 24) {  // every minute: 
//        debugEmaWater();

        if (changePerHourMinuteCounter < 59) {
            changePerHourMinuteCounter++;
        }
        tempDispCounter2 = 0;
        
        // for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
        //     last59MedEMAs[k] = last59MedEMAs[k+1];
        // }
        // last59MedEMAs[58] = (air[0].currentEMA[1] + air[1].currentEMA[1]) / 2;
    }
    tempDispCounter2++;






    // floor temp stuff
    for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) { // update floor emas
        floorSensor[i].update();
    }

    
    // floor thermistor difference check, for outlier errors
    int difference = int(abs(floorSensor[0].ema - floorSensor[1].ema));
    if (difference > 80) {
        if (DEBUG) {
            Serial.print(F("Floor Read Error. Difference: "));
            Serial.println(difference);
        }

        if (floorSensor[0].ema > floorSensor[1].ema) {
            floorSensor[1].ema = floorSensor[0].ema;
        } else {
            floorSensor[0].ema = floorSensor[1].ema;
        }
    }
    floorEmaAvg = (floorSensor[0].ema + floorSensor[1].ema) / FLOOR_SENSOR_COUNT; // avg two readings
    floorEmaAvgSlow = (floorSensor[0].slowEma + floorSensor[1].slowEma) / FLOOR_SENSOR_COUNT; // avg two readings
}


void countWater() {
    waterPulseCount++;
}




void waterResetTotal() {
    totalMilliLitres = 0;
}


// void calcFlow() {
//     if (waterPulseCounter != 0) {
//         float thisDuration = (currentTime - (last250ms + 750)) / 1000;
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
//           Input = Input;
//           analogWrite(5, Output);
//           break;
  
//         case _myPID.autoTune->TUNINGS:
//           _myPID.autoTune->setAutoTuneConstants(&Kp, &Ki, &Kd); // set new tunings
//           _myPID.SetMode(QuickPID::AUTOMATIC); // setup PID
//           _myPID.SetSampleTimeUs(sampleTimeUs);
//           _myPID.SetTunings(Kp, Ki, Kd, POn, DOn); // apply new tunings to PID
//           Setpoint = 500;
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
