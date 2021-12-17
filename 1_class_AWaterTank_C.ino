class WaterTank_C {
    private: 
        const uint8_t PIN = A3;

        // This is the EMA which is displayed as the level
        //  higher is smoother but slower response so less accurate
        const uint8_t EMA_PERIODS_SHORT = 20; // * 2.5s per period = 50 s

        // This is a slow EMA for comparison against the first one
        //  to detect when tank is being filled. Higher means slow response,
        //  making it easier to tell when filling
        //  Too high takes longer to turn off filling icon;
        //  oh wait, filling icon should just be turned off from detecting when it gets to FF and was filling; should not depend on this trigger alone

        // increase this to allow wider sensitivity for FILL_TRIGGER
        const uint8_t EMA_PERIODS_LONG = 120; //  * 2.5  == 5m

        // This is the EMA for our difference between the above EMAs
        //  should be ??
        const uint8_t EMA_PERIODS_DIFF = 20; //  * 2.5 = 50 s

        // higher makes it take longer to trigger ON filling
        //  difference(EMA) must be > this to trigger filling, should be high
        //  enough to not trigger randomly when not filling, but not too high
        const uint8_t FILL_TRIGGER = 30;
    //120, 24, 10
        const uint8_t STOP_FILL_TRIGGER = 18; // lower makes it harder to trigger OFF filling
        
    public:
        bool filling = false;

        uint16_t ema = 0;
        uint16_t lastEma = 0;

        uint16_t slowEma = 0;
        uint16_t lastSlowEma = 0;

        int16_t diff = 0;
        int16_t lastDiff = 0;

        int16_t diffEma = 0;
        int16_t lastDiffEma = 0;

        String percent = "--";

        WaterTank_C() {
            pinMode(PIN, INPUT);
        }

        init() {            
            ema = analogRead(PIN);
            lastEma = ema;
            slowEma = ema;
            lastSlowEma = ema;
            
            diffEma = ema - slowEma;
            lastDiffEma = ema - slowEma;
        }

        String getTankPercent() {    
            uint16_t LOW_BOUND = 350;
            uint16_t HIGH_BOUND = 750; // 743 ~ 0.98%
            uint8_t tankPercent;
            String output = "FF";

            if (ema >= HIGH_BOUND) {
                return output; // full

            } else if (ema > LOW_BOUND) {
                tankPercent = (float) 100 * (ema - LOW_BOUND) / (HIGH_BOUND - LOW_BOUND);
                output = (tankPercent < 10) ? " " + String(tankPercent) : String(tankPercent);
                return output;
            } else if (ema < 250) {
                output = "ER";
                return output; // error
            }
            output = "EE";
            return output; // empty
        }

        
        update() { // every 2.5 seconds
          
            // calculate water level quick EMA
            lastEma = ema; // save last EMA
            ema = (analogRead(PIN) * (2. / (1 + EMA_PERIODS_SHORT )) + lastEma * (1 - (2. / (1 + EMA_PERIODS_SHORT ))));

            // calculate water level slow EMA
            lastSlowEma = slowEma;
            slowEma = (analogRead(PIN) * (2. / (1 + EMA_PERIODS_LONG )) + lastSlowEma * (1 - (2. / (1 + EMA_PERIODS_LONG ))));

            // calculate difference
            lastDiff = diff; // save old diff for fun
            diff = ema - slowEma;
            
            // calculate difference EMA
            lastDiffEma = diffEma;
            diffEma = (diff * (2. / (1 + EMA_PERIODS_DIFF )) + lastDiffEma * (1 - (2. / (1 + EMA_PERIODS_DIFF ))));

            // Determine if tank is currently being filled
            // If current read is higher than longerEMAread + window, yes filling.
            if (diffEma >= FILL_TRIGGER) {
                if (percent == "FF") {
                    filling = false;
                } else {
                    filling = true;
                }
            }

            // if current read is around longerEMAread, not filling. 
            if (diffEma <= STOP_FILL_TRIGGER) filling = false;

            percent = getTankPercent();
        }

} waterTank = WaterTank_C();