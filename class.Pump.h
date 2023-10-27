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

// Occasional PWM boost (to prevent motor stalling)
const uint16_t PULSE_PWM_SECONDS_INTERVAL = 30 * 60; // seconds (every 30m)
uint16_t pulsePwmCounter = 0;											// counts the seconds since last boost

// Delayed start phase: seconds to delay before checking/starting
static const uint8_t DELAY_SECONDS = 3;				 // seconds after setpoint change it checks which pump state to be in

// holds setpoint offset, to maintain constant feel
double difference = 0;
// if temp drops lower than this from the setpoint, start the pump regardless of timers
const uint8_t EMERGENCY_ON_TRIGGER_OFFSET = 5; // degrees from setpoint

/* ==========================================================================
 * Heartbeat functionality vars
 * ========================================================================== */
const uint8_t HEARTBEAT_CYCLE_PWM = 145; // slightly faster than normal
const uint16_t HEARTBEAT_OFF_CYCLE_MIN_DURATION = 30 * 60; //30m
const uint32_t HEARTBEAT_ON_CYCLE_DURATION = 1 * 60; // 1m

const uint32_t HEARTBEAT_RATIO_EMA_PERIOD = 1 * 60 * 60 * 24; // 1 day
const double HEARTBEAT_RATIO_THRESHOLD = 0.1;

// initially set, and then calculated after each off cycle
uint32_t heartbeatCalculatedOffTime = HEARTBEAT_OFF_CYCLE_MIN_DURATION - HEARTBEAT_ON_CYCLE_DURATION;
// on off ratio ranges 0-1
float heartbeatOnOffRatioEMA = 0;
// to determine how far in the heartbeat cycle we are (to turn on and off the pump)
uint32_t heartbeatTimer = 0;
// for Heartbeat
uint32_t lastOffCycleDuration = 0;
bool isHeartbeatOn = false;

bool isHeartbeatRatioAboveThreshold = false;

/* ==========================================================================
 * Pump class
 * ========================================================================== */
class Pump_C {
public:
	uint32_t accumOn = 0;		 // time spent On
	uint32_t accumAbove = 0; // total time spent above setpoint
	uint8_t pwm = 0; // holds motor PWM

  bool _isAboveAdjustedSetPoint = false;
  bool isPumpOn = pwm > 0;

  /* 
  0 === off, 
  1 === on, 
  2 === starting, 
  3 === delayedStart, 
  // BELOW: should IGNORE temp checks when running
  4 === Continuing Heartbeat cycle
  5 === starting Heartbeat cycle
  */
	uint8_t state = 0;					
  uint8_t prevState = 0;
	uint32_t cycleDuration = 0; // for this cycle


  /*
  * Constructor function
  * set up pump pin
  */
	Pump_C() {
		pinMode(HEAT_PUMP_PIN, OUTPUT);
	}

  /*
  * save prev state and new state
  * @param {uint8_t} _state - new state
  */
  void setState(uint8_t _state) {
    prevState = state;
    state = _state;
  }


  /*
  Updates onOffRatioEma based on last on/off cycle
  Runs after the on cycle completes (e.g. when stopping the pump)
  - cycleDuration === duration of the recent ON cycle

  @param {uint32_t} totalOnOffCycleDuration - on+off total time
  */
  void updateOnOffRatioEMA(uint32_t totalOnOffCycleDuration) {
    float thisRatio = (totalOnOffCycleDuration == 0 || cycleDuration == 0) 
        ? 0 
        : cycleDuration / totalOnOffCycleDuration;

    heartbeatOnOffRatioEMA = (thisRatio * (2. / 1 + HEARTBEAT_RATIO_EMA_PERIOD)) +
        heartbeatOnOffRatioEMA * (1 - (2. / 1 + HEARTBEAT_RATIO_EMA_PERIOD));

    isHeartbeatRatioAboveThreshold = heartbeatOnOffRatioEMA >= HEARTBEAT_RATIO_THRESHOLD;
    DEBUG_heartbeat();
  }

