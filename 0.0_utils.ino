/* 
  Takes seconds and computes hh:mm:ss string
  @param {uint32_t} _seconds - the time to convert

  @return {String} - time as hh:mm:ss
*/
String formatTimeToString(uint32_t _s){ 
  // get base time from seconds count
  uint16_t hours = _s / 3600;
  _s -= (hours * 3600);
  uint8_t minutes = _s / 60;
  _s -= (minutes * 60);
  uint8_t seconds = _s % 60;

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
String formatHoursWithTenths(int32_t _s) {
  return String((_s / 3600.), 1);
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
  Calculates EMA
  @param {float} newReading - new value to add into the EMA
  @param {float} prevEMA - previous EMA value
  @param {uint16_t} duration - duration over which the EMA is calculated

  @return {float} - calculated EMA
*/
float calcEma(float newReading, float prevEMA, uint16_t duration)
{
	return (newReading * (2. / (1 + duration)) + prevEMA * (1 - (2. / (1 + duration))));
}

/* 
  Limits decimals of the number
  @param {float} num - incoming value to limit
  @param {uint8_t} decimals - how many decimal places to keep

  @return {String} - number limited to the decimal places
*/
String limitDecimals(float num, uint8_t decimals = 1) {
  // save for the end
  bool isNegative = num < 0;

  // convert to positive for the calculation
  num = abs(num);		 
  uint32_t multiplier = pow(10.0, decimals);

  uint16_t truncatedNum = uint16_t(num); // 65.55 => 65

  // num * multiplier - truncatedNum * multiplier:
  // (65.55, 1) => uint32_t(65.55 * (10)) - uint32_t(65 * (10)) => 655 - 650 => 5
  // returns => 65.5
  // (65.55, 2) => uint32_t(65.55 * (100)) - uint32_t(65 * (100)) => 6554 - 6500 => 55
  // returns => 65.55
  String decimalString = String(
      uint32_t(num * multiplier)					 
      - uint32_t(truncatedNum * multiplier) 
  );

  return (isNegative ? "-" : "") + String(truncatedNum) + "." + decimalString;
}
