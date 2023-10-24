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
