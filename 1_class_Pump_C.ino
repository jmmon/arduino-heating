// Floor Sensor approximate readings examples
// ~489/ 518 cold floor
// 590/632 - 32m in to on-cycle
// 622/649 floor (warm but not hot)

const uint8_t ON_PHASE_BASE_PWM = 155,
							COLD_FLOOR_PWM_BOOST = 44;

// PWM boost during motor start
const uint8_t STARTING_PHASE_PWM_BOOST = 56, // ({sum(1-10)} == 55) + {1*remaining seconds}
		STARTING_PHASE_SECONDS = 11,
							STARTING_PHASE_STEP = 10; // start amt for startingPhaseStepAdjust
uint8_t startingPhaseStepAdjust = 1;

const uint8_t ON_CYCLE_MINIMUM_SECONDS = 600;		 // 10m
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

	uint8_t pwm = 0;	 // holds motor PWM
	uint8_t state = 0; // holds motor state
	// uint8_t lastState = 0;
	uint32_t cycleDuration = 0; // for this cycle
	// uint32_t lastDuration = 0;

	Pump_C()
	{ // constructor
		pinMode(HEAT_PUMP_PIN, OUTPUT);
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

	void start()
	{
		if (pwm > 0)
		{
			runOn();
		}
		else
		{
			state = 2; // motor starting phase

			resetDuration();
			timeRemaining = ON_CYCLE_MINIMUM_SECONDS;
			pulsePwmCounter = 0;													 // reset "pwm pulse" counter when turning on
			startingPhaseStepAdjust = STARTING_PHASE_STEP; // smooth pwm transition, subtract this from pwm, and then --

			coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
			pwm = ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_BOOST;
			pwm += (coldFloor && (COLD_FLOOR_PWM_BOOST + pwm) > pwm) ? COLD_FLOOR_PWM_BOOST : 0;
		}
	}

	void stop(bool skipTimer = false)
	{
		coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
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
		if (
				(floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) &&
				(floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE))
		{
			coldFloor = false;
		}

		pwm = (coldFloor) ? ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST : ON_PHASE_BASE_PWM;
	}

	void stepDownPwm()
	{ // during startup phase
		if (coldFloor)
		{
			if ((pwm - startingPhaseStepAdjust) >= (ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST))
				pwm -= startingPhaseStepAdjust;
			else
				pwm = ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST;
		}
		else
		{
			if ((pwm - startingPhaseStepAdjust) >= ON_PHASE_BASE_PWM)
				pwm -= startingPhaseStepAdjust;
			else
				pwm = ON_PHASE_BASE_PWM;
		}

		if (startingPhaseStepAdjust - 1 >= 1)
			startingPhaseStepAdjust -= 1;
	}

	void pulsePwm()
	{ // occasional high power pulse to prevent motor hangups
		pulsePwmCounter++;
		if (pulsePwmCounter >= PULSE_PWM_SECONDS_INTERVAL)
		{
			pulsePwmCounter = 0;
			pwm = (ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT >= pwm) ? ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT : pwm;
		}
	}

	void update()
	{									 // called every second
		cycleDuration++; // this cycle cycleDuration
		debugHighsLowsFloor();

		if (pwm > 0)
		{ // time spent on
			accumOn++;
		}

		if (Input >= Setpoint)
		{ // time spent above setpoint
			accumAbove++;
		}

		if (timeRemaining > 0)
		{
			timeRemaining--;
			if (state == 0)
			{ // offWithTimer();
				// special cases in extreme change
				if (Output == outputMax)
					start();
				if (Input <= Setpoint - EMERGENCY_ON_TRIGGER_OFFSET)
					start();
			}
			else if (state == 1)
			{ // onWithTimer();
				runOn();
				pulsePwm(); // occasional higher PWM pulse
			}
			else if (state == 2)
			{ // startup();
				if (timeRemaining > END_OF_STARTUP_TIMER)
					stepDownPwm();
				else
					runOn();
			}
		}
		else
		{
			// NO timer restriction (extended phase)
			if (state == 0)
			{ // offContinued();
				if (Output > MIDPOINT)
					start(); // most common start trigger
				else if (pwm > 0)
					stop(); // coming from delay, in case we need to stop
			}
			else if (state == 1)
			{ // onContinued();
				if (Output <= MIDPOINT)
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

		analogWrite(HEAT_PUMP_PIN, pwm);
	}

} pump = Pump_C();
