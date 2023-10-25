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

  /* 
  0 === off, 
  1 === on, 
  2 === starting, 
  3 === delayedStart, 
  4 === Continuing Heartbeat cycle
  5 === starting Heartbeat cycle
  */
	uint8_t state = 0;					
	uint32_t cycleDuration = 0; // for this cycle

  // for Heartbeat
  uint32_t lastOffCycleDuration = 0;
  bool isHeartbeatOn = false;


  /*
  Constructor function
  set up pump pin
  */
	Pump_C()
	{
		pinMode(HEAT_PUMP_PIN, OUTPUT);
	}

  /*
  Check if floor is cold
  @return {bool}
  */
	bool isFloorCold()
	{
		return (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE || floorEmaAvgSlow < FLOOR_WARMUP_TEMPERATURE);
	}

  /*
  Compares EMAs for given air sensor, and limits the result
  @param {uint8_t} airSensorIndex - index of air sensor
  @param {double} limit - temperature limit
  */
  double updateAirLimitedTempDifference(uint8_t airSensorIndex = 0, double limit = 2) 
  {
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

  @return {bool}
  */
	bool isAboveAdjustedSetPoint()
	{
    updateAirLimitedTempDifference(0);
		return weightedAirTemp >= setPoint - difference;
	}


  /*
  Updates onOffRatioEma based on last on/off cycle
  Runs after the on cycle completes 
  - cycleDuration === duration of the recent ON cycle

  @param {uint32_t} totalTime - on+off total time
  */
  void calculateOnOffRatioEMA(uint32_t totalTime) {
    float thisRatio = (totalTime == 0 && cycleDuration == 0) 
        ? 0 
        : cycleDuration / totalTime;
    heartbeatOnOffRatioEMA = (thisRatio * (2. / 1 + HEARTBEAT_ON_RATIO_EMA_PERIOD)) +
        heartbeatOnOffRatioEMA * (1 - (2. / 1 + HEARTBEAT_ON_RATIO_EMA_PERIOD));
    DEBUG_heartbeat();
  }

  /* 
  only runs when pump is OFF
  */
  void updateHeartbeat(bool initialize = false) {
      if (initialize) {
        heartbeatTimer = 0;
        isHeartbeatOn = false;
        // could have this update every run, or only on init
        heartbeatCalculatedOffTime = (HEARTBEAT_CYCLE_MINIMUM_OFF_DURATION 
          / (heartbeatOnOffRatioEMA - HEARTBEAT_ON_CYCLE_DURATION));
      }

      if (!isHeartbeatOn) {
        // turn on heartbeat after enough time has passed
        if (heartbeatTimer >= heartbeatCalculatedOffTime) {
          isHeartbeatOn = true;
        }

      } else {
        // turn off heartbeat after some more time
        if (heartbeatTimer >= heartbeatCalculatedOffTime + HEARTBEAT_ON_CYCLE_DURATION) {
          heartbeatTimer = 0; // reset timer
          isHeartbeatOn = false;
        }
      }

      heartbeatTimer++;
  }

  /* 
  Returns PWM, limited to 255 (the max PWM)
  */
	uint8_t limitPwm(int16_t target) {
		return target >= 255 ? 255 : target;
	}

  /* 
  Returns PWM, depending on cold floor and heartbeat cycle
  */
	uint8_t limitedBasePwm(bool isHeartbeatCycle = false) {
    uint8_t base = isHeartbeatCycle ? HEARTBEAT_CYCLE_PWM : ON_PHASE_BASE_PWM;
		return limitPwm(base + (coldFloor) ? COLD_FLOOR_PWM_BOOST : 0);
	}

  /* 
  Converts state (or PWM) into a string to display
  @return {String}
  */
	String getStatusString() {
		return (state == 0)		? "OFF"
					 : (state == 3) ? "---"
													: String(pwm);
	}

  /*
  Resets cycle duration
  */
	void resetCycleDuration()
	{
		cycleDuration = 0;
	}

  /**
  * Starting the pump from off
  - save and re-set the cycle duration/timer
  - set state to the appropriate "pump start" phase
  - re-set pulsePwmCounter, startingPhasePwmStepAdjust
  - Set up appropriate PWM for the pump

  @param {bool} isHeartbeatCycle - is this going into a heartbeat cycle
  **/
	void start(bool isHeartbeatCycle = false)
	{
    // save off cycle time for our AntiFreeze function
    lastOffCycleDuration = cycleDuration;
    resetCycleDuration();
    timeRemaining = ON_CYCLE_MINIMUM_SECONDS;

    uint8_t basePwm = ON_PHASE_BASE_PWM;

    state = 2;

    // smooth pwm transition, subtract this from pwm, and then --
    startingPhasePwmStepAdjust = STARTING_PHASE_INITIAL_STEP;

    // // if heartbeat cycle, adjust some variables
    // if (isHeartbeatCycle) {
    //   state = 5; // Antifreeze starting phase??
    //
    //   timeRemaining = HEARTBEAT_ON_CYCLE_DURATION;
    //
    //   // smooth pwm transition, subtract this from pwm, and then --
    //   startingPhasePwmStepAdjust = startingPhasePwmStepAdjust - 1;
    //
    //   basePwm = HEARTBEAT_CYCLE_PWM;
    // }

    // reset "pwm pulse" counter when turning on
    pulsePwmCounter = 0;

    // set up next PWM
    // check if floor is cold
    coldFloor = isFloorCold();
    // add starting PWM boost, to decrease as it goes
    basePwm = limitPwm(basePwm + STARTING_PHASE_PWM_BOOST);
    // calc cold floor boosted PWM
    uint8_t coldPwm = limitPwm(basePwm + COLD_FLOOR_PWM_BOOST);
    // add on COLD_FLOOR_PWM_BOOST if coldFloor
    pwm = (coldFloor) ? coldPwm : basePwm;
	}

  /**
  * Stop the pump
  On initialization/startup, we don't want to run the timer, because
  that could prevent our heat from  turning on for 30m
  @param {bool} isInitialization - is this initialization?
  **/
	void stop(bool isInitialization = false)
	{
    // for our AntiFreeze function
    uint32_t totalOffOnDuration = lastOffCycleDuration + cycleDuration;
    calculateOnOffRatioEMA(totalOffOnDuration);

		// check for cold floor so it stays up to date
		coldFloor = isFloorCold();
		state = 0;
		resetCycleDuration();
		pwm = 0;
		timeRemaining = (isInitialization) ? 0 : OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)
	}

  /**
  * Initializes the "delayed start" mode,
  e.g. will force a check of the temp/state after delaySeconds
  @param {uint16_t} delaySeconds - seconds after which to check the state
  */
	void checkAfter(uint16_t delaySeconds = DELAY_SECONDS)
	{
		state = 3; // "delay" state
		timeRemaining = delaySeconds;
	}

  /**
  * When continuing pump ON phase,
  either going into normal ON phase or going to Antifreeze On phase
  @param {bool} isHeartbeatCycle - is this Antifreeze cycle
  */
	void runOn(bool isHeartbeatCycle = false)
	{ // continue run phase
    if (isHeartbeatCycle) {
      state = 4;
    } else {
      state = 1;
    }
		// turn off coldFloor if floor stays warm for a bit
		coldFloor = isFloorCold();
		pwm = limitedBasePwm(isHeartbeatCycle);
	}

  /**
  * During pump startup phase, step down PWM each cycle
  @param {bool} isHeartbeatCycle - is this Antifreeze cycle
  */
	void stepDownPwm(bool isHeartbeatCycle = false)
	{
		// base pwm
		uint8_t basePwm = limitedBasePwm(isHeartbeatCycle);

		// current PWM is boosted for startup. Calc after stepping down one time
		uint8_t steppedDownPwm = pwm - startingPhasePwmStepAdjust;
		// if pwm stepped down is more than the base pwm, we use it; else we use basePwm;
		pwm = (steppedDownPwm > basePwm) ? steppedDownPwm : basePwm;

		startingPhasePwmStepAdjust -= 1;
	}

  /**
  * called every second while pump is ON
  */
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

  /**
  * called every second
  */
	void update()
	{
		cycleDuration++; // this cycle cycleDuration
		DEBUG_highsLowsFloor();

		// time spent on
		bool isPumpOn = pwm > 0;
		if (isPumpOn)
			accumOn++;
    // to track when we can skip the ANTIFREEZE cycle

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

  /*
  When starting the pump
  */
  void pumpStart(bool isHeartbeatCycle = false) {
    // if starting up, step down
    if (startingPhasePwmStepAdjust > 0)
      stepDownPwm(isHeartbeatCycle);
    else
      runOn(isHeartbeatCycle);
  }

  
  /*
  Continuing on (with timer)
  */
  void onWithTimer() {
    runOn();
    pulsePwm();
  }

  /*
  Continuing on (extended)
  */
  void onExtended(bool _isAboveAdjustedSetPoint, bool isHeartbeatCycle = false) {
      if (_isAboveAdjustedSetPoint && !isHeartbeatCycle)
        stop(); // most common stop trigger
      else
      {
        runOn(isHeartbeatCycle);
        pulsePwm();
      }
  }

  /*
  Turning off pump Off (with timer)
  */
  void offWithTimer() {
    // update heartbeat timer
    updateHeartbeat(true);

    // special cases in extreme change
    if (weightedAirTemp <= setPoint - EMERGENCY_ON_TRIGGER_OFFSET)
      start();
  }

  /*
  Continuing pump off pump (extended)
  */
  void offExtended(bool isPumpOn, bool _isAboveAdjustedSetPoint) {
    if (_isAboveAdjustedSetPoint) {
      if (isPumpOn)
        stop(); // coming from delay, in case we need to stop
    } else {
      updateHeartbeat();
      if (isPumpOn)
        runOn();
      else
        start(); // most common start trigger
    }
  }

  /*
  When coming from delayed check
  */
  void delayTimerEnd() {
    state = 0;
  }

  /**
  * change cycles if needed, and returns true if PWM changed
  */
	bool checkCycle(bool isPumpOn)
	{
		uint8_t lastPwm = pwm;

    // if during a timed cycle
		if (timeRemaining > 0) {
			timeRemaining--;

        switch(state) {
          case(0): offWithTimer();
          case(1): onWithTimer();
          case(2): pumpStart();
          case(5): pumpStart(true);
        }

		} else {
			// NO timer restriction, we can change if needed
      bool _isAboveAdjustedSetPoint = isAboveAdjustedSetPoint();
 
        switch(state){
          case(0): offExtended(isPumpOn, _isAboveAdjustedSetPoint);
          case(1): onExtended(_isAboveAdjustedSetPoint);
          case(3): delayTimerEnd();
          case(4): onExtended(_isAboveAdjustedSetPoint, true);

        }
		}

		// return true if changed
		return pwm != lastPwm;
	}

} pump = Pump_C();
