// Floor Sensor approximate readings examples
// ~489/ 518 cold floor
// 590/632 - 32m in to on-cycle
// 622/649 floor (warm but not hot)

const uint8_t ON_PHASE_BASE_PWM = 255;
const uint8_t COLD_FLOOR_PWM_BOOST = 75;

// PWM boost during motor start
const uint8_t STARTING_PHASE_PWM_BOOST = 56; // ({sum(1-10)} == 55) + {1*remaining seconds}
const uint8_t STARTING_PHASE_SECONDS = 11;
const uint8_t STARTING_PHASE_STEP = 10; // start amt for startingPhaseStepAdjust
uint8_t startingPhaseStepAdjust = 1;

const uint16_t ON_CYCLE_MINIMUM_SECONDS = 600;	 // 10m
const uint16_t OFF_CYCLE_MINIMUM_SECONDS = 1800; // 30m
const uint32_t END_OF_STARTUP_TIMER = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;

// PWM boost every so often, in case pump gets hung up:
const uint16_t PULSE_PWM_SECONDS_INTERVAL = 3600; // seconds (every hour)
const uint8_t PULSE_PWM_AMOUNT = 22;							// boost
uint16_t pulsePwmCounter = 0;											// counts the seconds since last boost

static const uint8_t DELAY_SECONDS = 3;				 // default cycleDuration for delay-start
const uint8_t EMERGENCY_ON_TRIGGER_OFFSET = 5; // if in 30-min off cycle, if temp drops by this amount off target, start the pump

uint32_t timeRemaining = 0; // Counter for minimum cycle times

class Pump_C
{
public:
	uint32_t accumOn = 0;		 // time spent On
	uint32_t accumAbove = 0; // total time spent above setpoint

	// using 16_t so it can overflow past 255 and be corrected to max 255
	uint8_t pwm = 0; // holds motor PWM

	uint8_t state = 0; // holds motor state
	// uint8_t lastState = 0;
	uint32_t cycleDuration = 0; // for this cycle
	// uint32_t lastDuration = 0;

	Pump_C()
	{ // constructor
		pinMode(HEAT_PUMP_PIN, OUTPUT);
	}

