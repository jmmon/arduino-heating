 #include <Arduino.h>
#include <TimeLib.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// #include <AutoPID.h>
#include <DHT.h>
// #include <dhtnew.h>
#include <ArduPID.h>

#define TONE_USE_INT
#define TONE_PITCH 432
#include <TonePitch.h>

#include <math.h>

/*
  NOTE: for some reason, I can't move this fn to utils.h or 
    consts.h without weird compile errors...
  Function to calculate EMA
  @param {uint16_t} reading - next value to add to the EMA
  @param {uint16_t} lastEma - previous EMA
  @param {uint16_t} _days - interval over which the EMA is calculated

  @return {float} - new EMA
*/
float calcEma(uint16_t reading, uint16_t lastEma, uint16_t _days) {
	return (reading * (2. / (1 + _days)) + lastEma * (1 - (2. / (1 + _days))));
}

#include "consts.h"
#include "utils.h"

#include "class.AirSensor.h"
#include "class.WaterTank.h"
#include "class.FloorSensor.h"
#include "class.Pump.h"
#include "class.Thermostat.h"

/*
  NOTE: for some reason, I must keep 0.9, 2.0, and 9.9 files as `ino` files
  or else it messes something up (starts the pump when it should be off)
*/
