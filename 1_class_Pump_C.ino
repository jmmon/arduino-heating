// #include <Arduino.h>

// Floor Sensor approximate readings examples
// ~489/ 518 cold floor
// 590/632 - 32m in to on-cycle
// 622/649 floor (warm but not hot)

// const uint8_t ON_PHASE_BASE_PWM = 255;
const uint8_t ON_PHASE_BASE_PWM = 145;
const uint8_t COLD_FLOOR_PWM_BOOST = 255 - ON_PHASE_BASE_PWM; // want to reach max

// PWM boost during motor start
const uint8_t STARTING_PHASE_PWM_BOOST = 28; // ({sum(1-10)} == 55) + {1*remaining seconds}
const uint8_t STARTING_PHASE_INITIAL_STEP = 7; // initial drop amount during starting phase
uint8_t startingPhaseStepAdjust = 0;

const uint16_t ON_CYCLE_MINIMUM_SECONDS = 60;	 // 1m
const uint16_t OFF_CYCLE_MINIMUM_SECONDS = 300; // 5m
// const uint32_t END_OF_STARTUP_TIMER = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;

// PWM boost every so often, in case pump gets hung up:
const uint16_t PULSE_PWM_SECONDS_INTERVAL = 1800; // seconds (every hour)
const uint8_t PULSE_PWM_AMOUNT = 23;							// boost
uint16_t pulsePwmCounter = 0;											// counts the seconds since last boost

static const uint8_t DELAY_SECONDS = 3;				 // seconds after setpoint change it checks which pump state to be in
const uint8_t EMERGENCY_ON_TRIGGER_OFFSET = 5; // if in 30-min off cycle, if temp drops by this amount off target, start the pump

uint32_t timeRemaining = 0; // Counter for minimum cycle times

double difference = 0;

class Pump_C
{
public:
	uint32_t accumOn = 0;		 // time spent On
	uint32_t accumAbove = 0; // total time spent above setpoint

	uint8_t pwm = 0; // holds motor PWM

	uint8_t state = 0;					// holds motor state
	uint32_t cycleDuration = 0; // for this cycle

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

	//
	void start()
	{
		state = 2; // motor starting phase

		// re-set cycle timer
		resetDuration();
		timeRemaining = ON_CYCLE_MINIMUM_SECONDS;

		// reset "pwm pulse" counter when turning on
		pulsePwmCounter = 0;
		// smooth pwm transition, subtract this from pwm, and then --
		startingPhaseStepAdjust = STARTING_PHASE_INITIAL_STEP;

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
		uint8_t steppedDownPwm = pwm - startingPhaseStepAdjust;
		// if pwm stepped down is more than the base pwm, we use it; else we use basePwm;
		pwm = (steppedDownPwm > basePwm) ? steppedDownPwm : basePwm;

		startingPhaseStepAdjust -= 1;
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

		// time spent above setpoint
		// bool isAboveTargetTemperature = weightedAirTemp >= setPoint;
		if (isAboveAdjustedSetPoint())
			accumAbove++;

		// check for cycle change
		bool pwmHasChanged = checkCycle();

		// adjust pump if needed
		if (pwmHasChanged)
			analogWrite(HEAT_PUMP_PIN, pwm);
	}

	bool checkCycle()
	{
		uint8_t lastPwm = pwm;
		bool isDuringACycle = timeRemaining > 0;

		if (isDuringACycle)
		{
			timeRemaining--;

			if (state == 0)
			{ // offWithTimer();
				// special cases in extreme change
				bool isTempBelowMaximumOffset = weightedAirTemp <= setPoint - EMERGENCY_ON_TRIGGER_OFFSET;

				if (isTempBelowMaximumOffset)
					start();
			}

			else if (state == 1)
			{ // onWithTimer();
				runOn();
				pulsePwm(); // occasional higher PWM pulse
			}

			else if (state == 2)
			{ // startup();
				// bool isStillStartingUp = timeRemaining > END_OF_STARTUP_TIMER;
				bool isStillStartingUp = startingPhaseStepAdjust > 0;
				if (isStillStartingUp)
					stepDownPwm();
				else
					runOn();
			}
		}
		else
		{
			// NO timer restriction (extended phase)
			// if floor is warm, floorOffset increases, so target temperature decreases
			// bool shouldPumpBeOn = weightedAirTemp < adjustedsetPoint;

			// bool shouldPumpBeOn = Output > MIDPOINT;

			// 23952 : 1221
			if (state == 0)
			{ // offContinued() && check after delayedStart
				bool isPumpOn = pwm > 0;

				if (!isAboveAdjustedSetPoint())
					if (isPumpOn)
						runOn();
					else
						start(); // most common start trigger
				else if (isPumpOn)
					stop(); // coming from delay, in case we need to stop
			}

			else if (state == 1)
			{ // onContinued();
				if (isAboveAdjustedSetPoint())
					stop(); // most common stop trigger
				else
				{
					runOn();
					pulsePwm();
				}
			}

			else if (state == 3)
			{						 // delayTimerEnd();
				state = 0; // it'll check next update
			}
		}

		// return true if changed
		return pwm != lastPwm;
	}

} pump = Pump_C();