	// if temp is cold return true, else if slow/fast temp are warm, return
	bool isCold()
	{
		// return (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE) || !((floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) &&
		// 																										(floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE));

		return (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
		// if (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE) return true;

		// if ((floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) && (floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE)) return false;
	}

	bool isWarm()
	{
		return ((floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) && (floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE));
	}

	void setPwm(uint16_t target)
	{
		pwm = target >= 255 ? 255 : (target <= 0) ? 0
																							: target;
	}

	String getStatus()
	{
		return (state == 0) ? "OFF" : (state == 3) ? "---"
																							 : String(pwm);
	}

	void resetDuration()
	{
		// lastDuration = cycleDuration;
		cycleDuration = 0;
	}

	//
	void start()
	{
		// bool isPumpOn = pwm > 0;
		// if (isPumpOn)
		// {
		// 	runOn();
		// 	return;
		// }

		state = 2; // motor starting phase

		resetDuration();
		timeRemaining = ON_CYCLE_MINIMUM_SECONDS;
		pulsePwmCounter = 0;													 // reset "pwm pulse" counter when turning on
		startingPhaseStepAdjust = STARTING_PHASE_STEP; // smooth pwm transition, subtract this from pwm, and then --

		// check if floor is cold
		coldFloor = isCold();
		// calculate pwm
		uint16_t nextPwm = ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_BOOST; // 181
		// add on COLD_FLOOR_PWM_BOOST if coldFloor
		nextPwm += (coldFloor && (COLD_FLOOR_PWM_BOOST + nextPwm) > nextPwm) ? COLD_FLOOR_PWM_BOOST : 0;
		setPwm(nextPwm);
	}

	void stop(bool skipTimer = false)
	{
		// check for cold floor so it stays up to date
		coldFloor = isCold();
		state = 0;
		resetDuration();
		setPwm(0);
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
		coldFloor = !isWarm();
		// if (
		// 	(floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) &&
		// 	(floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE))
		// {
		// 	coldFloor = false;
		// }
		uint16_t nextPwm = (coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM;
		setPwm(nextPwm);
	}

	void stepDownPwm()
	{ // during startup phase

		uint16_t basePwm = (coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM;
		uint16_t nextPwm = ((pwm - startingPhaseStepAdjust) >= (basePwm)) ? pwm - startingPhaseStepAdjust : basePwm;
		setPwm(nextPwm);

		// if (coldFloor)
		// {
		// 	uint16_t basePwm = (coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM;
		// 	uint16_t nextPwm = ((pwm - startingPhaseStepAdjust) >= (basePwm)) ? pwm - startingPhaseStepAdjust : basePwm;
		// 	// if ((pwm - startingPhaseStepAdjust) >= (ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST))
		// 	// 	pwm -= startingPhaseStepAdjust;
		// 	// else
		// 	// 	pwm = ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST;
		// }
		// else
		// {
		// 	uint16_t basePwm = (coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM;
		// 	uint16_t nextPwm = ((pwm - startingPhaseStepAdjust) >= basePwm) ? pwm - startingPhaseStepAdjust : basePwm;
		// 	// if ((pwm - startingPhaseStepAdjust) >= ON_PHASE_BASE_PWM)
		// 	// 	pwm -= startingPhaseStepAdjust;
		// 	// else
		// 	// 	pwm = ON_PHASE_BASE_PWM;
		// }

		if (startingPhaseStepAdjust - 1 >= 1)
			startingPhaseStepAdjust -= 1;
	}

	void pulsePwm()
	{ // occasional high power pulse to prevent motor hangups
		pulsePwmCounter++;
		bool isTimeForPwmPulse = pulsePwmCounter >= PULSE_PWM_SECONDS_INTERVAL;
		if (isTimeForPwmPulse)
		{
			pulsePwmCounter = 0;
			// uint16_t pulsePwm = ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT;
			// bool isPulsePwmGreater = pulsePwm >= pwm;
			// pwm = (isPulsePwmGreater) ? pulsePwm : pwm;

			uint16_t pulsePwm = ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT;
			uint16_t nextPwm = pulsePwm >= pwm ? pulsePwm : pwm;
			setPwm(nextPwm);
		}
	}

	void update()
	{									 // called every second
		cycleDuration++; // this cycle cycleDuration
		DEBUG_highsLowsFloor();

		// time spent on
		bool isPumpOn = pwm > 0;
		if (isPumpOn)
			accumOn++;

		// time spent above setpoint
		bool isAboveTargetTemperature = Input >= Setpoint;
		if (isAboveTargetTemperature)
			accumAbove++;

		bool isDuringACycle = timeRemaining > 0;
		if (isDuringACycle)
		{
			// timer is running, should stay in these states
			timeRemaining--;

			switch (state)
			{
				case (0): // offWithTimer();
					// special cases in extreme change
					bool shouldNeedMaxOutput = Output == outputMax;
					bool isTempBelowMaximumOffset = Input <= Setpoint - EMERGENCY_ON_TRIGGER_OFFSET;

					if (isTempBelowMaximumOffset || shouldNeedMaxOutput)
						start();
					break;

				case (1): // onWithTimer();
					runOn();
					pulsePwm(); // occasional higher PWM pulse
					break;

				case (2): // startup();
					bool isStillStartingUp = timeRemaining > END_OF_STARTUP_TIMER;
					if (isStillStartingUp)
						stepDownPwm();
					else
						runOn();
					break;

				default:
					break;
			}
		}
		else
		{
			// NO timer restriction (extended phase)
			switch (state)
			{
				case (0): // offContinued() && check after delayedStart
					bool shouldPumpBeOn = Output > MIDPOINT;
					bool isPumpOn = pwm > 0;

					if (shouldPumpBeOn)
						if (isPumpOn) runOn();
						else start(); // most common start trigger
					else if (isPumpOn)
						stop(); // coming from delay, in case we need to stop
					break;

				case (1): // onContinued();
					bool shouldPumpBeOff = Output <= MIDPOINT;
					if (shouldPumpBeOff)
						stop(); // most common stop trigger
					else
					{
						runOn();
						pulsePwm();
					}
					break;

				case (3):		 // delayTimerEnd();
					state = 0; // it'll check next update
					break;
				
				default:
					break;
			}
		}

		analogWrite(HEAT_PUMP_PIN, pwm);
	}

} pump = Pump_C();
