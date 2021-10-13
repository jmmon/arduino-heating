class Pump_C {
    private:
        uint8_t PIN = 5;

        uint8_t STARTING_PHASE_PWM = 220;
        uint8_t ON_PHASE_BASE_PWM = 191;
        uint8_t COLD_FLOOR_PWM = 255;

        uint8_t STARTING_PHASE_SECONDS = 4;
        uint8_t ON_CYCLE_MINIMUM_SECONDS = 180;
        uint16_t OFF_CYCLE_MINIMUM_SECONDS = 300;

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
            setState(2);
            resetDuration();
            timeRemaining = ON_CYCLE_MINIMUM_SECONDS; // 3 minute minimum

            if (coldFloor) {
                pwm = COLD_FLOOR_PWM;
            } else {
                if (pwm != 0) {
                    pwm = STARTING_PHASE_PWM;
                }
            }
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


            switch(state) {
                case(0):
                    if (timeRemaining > 0) { // while off (with timeRemaining)
                        if (Output == outputMax) { // special case
                            start();
                        } else {
                            pwm = 0;
                        }

                    } else { // while off, not restrained by timeRemaining
                        if (Output > MIDDLE) {
                            start();
                        } else {
                            pwm = 0;
                        }
                    }
                    break;


                case(1): // on phase
                    if (timeRemaining > 0) { // if on with timer, at least base PWM.
                        if (coldFloor) {
                            pwm = COLD_FLOOR_PWM;
                        } else {
                            pwm = ((Output > MIDDLE) ? (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2)) : ON_PHASE_BASE_PWM);
                        }
                        
                    } else { // while on
                        if (Output > MIDDLE) {
                            if (coldFloor) {
                                pwm = COLD_FLOOR_PWM;
                            } else {
                                pwm = (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2));
                            }
                            
                        } else {    //if Output drops below 0 turn it off
                            stop();
                        }
                    }
                    break;


                case(2):
                    if (timeRemaining > 0) { // motor start here
                        uint32_t endOfMotorStartup = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;
                        if (timeRemaining > endOfMotorStartup) { // if still starting
                            if (coldFloor) {
                                pwm = COLD_FLOOR_PWM;
                            } else {
                                pwm = STARTING_PHASE_PWM;
                            }
                            
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

            if (timeRemaining > 0) { // if restrained by timer
                timeRemaining --;
            }

            // if (timeRemaining > 0) { // if restrained by timer
            //     timeRemaining --;
            //     switch(state) {
            //         case(0): // if off (with timeRemaining)
            //             if (Output == outputMax) { // special case
            //                 start();
            //             } else {
            //                 pwm = 0;
            //             }
            //             break;

            //         case(1): // if on with timer, at least base PWM.
            //             pwm = ((Output > MIDDLE) ? 
            //                 (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2)) : 
            //                 ON_PHASE_BASE_PWM);
            //             break;

            //         case(2): // motor start here
            //             uint32_t endOfMotorStartup = ON_CYCLE_MINIMUM_SECONDS - STARTING_PHASE_SECONDS;
            //             if (timeRemaining > endOfMotorStartup) {
            //                 pwm = STARTING_PHASE_PWM;
            //             } else {
            //                 setState(1);
            //             }
            //             break;

            //         case(3): // do nothing
            //             break;
            //     }

            // } else { // if not restrained by timeRemaining
            //     switch(state) {
            //         case(0): // while off, not restrained by timeRemaining
            //             if (Output > MIDDLE) {
            //                 start();
            //             } else {
            //                 pwm = 0;
            //             }
            //             break;

            //         case(1): // while on
            //             if (Output > MIDDLE) {
            //                 pwm = (ON_PHASE_BASE_PWM + ((Output - MIDDLE) / 2));
            //             } else {    //if Output drops below 0 turn it off
            //                 stop();
            //             }
            //         break;

            //         case(2): // won't happen
            //             break;

            //         case(3): // delay timeRemaining is up, check if motor needs to be on
            //             holdSetPage = false;
                        
            //             setState(0); // 0 state, no timer, this is where we check if we need to start.
            //             break;
            //     }
            // }

            analogWrite(PIN, pwm);
        }  
