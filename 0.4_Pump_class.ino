// #include <Arduino.h>

// Floor Sensor approximate readings examples
// ~489/ 518 cold floor
// 590/632 - 32m in to on-cycle
// 622/649 floor (warm but not hot)

// Pump PWM (speeds)
const uint8_t ON_PHASE_BASE_PWM = 135;                        // normal run speed
const uint8_t COLD_FLOOR_PWM_BOOST = 255 - ON_PHASE_BASE_PWM; // when cold, make it max
const uint8_t PULSE_PWM_AMOUNT = 27;                          // occasional boost (to prevent stalling)

// PWM boost during motor start phase
const uint8_t STARTING_PHASE_PWM_BOOST = 28; // ({sum(1-10)} == 55) + {1*remaining seconds}
const uint8_t STARTING_PHASE_INITIAL_STEP = 7; // initial drop amount during starting phase
uint8_t startingPhasePwmStepAdjust = 0;

// timers
const uint16_t ON_CYCLE_MINIMUM_SECONDS = 60;	 // 1m
const uint16_t OFF_CYCLE_MINIMUM_SECONDS = 5 * 60; // 5m
uint32_t timeRemaining = 0; // cycle time counter, to track if we're past the minimum cycle times

const uint8_t HEARTBEAT_CYCLE_PWM = 145; // slightly faster than normal
const uint16_t HEARTBEAT_CYCLE_MINIMUM_OFF_DURATION = 30 * 60; //30m
const uint32_t HEARTBEAT_ON_CYCLE_DURATION = 60; // 1m
const uint32_t HEARTBEAT_ON_RATIO_EMA_PERIOD = 60 * 60 * 24; // 1 day
uint32_t heartbeatCalculatedOffTime = HEARTBEAT_CYCLE_MINIMUM_OFF_DURATION - HEARTBEAT_ON_CYCLE_DURATION;

// AntiFreeze function
float heartbeatOnOffRatioEMA = 0;
uint32_t heartbeatTimer = 0;

// Occasional PWM boost (to prevent motor stalling)
const uint16_t PULSE_PWM_SECONDS_INTERVAL = 30 * 60; // seconds (every hour)
uint16_t pulsePwmCounter = 0;											// counts the seconds since last boost

// Delayed start phase: seconds to delay before checking/starting
static const uint8_t DELAY_SECONDS = 3;				 // seconds after setpoint change it checks which pump state to be in

// holds setpoint offset, to maintain constant feel
double difference = 0;
// if temp drops lower than this from the setpoint, start the pump regardless of timers
const uint8_t EMERGENCY_ON_TRIGGER_OFFSET = 5; // degrees from setpoint

class Pump_C
{
public:
	uint32_t accumOn = 0;		 // time spent On
	uint32_t accumAbove = 0; // total time spent above setpoint

	uint8_t pwm = 0; // holds motor PWM

	uint8_t state = 0;					// 0 === off, 1 === on, 2 === starting, 3 === delayedStart, 4 === ANTIFREEZE
	uint32_t cycleDuration = 0; // for this cycle

  // for ANTIFREEZE cycle, to detect when we can skip
  uint32_t lastOnCycleStartedSecondsAgo = 0;  // when turning on, reset this to 0 and count up (continuously, until next turning on will reset it to 0)
  uint32_t lastOnCycleDuration = 0;   // when turning on, reset this to 0 and count up while pump stays on

	Pump_C()
	{ // constructor
		pinMode(HEAT_PUMP_PIN, OUTPUT);
	}

	// if temp is cold return true, else if slow/fast temp are warm, return
	bool isCold()
	{
		return (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE || floorEmaAvgSlow < FLOOR_WARMUP_TEMPERATURE);
	}

  double getTempDiffLimited(float old, float recent, double limit) 
  {
    double diff = double(old - recent);
    // limit the diff to +- the limit
    return diff >= 0
      ? min(diff, limit)
      : max(diff, (-1 * limit));
  }

  void incrementAntiFreezeTimers(bool isPumpOn) { // run every second
    lastOnCycleStartedSecondsAgo++;
    if (isPumpOn) lastOnCycleDuration++; // run every second that the pump is on (or starting)
  }
  void resetAntiFreezeTimers() { // run once when pump turns on
    lastOnCycleStartedSecondsAgo = 0;
    lastOnCycleDuration = 0;
  }

	bool isAboveAdjustedSetPoint()
	{
		// as floor offset increases pump will cut off sooner or start later
		// as floor offset decreases pump will turn on sooner or cut off later
		// return weightedAirTemp >= setPoint - floorOffset;

		// how about: take the air temp medium ema compared to current ema
		// medium ema - current === difference,
    // (or slow ema - medium ema === difference)
    //    this way a sudden drop in temp (from opening the door) will
    //      not trigger the pump quickly
		// if difference is positive this means we have extra capacity of heat
		// if difference is negative this means we don't have extra capacity of heat

		// save difference to global var
    // difference = getTempDiffLimited(air[0].currentEMA[1], air[0].currentEMA[0], 2);
    difference = getTempDiffLimited(air[0].currentEMA[2], air[0].currentEMA[1], 2);
		return weightedAirTemp >= setPoint - difference;
	}

	uint8_t limitPwm(int16_t target)
	{
		return target >= 255 ? 255 : target;
	}