  /* 
  * only runs while pump is OFF (and when pump on/off ratio > threshold
  */
  void updateHeartbeat(bool initialize = false) {
    if (!isHeartbeatRatioAboveThreshold) {
      heartbeatTimer = 0;
      isHeartbeatOn = false;
      return;
    }

    if (initialize ) {
      heartbeatTimer = 0;
      isHeartbeatOn = false;
      // could have this update every run, or only on init
      heartbeatCalculatedOffTime = (HEARTBEAT_OFF_CYCLE_MIN_DURATION 
        / (heartbeatOnOffRatioEMA - HEARTBEAT_ON_CYCLE_DURATION));
    }
    // heartbeatCalculatedOffTime = 
    //   (HEARTBEAT_OFF_CYCLE_MIN_DURATION / heartbeatOnOffRatioEMA) -
    //   HEARTBEAT_ON_CYCLE_DURATION;

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

  
  // ===========================================================================
  // Helper functions
  // ===========================================================================

  /* 
  Returns PWM, limited to 255 (the max PWM)
  */
	uint8_t limitPwm(int16_t value) {
		return value >= 255 ? 255 : value;
	}

  /* 
  Returns PWM, depending on cold floor and heartbeat cycle
  */
	uint8_t getLimitedBasePwm() {
    uint8_t base = (state == 5 || state == 4) 
      ? HEARTBEAT_CYCLE_PWM 
      : ON_PHASE_BASE_PWM;
    // uint8_t base = ON_PHASE_BASE_PWM;
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
	void resetCycleDuration() {
		cycleDuration = 0;
	}

  // ===========================================================================
  // Pump control functions (starting, stopping, etc)
  // ===========================================================================

  /**
  * Starting the pump from off
  - save and re-set the cycle duration/timer
  - set state to the appropriate "pump start" phase
  - re-set pulsePwmCounter, startingPhasePwmStepAdjust
  - Set up appropriate PWM for the pump

  @param {bool} isHeartbeatCycle - is this going into a heartbeat cycle
  **/
	void start() {
    // save off cycle time for our AntiFreeze function
    lastOffCycleDuration = cycleDuration;
    resetCycleDuration();
    timeRemaining = ON_CYCLE_MINIMUM_SECONDS;

    uint8_t basePwm = ON_PHASE_BASE_PWM;

    uint8_t newState = 2

    // smooth pwm transition, subtract this from pwm, and then --
    startingPhasePwmStepAdjust = STARTING_PHASE_INITIAL_STEP;

    // // if heartbeat cycle, adjust some variables
    if (isHeartbeatOn) {
      newState = 5; // Heartbeat starting phase
      timeRemaining = HEARTBEAT_ON_CYCLE_DURATION;
      basePwm = HEARTBEAT_CYCLE_PWM;
    }

    setState(newState);

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
	void stop(uint32_t nextDuration = 0, bool isInitialization = false)	{
    // for our Heartbeat function
    updateOnOffRatioEMA(lastOffCycleDuration + cycleDuration);

		// check for cold floor so it stays up to date
		coldFloor = isFloorCold();
    setState(0);
		resetCycleDuration();
		pwm = 0;
		timeRemaining = (isInitialization) ? 0 : OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)
	}

  /**
  * Initializes the "delayed start" mode,
  e.g. will force a check of the temp/state after delaySeconds
  @param {uint16_t} delaySeconds - seconds after which to check the state
  */
	void checkAfter(uint16_t delaySeconds = DELAY_SECONDS) {
		setState(3); // "delay" state
		timeRemaining = delaySeconds;
	}

  /**
  * When continuing pump ON phase,
  either going into normal ON phase or going to Antifreeze On phase
  - Switches state from "starting" phase to "on" phase

  @param {bool} isHeartbeatCycle - is this Antifreeze cycle
  */
	void runOn() {
    if (
      state == 5 || 
      state == 2
    ) {
      setState(state - 1);
    }

		// turn off coldFloor if floor stays warm for a bit
		coldFloor = isFloorCold();
		pwm = getLimitedBasePwm();
	}

  /**
  * During pump startup phase, step down PWM each cycle
  @param {bool} isHeartbeatCycle - is this Antifreeze cycle
  */
	void stepDownPwm() {
		// base pwm
		uint8_t basePwm = getLimitedBasePwm();

		// current PWM is boosted for startup. Calc after stepping down one time
		uint8_t steppedDownPwm = pwm - startingPhasePwmStepAdjust;
		// if pwm stepped down is more than the base pwm, we use it; else we use basePwm;
		pwm = (steppedDownPwm > basePwm) ? steppedDownPwm : basePwm;

		startingPhasePwmStepAdjust -= 1;
	}

  /**
  * called every second while pump is ON
  */
	void pulsePwm() { // occasional high power pulse to prevent motor hangups
		pulsePwmCounter++;
		bool isTimeForPwmPulse = pulsePwmCounter >= PULSE_PWM_SECONDS_INTERVAL;

		if (isTimeForPwmPulse) {
			pulsePwmCounter = 0;

			uint8_t pulsePwm = limitPwm(ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT);
			pwm = limitPwm(pulsePwm >= pwm ? pulsePwm : pwm);
		}
	}


  // ===========================================================================
  // Switching to different cycles:
  // ===========================================================================

  /*
  state === 2 || state === 5 (start phase)
  Step down while needed, else we runOn with the current PWM
  */
  void whilePumpStarting() {
    if (startingPhasePwmStepAdjust > 0) {
      stepDownPwm();
    } else {
      runOn();
    }
  }

  
  /*
  state === 1 || state === 4 (on/run phase)
  While pump is on (with timer)
  */
  void whilePumpOn() {
    runOn();
    pulsePwm();
  }

  /*
  state === 1 || state === 4 (on/run phase)
  While pump is continuing on (extended, no timer)
  Stop if needed, else we do the same as whilePumpOn
  */
  void whilePumpOnExtended() {
    // skip stopping if we're in heartbeat cycle
    if (
      // (state != 4 && state != 5) && 
      _isAboveAdjustedSetPoint
    ) {
      stop(); // most common stop trigger
    } else {
      whilePumpOn();
    }
  }

  /*
  state === 0 (off phase)
  While pump is off (with a timer)
  Emergency case to re-start if temp is under a threshold
  */
  void whilePumpOff() {
    // update heartbeat timer while off
    updateHeartbeat();

    // special cases in extreme change
    if (weightedAirTemp <= setPoint - EMERGENCY_ON_TRIGGER_OFFSET) {
      start();
    }
  }

  /*
  state === 0 (off phase)
  While pump is past the timer but still off (extended)
  Stops pump if warm enough. Else, we need to restart the pump
  @param {bool} isPumpOn - is pump on?
  @param {bool} _isAboveAdjustedSetPoint - is warm enough?
  */
  void whilePumpOffExtended() {
    if (_isAboveAdjustedSetPoint) {
      updateHeartbeat();

      if (isPumpOn) {
        stop(); // coming from delay, in case we need to stop
      }
    
      if (isPumpOn && !isHeartBeatOn) {
        stop(); // coming from delay, in case we need to stop
      } else if (isHeartBeatOn && !isPumpOn) {
        start(); // Start trigger for heartbeat
      }
    } else {
      // not warm enough, so we start the pump

      if (isPumpOn) {
        runOn();
      } else {
        start(); // most common start trigger
      }
    }
  }

  /*
  state === 3
  When coming from delayed check
  */
  void whileEndingDelayStartTimer() {
    setState(0);
  }

  /**
  * change cycles if needed, and returns true if PWM changed
  */
	bool checkCycle() {
		uint8_t lastPwm = pwm;

    // if during a timed cycle
		if (timeRemaining > 0) {
			timeRemaining--;

      switch(state) {
        case(0): whilePumpOff(); break;

        case(1): whilePumpOn(); break;
        case(4): whilePumpOn(); break;

        case(2): whilePumpStarting(); break;
        case(5): whilePumpStarting(); break;
      }

		} else {
			// NO timer restriction, we can change if needed
      switch(state) {
        case(0): whilePumpOffExtended(); break;

        case(1): whilePumpOnExtended(); break;
        case(4): whilePumpOnExtended(); break;

        case(3): whileEndingDelayStartTimer(); break;
      }
		}

		// return true if changed
		return pwm != lastPwm;
	}

  /**
  * called every second by the main loop,
  * update accum times, check if cycle needs to change, then update PWM if changed
  */
	void update() {
		cycleDuration++; // this cycle cycleDuration
		DEBUG_highsLowsFloor(); // log the highs/lows and floor temp

		// time spent on
		isPumpOn = pwm > 0;
		if (isPumpOn) {
      accumOn++;
    }

    // update setpoint
    _isAboveAdjustedSetPoint = isAboveAdjustedSetPoint();

		// time spent above setpoint
		// bool isAboveTargetTemperature = weightedAirTemp >= setPoint;
		if (_isAboveAdjustedSetPoint) {
      accumAbove++;
    }

		// update state/PWM if needed
		bool pwmHasChanged = checkCycle();

		// adjust pump if needed
		if (pwmHasChanged) {
      analogWrite(HEAT_PUMP_PIN, pwm);
    }
	}

} pump = Pump_C();


