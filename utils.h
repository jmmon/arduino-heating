/* 
  Takes seconds and computes hh:mm:ss string
  @param {uint32_t} _seconds - the time to convert
  @return {String} - time as hh:mm:ss
*/
String formatTimeToString(uint32_t _seconds){ 
  // get base time from seconds count
  uint16_t hours = _seconds / 3600;
  _seconds -= (hours * 3600);
  uint8_t minutes = _seconds / 60;
  //_seconds -= (minutes * 60);
  uint8_t seconds = _seconds % 60;

  // rollover if necessary
  if (seconds >= 60)
  {
    minutes += 1;
    seconds -= 60;
  }
  if (minutes >= 60)
  {
    hours += 1;
    minutes -= 60;
  }

  return (((hours < 10) ? ("0") : ("")) + String(hours) +
          ((minutes < 10) ? (":0") : (":")) + (String(minutes)) +
          ((seconds < 10) ? (":0") : (":")) + String(seconds));
}

/* 
  Takes seconds and returns hours, limited to tenths of hours
  @param {uint32_t} _seconds - the time to convert
  @return {String} - hours with tenths as h.h
*/
String formatHoursWithTenths(int32_t _seconds)
{
  return String((_seconds / 3600.), 1);
}

/* 
  Takes ms and returns seconds, rounded
  @param {uint32_t} _currentTime - the time to convert, in ms
  @return {uint32_t} - seconds, rounded
*/
uint32_t getTotalSeconds(uint32_t _currentTime)
{
  return uint32_t(round(_currentTime / 1000.));
}

/* 
  Limits number decimals to {decimals} places
  @param {float} num - the time to convert, in ms
  @param {uint8_t} [decimals=1] - decimals to keep
  @return {String}
*/
String limitDecimals(float num, uint8_t decimals = 1) {
  // save isNegative to apply at the end
  bool isNegative = num < 0;						 
  // convert to positive for the calculations
  num = (isNegative) ? 0 - num : num;		 
  // get our multiplier/divisor
  uint32_t mult = pow(10.0, decimals); 

  // e.g. num == 65.55, decimals == 1: => 65.6 (rounded)
  uint16_t mainInteger = uint16_t(num); // 65.55 => 65

  // gets 10x, so 65.55 => 65.55 * 10 => round(655.5) => 656
  // mainInteger * 10 => 65 * 10 => 650
  // 656 - 650 => 6
  String decimal = String(uint32_t(round(num * mult)) - uint32_t(mainInteger * mult));																 

  return (isNegative ? "-" : "") + String(mainInteger) + "." + decimal;
}

/* 
  Calculates EMA
  @param {float} newValue - next value to add to the EMA
  @param {float} prevEMA - previous EMA
  @param {uint32_t} interval - interval over which the EMA is calculated
  @return {float} - new EMA
*/
// float calcEMAFromFloats(float newValue, float prevEMA, uint32_t interval) {
//   return (newValue * (2. / (1 + interval)) + prevEMA * (1 - (2. / (1 + interval))));
// }



// // move this to functions?
// float calcEma(uint16_t reading, uint16_t lastEma, uint16_t _days)
// {
// 	return (reading * (2. / (1 + _days)) + lastEma * (1 - (2. / (1 + _days))));
// }