	uint8_t limitedBasePwm()
	{
		return limitPwm((coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM);
	}

	String getStatusString()
	{
		return (state == 0)		? "OFF"
					 : (state == 3) ? "---"
													: String(pwm);
	}

	void resetDuration()
	{
		cycleDuration = 0;
	}

	// to start the pump, go to `starting phase`, reset duration, check cold floor, and set pwm.
	void start()
	{
		state = 2; // motor starting phase

		// re-set cycle timer
		resetDuration();
		timeRemaining = ON_CYCLE_MINIMUM_SECONDS;

    // reset timers for ANTIFREEZE cycle
    resetAntiFreezeTimers();

		// reset "pwm pulse" counter when turning on
		pulsePwmCounter = 0;
		// smooth pwm transition, subtract this from pwm, and then --
		startingPhasePwmStepAdjust = STARTING_PHASE_INITIAL_STEP;

		// check if floor is cold
		coldFloor = isCold();
		// add starting PWM boost, to decrease as it goes
		uint8_t basePwm = limitPwm(ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_BOOST);
		// calc cold floor boosted PWM
		uint8_t coldPwm = limitPwm(basePwm + COLD_FLOOR_PWM_BOOST);
		// add on COLD_FLOOR_PWM_BOOST if coldFloor
		pwm = (coldFloor && coldPwm > basePwm) ? coldPwm : basePwm;
	}

	void stop(bool skipTimer = false)
	{
		// check for cold floor so it stays up to date
		coldFloor = isCold();
		state = 0;
		resetDuration();
		pwm = 0;
		timeRemaining = (skipTimer) ? 0 : OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)
	}

	void checkAfter(uint16_t delaySeconds = DELAY_SECONDS)
	{
		state = 3; // "delay" state
		timeRemaining = delaySeconds;
	}

	void runOn()
	{ // continue run phase
		state = 1;
		// turn off coldFloor if floor stays warm for a bit
		coldFloor = isCold();
		pwm = limitedBasePwm();
	}

	// during startup phase
	void stepDownPwm()
	{
		// base pwm
		uint8_t basePwm = limitedBasePwm();

		// current PWM is boosted for startup. Calc after stepping down one time
		uint8_t steppedDownPwm = pwm - startingPhasePwmStepAdjust;
		// if pwm stepped down is more than the base pwm, we use it; else we use basePwm;
		pwm = (steppedDownPwm > basePwm) ? steppedDownPwm : basePwm;

		startingPhasePwmStepAdjust -= 1;
	}

	void pulsePwm()
	{ // occasional high power pulse to prevent motor hangups
		pulsePwmCounter++;
		bool isTimeForPwmPulse = pulsePwmCounter >= PULSE_PWM_SECONDS_INTERVAL;
		if (isTimeForPwmPulse)
		{
			pulsePwmCounter = 0;

			uint8_t pulsePwm = limitPwm(ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT);
			pwm = limitPwm(pulsePwm >= pwm ? pulsePwm : pwm);
		}
	}

	// called every second
	void update()
	{
		cycleDuration++; // this cycle cycleDuration
		DEBUG_highsLowsFloor();

		// time spent on
		bool isPumpOn = pwm > 0;
		if (isPumpOn)
			accumOn++;
    // to track when we can skip the ANTIFREEZE cycle
    incrementAntiFreezeTimers(isPumpOn);

		// time spent above setpoint
		// bool isAboveTargetTemperature = weightedAirTemp >= setPoint;
		if (isAboveAdjustedSetPoint())
			accumAbove++;

		// check for cycle change
		bool pwmHasChanged = checkCycle(isPumpOn);

		// adjust pump if needed
		if (pwmHasChanged)
			analogWrite(HEAT_PUMP_PIN, pwm);
	}

  // ===============================================
  // Switching to different cycles:
  // ===============================================

  void offWithTimer() {
    // special cases in extreme change
    bool isTempBelowMaximumOffset = weightedAirTemp <= setPoint - EMERGENCY_ON_TRIGGER_OFFSET;

    if (isTempBelowMaximumOffset)
      start();
  }

  void onWithTimer() {
    runOn();
    pulsePwm();
  }

  void pumpStart() {
    // bool isStillStartingUp = timeRemaining > END_OF_STARTUP_TIMER;
    bool isStillStartingUp = startingPhasePwmStepAdjust > 0;
    if (isStillStartingUp)
      stepDownPwm();
    else
      runOn();
  }

  void offContinued(bool isPumpOn, bool _isAboveAdjustedSetPoint) {
    if (!_isAboveAdjustedSetPoint)
      if (isPumpOn)
        runOn();
      else
        start(); // most common start trigger
    else if (isPumpOn)
      stop(); // coming from delay, in case we need to stop
  }

  void onContinued(bool _isAboveAdjustedSetPoint) {
    if (_isAboveAdjustedSetPoint)
      stop(); // most common stop trigger
    else
    {
      runOn();
      pulsePwm();
    }
  }

  void delayTimerEnd() {
    state = 0;
  }

	bool checkCycle(bool isPumpOn)
	{
		uint8_t lastPwm = pwm;
		bool isDuringATimedCycle = timeRemaining > 0;

		if (isDuringATimedCycle)
		{
			timeRemaining--;

			if (state == 0) offWithTimer();
			else if (state == 1) onWithTimer();
			else if (state == 2) pumpStart();
		}
		else
		{
			// NO timer restriction (extended phase)
      bool _isAboveAdjustedSetPoint = isAboveAdjustedSetPoint();

			if (state == 0) offContinued(isPumpOn, _isAboveAdjustedSetPoint);
			else if (state == 1) onContinued(_isAboveAdjustedSetPoint);
			else if (state == 3) delayTimerEnd();
		}

		// return true if changed
		return pwm != lastPwm;
	}

} pump = Pump_C();
