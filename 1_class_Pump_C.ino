class Pump_C {
    private:
        uint8_t PIN = 5;

        uint8_t STARTING_PHASE_PWM_OFFSET = 48;
        uint8_t STARTING_PHASE_SECONDS = 4;

        uint8_t ON_PHASE_BASE_PWM = 171;
        uint8_t COLD_FLOOR_PWM = 223;

        uint8_t ON_CYCLE_MINIMUM_SECONDS = 300; // 3m
        uint16_t OFF_CYCLE_MINIMUM_SECONDS = 1800; // 30m

        uint8_t state = 0;
        uint8_t lastState = 0;
        uint32_t timeRemaining = 0;
        uint8_t pwm = 0;

        uint32_t duration = 0;
        uint32_t lastDuration = 0;

        uint32_t accumOn = 0;
        uint32_t accumOff = 0;

        uint32_t accumAbove = 0; // total time spent above setpoint
        uint32_t accumBelow = 0; // total time spent below setpoint


    public:
        Pump_C () { // constructor
            pinMode(PIN, OUTPUT);
        }

        uint8_t getState() {
            return state;
        }

        setState(uint8_t i) {
            lastState = state; // save lastState
            state = i;

        }
        
        uint32_t getDuration() {
            return duration;
        }
        
        uint32_t getTimeRemaining() {
            return timeRemaining;
        }

        resetDuration() {
            lastDuration = duration;
            duration = 0;
        }

        String getStatus() {
            if (state == 1) return String(pwm);
            return "OFF";
        }



        uint32_t getAccumOn() {
            return accumOn;
        }

        uint32_t getAccumOff() {
            return accumOff;
        }
        uint32_t getAccumAbove() {
            return accumAbove;
        }

        uint32_t getAccumBelow() {
            return accumBelow;
        }



        uint8_t getPwm() {
            return pwm;
        }



        void start() {
            coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
            setState(2);
            resetDuration();
            timeRemaining = ON_CYCLE_MINIMUM_SECONDS; // 3 minute minimum
            

            // pwm = (pwm != 0) ? (ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_OFFSET) : 0;
            pwm = (coldFloor) ? COLD_FLOOR_PWM :
               (pwm != 0) ? (ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_OFFSET) :
                0;

            uint8_t temp = ON_PHASE_BASE_PWM + ((float) (Output - MIDPOINT) / (outputMax - MIDPOINT) * (255 - ON_PHASE_BASE_PWM)); // set based on Output

            pwm = (pwm != 0 && temp > pwm) ? temp : pwm;

            analogWrite(PIN, pwm);

        }

        void stop(bool init = false) {
            coldFloor = (floorEmaAvg < FLOOR_WARMUP_TEMPERATURE);
            setState(0);
            resetDuration();
            timeRemaining = (init) ? 0 : OFF_CYCLE_MINIMUM_SECONDS; // 30 minute wait (except on init)
            pwm = 0;
            analogWrite(PIN, pwm);
        }

        void checkAfter(uint16_t delaySeconds = 3) {
            setState(3);
            timeRemaining = delaySeconds;
        }


        void update() { // called every second  
            debugHighsLowsFloor();

            duration++;

            // time spent on or off
            if (state == 0) {
                accumOff++;
            } else if (state != 3) {
                accumOn++;
            }

            // time spent above or below setpoint
            if (Input > Setpoint) {
                accumAbove++;
            } else if (Input < Setpoint) {
                accumBelow++;
            }

            // if restrained by timer
            if (timeRemaining > 0) {
                timeRemaining --;
            }

            
            uint8_t tempPwm = ON_PHASE_BASE_PWM + ((float) (Output - MIDPOINT) / (outputMax - MIDPOINT) * (255 - ON_PHASE_BASE_PWM)); // set based on Output

            switch(state) {
                case(0):
                    if (timeRemaining > 0) { // while off (with timeRemaining)
                        if (Output == outputMax) { // special case
                            start();
                        } else {
                            pwm = 0;
                        }

                    } else { // while off, not restrained by timeRemaining
                        if (Output > MIDPOINT) { // most common start trigger
                            start();
                        } else {
                            pwm = 0;
                        }
                    }
                    break;


                case(1): // on phase
                    if (timeRemaining > 0) { // if on with timer, at least base PWM.

                        pwm = (coldFloor) ? COLD_FLOOR_PWM :
                            ON_PHASE_BASE_PWM;

                        pwm = (tempPwm > pwm) ? tempPwm : pwm;
                            
                        
                    } else { // while on
                        if (Output > MIDPOINT) {
                            
                            pwm = (coldFloor) ? COLD_FLOOR_PWM :
                            (ON_PHASE_BASE_PWM);

                            pwm = (tempPwm > pwm) ? tempPwm : pwm;
                            
                        } else {    //if Output drops below 0 turn it off
                            stop();
                        }
                    }
                    break;


                case(2):
                    if (timeRemaining > 0) { // motor start here
                        uint32_t endOfMotorStartup = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;
                        if (timeRemaining > endOfMotorStartup) { // if still starting
                            // pwm = (ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_OFFSET);
                            pwm = (coldFloor) ? COLD_FLOOR_PWM : 
                                (ON_PHASE_BASE_PWM + STARTING_PHASE_PWM_OFFSET);

                            pwm = (tempPwm > pwm) ? tempPwm : pwm;
                            

                            
                        } else {
                            setState(1);
                        }

                    } // else do nothing
                    break;


                case(3):
                    if (timeRemaining > 0) { // do nothing
                        
                    } else { // delay timeRemaining is up, check if motor needs to be on
                        holdSetPage = false;
                        
                        setState(0); // 0 state, no timer, this is where we check if we need to start.
                    }
                    break;
            }

            analogWrite(PIN, pwm);
        }  

} pump = Pump_C();
