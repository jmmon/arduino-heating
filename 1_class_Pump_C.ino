// ~489/ 518 cold floor
// 590/632 - 32m in to on-cycle
// 622/649 floor (warm but not hot)


class Pump_C {
    private:
        const uint8_t PIN = HEAT_PUMP_PIN;

        const uint8_t ON_PHASE_BASE_PWM = 125;
        const uint8_t COLD_FLOOR_PWM_BOOST = 44;

        // PWM boost during motor start
        const uint8_t STARTING_PHASE_PWM_BOOST = 56; // ({sum(1-10)} == 55) + {1*remaining seconds}
        const uint8_t STARTING_PHASE_SECONDS = 11;
        const uint8_t STARTING_PHASE_STEP = 10; // start amt for startingPhaseStepAdjust
        uint8_t startingPhaseStepAdjust = 1;

        const uint8_t ON_CYCLE_MINIMUM_SECONDS = 600; // 10m
        const uint16_t OFF_CYCLE_MINIMUM_SECONDS = 1800; // 30m 
        const uint32_t END_OF_STARTUP_TIMER = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;

        // PWM boost every so often, in case pump gets hung up:
        const uint16_t PULSE_PWM_SECONDS_INTERVAL = 1800; // seconds
        const uint8_t PULSE_PWM_AMOUNT = 22; // boost
        uint16_t pulsePwmCounter = 0; // counts the seconds since last boost

        static const uint8_t DELAY_SECONDS = 3; // default cycleDuration for delay-start
        const uint8_t OFFSET = 5; // if in 30-min off cycle, if temp drops by this amount off target, start the pump

        uint32_t timeRemaining = 0; // Counter for minimum cycle times

    public:
        uint32_t accumOn = 0; // time spent On
        uint32_t accumAbove = 0; // total time spent above setpoint
        
        uint8_t pwm = 0; // holds motor PWM
        uint8_t state = 0; // holds motor state
        uint8_t lastState = 0;
        uint32_t cycleDuration = 0; // for this cycle
        //uint32_t lastDuration = 0;


        Pump_C () { // constructor
            pinMode(PIN, OUTPUT);
        }

        String getStatus() {
            return (state == 0) ? "OFF" :
                (state == 3) ? "---" :
                String(pwm);
        }

        void setState(uint8_t i) {
            //lastState = state; // save lastState
            state = i;
        }

        void resetDuration() {
            //lastDuration = cycleDuration;
            cycleDuration = 0;
        }

        void start() {
            if (pwm > 0) {
                runOn();
                
            } else {
                setState(2); // motor starting phase
                resetDuration();
                timeRemaining = ON_CYCLE_MINIMUM_SECONDS;
                pulsePwmCounter = 0; // reset "pwm pulse" counter when turning on
                startingPhaseStepAdjust = STARTING_PHASE_STEP; // smooth pwm transition, subtract this from pwm, and then --
                
                coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE); 
                pwm = ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_BOOST;
                pwm += (coldFloor && (COLD_FLOOR_PWM_BOOST + pwm) > pwm) ? 
                    COLD_FLOOR_PWM_BOOST : 
                    0;
            }
        }

        void stop(bool skipTimer = false) {
            coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
            setState(0);
            resetDuration();
            pwm = 0;
            timeRemaining = (skipTimer) ? 
                0 : 
                OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)
        }

        void checkAfter(uint16_t delaySeconds = DELAY_SECONDS) {
            setState(3); // "delay" state
            timeRemaining = delaySeconds;
        }

        void runOn() { // continue run phase
            setState(1);
            // turn off coldFloor if floor stays warm for a bit
            if ((floorEmaAvg >= FLOOR_WARMUP_TEMPERATURE) &&
                (floorEmaAvgSlow >= FLOOR_WARMUP_TEMPERATURE)
            ) {
                coldFloor = false;
            }
            
            pwm = (coldFloor) ? 
                ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST :
                ON_PHASE_BASE_PWM;
        }

        void stepDownPwm() { // during startup phase
            if (coldFloor) {
                if ((pwm - startingPhaseStepAdjust) >= (ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST)) pwm -= startingPhaseStepAdjust;
                else pwm = ON_PHASE_BASE_PWM + COLD_FLOOR_PWM_BOOST;
                
            } else {
                if ((pwm - startingPhaseStepAdjust) >= ON_PHASE_BASE_PWM) pwm -= startingPhaseStepAdjust;
                else pwm = ON_PHASE_BASE_PWM;
                
            }
            
            if (startingPhaseStepAdjust - 1 >= 1) startingPhaseStepAdjust -= 1;
        }

        void pulsePwm() { // occasional pulse to prevent motor hangups
            pulsePwmCounter++;
            if (pulsePwmCounter >= PULSE_PWM_SECONDS_INTERVAL) {
                pulsePwmCounter = 0;
                pwm = (ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT >= pwm) ? 
                    ON_PHASE_BASE_PWM + PULSE_PWM_AMOUNT :
                    pwm ;
            }
        }


        void update() { // called every second  
            cycleDuration++; // this cycle cycleDuration
            debugHighsLowsFloor();

            // time spent on
            if (pwm > 0) {
                accumOn++;
            }

            // time spent above target
            if (Input >= Setpoint) {
                accumAbove++;
            }

            if (timeRemaining > 0) { 
                timeRemaining --;
                switch(state) {
                    case(0): // offWithTimer();
                        // special cases in extreme change
                        if (Output == outputMax) start();
                        if (Input <= Setpoint - OFFSET) start();
                        
                        break;

                    case(1): // onWithTimer();
                        runOn();
                        pulsePwm(); // occasional higher PWM pulse
                        
                        break;

                    case(2): // startup();
                        if (timeRemaining > END_OF_STARTUP_TIMER) stepDownPwm();
                        else runOn();
                        
                        break;
                }

            } else { 
                // NO timer restriction (extended phase)
                switch(state) {
                    case(0): // offContinued();
                        if (Output > MIDPOINT) start(); // most common start trigger
                        else if (pwm > 0) stop(); // coming from delay, in case we need to stop
                        
                        break;

                    case(1): // onContinued();
                        runOn();
                        pulsePwm();
                        if (Output <= MIDPOINT) stop(); // most common stop trigger
                        
                        break;

                    case(3): // delayTimerEnd();
                        setState(0); // it'll check next update
                        
                        break;
                }
            }
            
            analogWrite(PIN, pwm);
            
        }  
} pump = Pump_C();
