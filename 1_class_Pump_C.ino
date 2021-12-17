class Pump_C {
    private:
        const uint8_t PIN = 5;

        const uint8_t STARTING_PHASE_PWM_BOOST = 25;
        const uint8_t STARTING_PHASE_SECONDS = 6;

        const uint8_t ON_PHASE_BASE_PWM = 151;
        const uint8_t COLD_FLOOR_PWM = 223;

        const uint8_t ON_CYCLE_MINIMUM_SECONDS = 600; // 10m
        const uint16_t OFF_CYCLE_MINIMUM_SECONDS = 1800; // 30m
        
        const uint32_t END_OF_STARTUP_TIMER = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;

        static const uint8_t DELAY_SECONDS = 3;

        //uint8_t lastState = 0;
        uint32_t timeRemaining = 0;
        uint8_t proportionalPwm = 0;

        //uint32_t lastDuration = 0;

    public:
        uint8_t pwm = 0;
        uint8_t state = 0;
        uint32_t duration = 0; // for cycle
        uint32_t time = 0; // total running time

        uint32_t accumOn = 0;
        uint32_t accumAbove = 0; // total time spent above setpoint

        Pump_C () { // constructor
            pinMode(PIN, OUTPUT);
        }

        void setState(uint8_t i) {
            //lastState = state; // save lastState
            state = i;
        }

        void resetDuration() {
            //lastDuration = duration;
            duration = 0;
        }

        String getStatus() {
            return (state == 0) ? "OFF" :
                (state == 3) ? "---" :
                String(pwm);
        }

        uint8_t getPwmFromOutput() {
            return (ON_PHASE_BASE_PWM + ((float) (Output - MIDPOINT) / (outputMax - MIDPOINT) * (255 - ON_PHASE_BASE_PWM)));
        }

        void checkAfter(uint16_t delaySeconds = DELAY_SECONDS) {
            setState(3); // "delay" state
            timeRemaining = delaySeconds;
        }

        void start() {
            if (pwm > 0) { 
                runOn();
            } else {
                // save until turning off
                coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE); 
                setState(2); // motor starting phase
                resetDuration();
                pwm = ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_BOOST;
                pwm = (coldFloor && coldFloor > pwm) ? COLD_FLOOR_PWM : pwm;
                timeRemaining = ON_CYCLE_MINIMUM_SECONDS;
                // proportionalPwm = getPwmFromOutput();
                // pwm = (proportionalPwm > pwm) ? proportionalPwm : pwm;
                analogWrite(PIN, pwm); // motor startup
            }
        }

        void runOn() { // after start phase
            setState(1);
            // at least base PWM, or COLD_FLOOR_PWM
            pwm = (coldFloor) ? COLD_FLOOR_PWM :
                ON_PHASE_BASE_PWM;
            //pwm = (proportionalPwm > pwm) ? proportionalPwm : pwm;
            analogWrite(PIN, pwm);
        }

        void takeHigherPwm() {
            // at least base PWM, or COLD_FLOOR_PWM
            uint8_t tempPwm = (coldFloor) ? COLD_FLOOR_PWM : ON_PHASE_BASE_PWM;
            //tempPwm = (proportionalPwm > tempPwm) ? proportionalPwm : tempPwm;
            pwm = (tempPwm > pwm) ? tempPwm : pwm; // take higher    
            analogWrite(PIN, pwm);
        }

        void stop(bool skipTimer = false) {
            coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE); // save until turning back on
            setState(0);
            resetDuration();
            pwm = 0;
            timeRemaining = (skipTimer) ? 0 : OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)

            analogWrite(PIN, pwm);
        }


        void update() { // called every second  
            debugHighsLowsFloor();

            duration++; // cycle duration

            time++; // total run time
            // time spent on
            if (pwm > 0) {
                accumOn++;
            }

            // time spent above setpoint
            if (Input >= Setpoint) {
                accumAbove++;
            }


            if (timeRemaining > 0) { 
                // timer restriction
                timeRemaining --;
                switch(state) {
                    case(0): // Off
                        // special case in extreme change
                        if (Output == outputMax) start();
                        break;

                    case(1): // On
                        takeHigherPwm();
                        break;

                    case(2): // starting end trigger
                        if (timeRemaining <= END_OF_STARTUP_TIMER) runOn();
                        break;
                }

            } else { 
                // NO timer restriction (extended phase)
                switch(state) {
                    case(0):
                        // most common start trigger
                        if (Output > MIDPOINT) start(); 
                        // coming from delay, in case we need to stop
                        else if (pwm > 0) stop(); 
                        break;

                    case(1):
                        // most common stop trigger
                        if (Output <= MIDPOINT) stop(); 
                        break;

                    case(3): // delay
                        setState(0); // it'll check next update
                        break;
                }
            }
        }  
} pump = Pump_C();
