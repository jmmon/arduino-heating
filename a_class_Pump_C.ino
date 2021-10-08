class Pump_C {
    private:
        uint8_t PIN = 5;

        uint8_t STARTING_PHASE_PWM = 220;
        uint8_t ON_PHASE_BASE_PWM = 191;

        uint8_t STARTING_PHASE_SECONDS = 4;
        uint8_t ON_CYCLE_MINIMUM_SECONDS = 180;
        uint16_t OFF_CYCLE_MINIMUM_SECONDS = 300;

        uint8_t state = 0;
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

        setState(uint8_t i) {
            state = i;
        }

        uint8_t getState() {
            return state;
        }
        
        uint32_t getDuration() {
            return duration;
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
            setState(2);
            resetDuration();
            timeRemaining = ON_CYCLE_MINIMUM_SECONDS; // 3 minute minimum
            pwm = STARTING_PHASE_PWM;
            analogWrite(PIN, pwm);
        }

        void stop() {
            setState(0);
            resetDuration();
            timeRemaining = OFF_CYCLE_MINIMUM_SECONDS; // 5 minute minimum
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

            // update accumulative times
            if (state == 0) {
                accumOff++;
            } else if (state != 3) {
                accumOn++;
            }

            //
            if (Input > Setpoint) {
                accumAbove++;
            } else if (Input < Setpoint) {
                accumBelow++;
            }

            if (timeRemaining > 0) { // if restrained by timer
                timeRemaining --;
                switch(state) {
                    case(0): // if off (with timeRemaining)
                        if (Output == outputMax) { // special case
                            start();
                        } else {
                            pwm = 0;
                        }
                        break;

                    case(1): // if on with timer, at least base PWM.
                        pwm = ((Output > MIDDLE) ? 
                            (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2)) : 
                            ON_PHASE_BASE_PWM);
                        break;

                    case(2): // motor start here
                        uint32_t endOfMotorStartup = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;
                        if (timeRemaining > endOfMotorStartup) {
                            pwm = STARTING_PHASE_PWM;
                        } else {
                            setState(1);
                        }
                        break;

                    case(3): // do nothing
                        break;
                }

            } else { // if not restrained by timeRemaining
                switch(state) {
                    case(0): // while off, not restrained by timeRemaining
                        if (Output > MIDDLE) {
                            start();
                        } else {
                            pwm = 0;
                        }
                        break;

                    case(1): // while on
                        if (Output > MIDDLE) {
                            pwm = (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2));
                        } else {    //if Output drops below 0 turn it off
                            stop();
                        }
                    break;

                    case(2): // won't happen
                        break;

                    case(3): // delay timeRemaining is up, check if motor needs to be on
                        set = false;
                        setState(0);
                        break;
                }
            }

            analogWrite(PIN, pwm);
        }
} 
pump = Pump_C();