












/**
 * prepares strings for lcd display?
 * class display
 *    
 * 
 * 
 */


// class Thermostat_C {
//     private:
//         uint8_t BUTTONS_PIN = A2;
// //        Air_C air[4] = {
// //            Air_C(&dht[0], "MAIN", 1),
// //            Air_C(&dht[1], "UPSTAIRS", 2),
// //            Air_C(&dht[2], "OUTSIDE", 2),
// //            Air_C(&dht[3], "GREENHOUSE", 2),
// //        };
// //
//     public:
// //        uint8_t weightedTemp;
// //        
//         Thermostat_C() {
//             pinMode(BUTTONS_PIN, INPUT);
//         }
// //        
// //        //what can the thermostat do?
// //        /**
// //         * 
// //         * update water?
// //         * update setpoint (buttons)
// //         * update pages (display pages)
// //         * // check for errors in two main sensors
// //         */
// //
// //        lcdUpdate() {
// //            lcdCounter--;
// //            if (lcdCounter <= 0) { // update LCD
// //                lcdCounter = LCD_INTERVAL_QTR_SECS;
// //                
// //                lcdPage ++;
// //                if (lcdPage >= LCD_PAGE_MAX) { // page 4 == page 0
// //                    lcdPage = 0;
// //                }
// //
// //                lcdChangePage(); // display page
// //            }
// //        }
// //
//         updateSetpoint() { // every 0.25 seconds
//            int8_t buttonStatus = 0;
//            uint16_t buttonRead = analogRead(BUTTONS_PIN);
//            if (buttonRead > 63) {
//                if (buttonRead > 831) {
//                    buttonStatus = 1;
//                    for (uint8_t k = 0; k < 2; k++) {  // reset record temps
//                        air[k].lowest = air[k].tempF;
//                    }
//                } else {
//                    buttonStatus = -1;
//                    for (uint8_t k = 0; k < 2; k++) {
//                        air[k].highest = air[k].tempF;
//                    }
//                }
               
//                Setpoint += double(0.5 * buttonStatus);
//                lcdPageSet(buttonRead);
               
//                pump.checkAfter(); // default 3 seconds
//            }
//         }


//         adjustPid() { // every 20ms
//             static int PATTERN[5] = {1,2,1,1,2};
//             static int pattern[5];
//             static int count;
//             int reading;
//             count++;
//             if (count >= 5) count = 0;
//             pattern[count] = reading;
//         }

//         // float avg(int inputVal) {
// //   static int arrDat[16];
// //   static int pos;
// //   static long sum;
// //   pos++;
// //   if (pos >= 16) pos = 0;
// //   sum = sum - arrDat[pos] + inputVal;
// //   arrDat[pos] = inputVal;
// //   return (float)sum / 16.0;
// // }
        
// //
// //        String readFloors() {
// //            for (uint8_t i = 0; i < FLOOR_SENSOR_COUNT; i++) { // update floor emas
// //                floorSensor[i].update();
// //            }
// //
// //            int difference = int(abs(floorSensor[0].ema - floorSensor[1].ema)); // floor sensors error check
// //
// //            String err = "";
// //            if (difference > 80) { // floor thermistor difference check
// //                String err = "FLR ERR " + difference;
// //                
// //                if (floorSensor[0].ema > floorSensor[1].ema) {
// //                    floorSensor[1].ema = floorSensor[0].ema;
// //                } else {
// //                    floorSensor[0].ema = floorSensor[1].ema;
// //                }
// //            }
// //            floorEmaAvg = (floorSensor[0].ema + floorSensor[1].ema) / FLOOR_SENSOR_COUNT; // avg two readings
// //
// //            return err;
// //        }
// //
// //        updateTemp() {
// //            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
// //                air[i].readTemp();
// //            }
// //
// //            String err = errorCheck();
// //
// //            if (err != "") {
// //                lcdPageErr(err);
// //            }
// //
// //            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) { // update highs/lows records
// //                air[i].update();
// //            }
// //
// //            weightedTemp = (air[0].WEIGHT * air[0].currentEMA[0] + air[1].WEIGHT * air[1].currentEMA[0]) / (air[0].WEIGHT + air[1].WEIGHT);
// //            temperature = weightedTemp;
// //
// //            for (uint8_t i = 0; i < AIR_SENSOR_COUNT; i++) {
// //                air[i].updateEmaTrend();
// //            }
// //            avgTrend = (air[0].trendEMA + air[1].trendEMA) / 2.;
// //
// //            //debugAirEmas();
// //
// //            if (tempDispCounter2 >= 24) {  // every minute: 
// //                //debugEmaWater();
// //
// //                if (changePerHourMinuteCounter < 59) {
// //                    changePerHourMinuteCounter++;
// //                }
// //                tempDispCounter2 = 0;
// //                
// //                for (uint8_t k=0; k < changePerHourMinuteCounter - 1; k++) {
// //                    last59MedEMAs[k] = last59MedEMAs[k+1];
// //                }
// //                last59MedEMAs[58] = (air[0].currentEMA[1] + air[1].currentEMA[1]) / 2;
// //            }
// //
// //            tempDispCounter2++;
// //
// //            readFloors();
// //        }
// //
// //        String errorCheck() {
// //            String err = "";
// //            // check for strange readings (>20 from long avg) on two main air sensors
// //            if (air[0].currentEMA[2] != 0) { // don't run the first time 
// //                if (air[0].tempF > 20 + air[0].currentEMA[2] || air[0].tempF < -20 + air[0].currentEMA[2]) {
// //                    // out of range, set to other sensor (hoping it is within range);
// //                    err = "ERR MAIN " + String((air[0].currentEMA[2] - air[0].tempF));
// //
// //                    air[0].tempF = air[1].tempF;
// //                }
// //                if (air[1].tempF > 20 + air[1].currentEMA[2] || air[1].tempF < -20 + air[1].currentEMA[2]) {
// //                    // out of range, set to other sensor (hoping it is within range);
// //                    err = "ERR Upst " + String((air[1].currentEMA[2] - air[1].tempF));
// //                        
// //                    air[1].tempF = air[0].tempF;
// //                }
// //            }
// //
// //
// //            if (isnan(air[0].humid) || isnan(air[0].tempF)) { // check for errors in two main sensors
// //                
// //                if (isnan(air[1].humid) || isnan(air[1].tempF)) {
// //                    err = "NaN BOTH ";
// //                } else {
// //                    if (errorCounter1 == 30) {
// //                        err = "NaN MAIN ";
// //                        errorCounter1 = 0;
// //                    }
// //                    air[0].humid = air[1].humid;
// //                    air[0].tempF = air[1].tempF;
// //                }
// //                errorCounter1++;
// //            } else {
// //                if (isnan(air[1].humid) || isnan(air[1].tempF)) {
// //                    if (errorCounter2 == 30) {
// //                        err = "NaN UPSTAIRS ";
// //                        errorCounter2 = 0;
// //                    }
// //                    air[1].humid = air[0].humid;
// //                    air[1].tempF = air[0].tempF;
// //                    errorCounter2++;
// //                }
// //            }
// //            return err;
// //        }
// //

// } thermostat = Thermostat_C();
