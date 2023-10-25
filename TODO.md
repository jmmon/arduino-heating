lcd references:
air,
pump,
waterTank,
floorSensor

Sketch uses 18510 bytes (60%) of program storage space. Maximum is 30720 bytes.
Global variables use 1348 bytes (65%) of dynamic memory, leaving 700 bytes for local variables. Maximum is 2048 bytes.

Sketch uses 18422 bytes (59%) of program storage space. Maximum is 30720 bytes.
Global variables use 1332 bytes (65%) of dynamic memory, leaving 716 bytes for local variables. Maximum is 2048 bytes.

Sketch uses 18354 bytes (59%) of program storage space. Maximum is 30720 bytes.
Global variables use 1332 bytes (65%) of dynamic memory, leaving 716 bytes for local variables. Maximum is 2048 bytes.


disabling proportional PWM



testing "static const byte" chars for the custom chars
didn't work; is there another type which would work (and save space)?



Could use net-above/below-temperature time to bump set temperature up, so heat starts sooner to help balance out the net-above/below-temperature





04/20/2022
Cleaned up variables!

before:
	Sketch uses 26416 bytes (85%) of program storage space. Maximum is 30720 bytes.
	Global variables use 1389 bytes (67%) of dynamic memory, leaving 659 bytes for local variables. Maximum is 2048 bytes.

after:
	Sketch uses 23482 bytes (76%) of program storage space. Maximum is 30720 bytes.
	Global variables use 1289 bytes (62%) of dynamic memory, leaving 759 bytes for local variables. Maximum is 2048 bytes.


Saved this much: !!
	26416 - 23482 = 	2934 bytes storage,
	1389 - 1289 =  		100 bytes memory



TODO 4/3/22

(using the t-stat pin with a different wavelength to distinguish)

Add screen menu functionality using a button to switch between screens.

press => cycles to the next page in the menu cycle (all pages existing);
				- starts a timer: 5 seconds?
					* after time is up, switches to view cycle
				- minimum time between presses to be sure it's intentional (~150ms?)

double press => pause the screen (pause the cycle timer)

press+hold => enable/disable page from the view cycle 
			-(beep will sound: a "do, mi" or "do fa" note frequencies)





# Dec 4th 2022:

    Idea: could change the LCD pages so it compiles a string and then prints it
		That may reduce size and could diff check on last vs next strings and only print when changed?\
	Can't do this with custom icons probably..?
	\xhh
	where hh == hexadecimal value of special character
  **Done**

   #### Bug Reports:
	- Water level does not beep once hitting full!!!
    - Water level is a bit behind where it should be, wonder if I could tweak the numbers better. 
		* Seems to be behind by ~7+%!!!




# Dec 31st 2022:
	Should keep a trend (other than just the floor temp) and use it to maintain temperature better.
	Problem: When super cold outside, the inside set temperature is a little cold;
	And when not as cold, the inside temperature is a little warm!

	Trying to make it so when the temperature is warmer outside, the inside heats a little less. (Move the target temperature lower for example).
	When the temperature is colder outside, the inside heats up a little more. (Move the target temperature higher for example)

*** Should make a variable: some sort of ratio. That way I can tweak the variable to change how the trend affects the target temperature.
  e.g. fastDrop_ratio ??
  
  **Done?** Now using the temperature rate of change to decide how to preempt the heating/cooling



# Jan 19th 2023:
	Need to make sure water alarm works better!
	Should be preemptive: for example:
		If tank is > 80% and recent water-level-trend is high, start the alarm!
	That way I (hopefully) don't run into the issue of overfilling


# March 8th 2023:
  Should translate the water tank rate into a "Percent per minute" flow, or something like That
  Then I might be able to better estimate the current level based on the short EMA + some extra
  Just to make sure the tank never overfills again!

  #### Feature Request:
    new variable for waterTank: 
        - `uint16_t percentPerMinute = [range 0~1000, represents 0.0~100.0%]` water tank level rate of change

# March 10th 2023:
  #### Feature Request:
   **Occasional anti-freeze mode:**
      Goal: Hope to prevent freezing of pipes inside the walls.
      What: occasionally run, cycle long enough that the tubes (specifically the BB/recirc exterior loop) are full of hot water. 
        - How often? Every 2 hours? 3 hours? 4 hours?
        - On full speed, to reach ~110*F on the BB/recirc loop, requires approximately 5 minutes (300 seconds) 
          * when snowing outside, probably near 30*F so not that cold! Might take longer on colder days, when in negatives
          * So could run it for 7 minutes at full speed
          * How much time on Normal speed? Might take 2-3x as much time. Could do an in-between speed
        - How to disable this during the summer? If setpoint is < 60, don't run this?
        - Don't run when: pump has been on for x minutes during the last y duration?
          * if tubes are already hot, skip the cycle
          * Maybe save a "recent longest cycle duration":
            - if pump ran for (duration) continuous minutes during the last (interval / 2), skip the antifreeze cycle

  #### Major Change Idea:
   **T-Stat: Standalone power supply:**
    Goal: Prevent beeping from causing strange temperature readings
    What: When filling, and alarm is beeping, temp readings fluctuate wildly. Maybe caused by voltage drop due to power requirement of buzzer?
    Tasks:
        - Check power requirements of buzzer **OR** check for voltage drop when buzzer is beeping (could run a test alarm pattern at startup for easy measuring)
        - If cause is probable, should add a voltage converter to the T-Stat
            - Send 12v or 24v to the T-Stat
            - Receive it with the voltage converter and transform it into what is needed (5v or 3.3v)
                - Do I need both 5v AND 3.3v? Could run resistors to provide 3.3v

**OR**
   **Rebuild T-stat as its own Arduino:**
    Goal: 
        - separate concerns: T-Stat will control the display, temp buttons, display screen paging, buzzer
        - Temp-Sensors will still go to the Heat arduino?
            - Heat arduino should be able to stand-alone in case the T-stat goes down (or while uploading)
                - static temperature setting
    Communication:
        - Heat is master, T-Stat is slave
        - T-Stat will receive temperature datas from the Heat arduino (how? serial? Sending messages maybe)
        - T-Stat will send pump signal to the Heat arduino (serial message with a PWM value? or at least a mode value)
        - Temp-Sensor on T-Stat will be routed back to Heat arduino I guess? And then forwareded to T-Stat


# October 23 2023
  #### Feature Request:
**Heartbeat anti-freeze mode: v2**
  Goal: prevent freezing of both cold and hot pipes inside the walls
    What: Occasionally run, every 1-4 hours, enough to keep the cold water pipe from freezing
  
- When switching cycles, note the time of the last cycle and use it to calculate some sort of EMA.
- Actually, what we need is a ratio, so we need to get every onTime/offTime pair
- Start with an off cycle or start with an on cycle??
  - if first saving the off cycle, 
    - at evening time it will bump down the EMA to reduce heartbeat
    - at morning time it will bump the ema up to increase heartbeat
  - if first saving on cycle,
    - at evening time it will bump up the EMA to increase heartbeat
    - at morning time it will bump the ema up to reduce heartbeat

- When switching cycles, save the last cycle time in a variable
- Once the next cycle ends, we have the pair for the last cycle time.
- Calculate ratio from those two:
  - we want on time / total time, so `onTime/(onTime + offTime)`
- and then calculate that into the EMA

- The EMA would range from 0 to 1 and represent ratio of on to total time
```
uint32_t lastCycleTime = 0;
loop {
    // wait...

    if (cycleOff) {
        lastOffCycleTime = cycleTime;
        pump.turnOn();
    } else {
        // cycleTime is now "lastOnCycleTime"
        calculateIntoOnOffEma(lastOffCycleTime, cycleTime);
        pump.turnOff();
    }
}

calculateIntoOnOffEma(off, on) {
    uint32_t total = off + on;
    
    float ratio = on / total;

    calculateOnOffRatioEma(ratio);
}
```

- So our heartbeat should run during off cycles, but I must continue the same timerCount
  - e.g. do NOT start the timer when the off cycle starts; RESUME the timer
    and PAUSE it when the on cycle starts.

- We need a threshold to start the heartbeat, because we probably don't need it
  when the ratio is less than ~0.1 or some number

- EMA might be taken every second? So 60 * 60 * 24 = 86400 seconds per day
  so that will be the EMA duration

- Now we need to determine how much we want to heartbeat. We need:
  `offTime` and/or `onTime`, `totalCycleTime`. Then, while cycle is off, we continue our timer
  and when it's time to do a heartbeat, we stay on for `onTime` and then turn off

- Ok, so here's how controlling the timer works: 
``` 
uint32_t timerCount = 0;
isHeartbeatOn = false;


loop {
    if (cycleOff) {
        runTimer()
    }
}

function updateTimer() {
    // increment timer
    timerCount ++;

    if (isHeartbeatOn) {
        // turn off heartbeat after on for calculatedOnTime
        if (timerCount >= calculatedOffTime + calculatedOnTime) {
            timerCount = 0; // reset timer
            isHeartbeatOn = false;
        }
    } else {
        // turn on heartbeat after off for long enough
        if (timerCount >= calculatedOffTime) {
            isHeartbeatOn = true;
        }

    }
}

```

- Calculating the times:
- How much do we need at most? 1m on out of 30m?
- use the ratio to adjust the off time according to the base
- 15% => 30m / 0.15 === 200 minutes
- 20% => 30m / 0.2 === 150 minutes
- 25% => 30m / 0.25 === 120 minutes
- 30% => 30m / 0.3 === 100 minutes
- 40% => 30m / 0.4 === 75 minutes // doubt we'll go past here
- 50% => 30m / 0.5 === 60 minutes
- 75% => 30m / 0.75 = 40 minutes
- 100% => 30m / 1 === 30 minutes

- it'll work for now... but I want something a bit more logical to base it off of


```
uint32_t BASE = 60 * 30 * 1000; // 30m
uint32_t calculatedOnTime = 60 * 1000 // ms === 1 minute
uint32_t calculatedTotalTime = BASE / ratio;
uint32_t calculatedOffTime = calculatedTotalTime - calculatedOnTime;
```

## Need to figure out logic for handling pump states, integrating the new mode
### Currently:
- Switches to "start" and resets timer, starts at high PWM and steps down
- Later switches to "on" and continues the timer, now at a steady PWM
### Basic logic:
1. Update function runs every second, which runs a function depending on the current pump state.
2. These functions adjust the PWM or the state as necessary (depending on temp etc) for the next update.

- I want the Heartbeat time (time spent on) to also accumulate in accumOn
- It needs to start higher and step down, just like regular startup of pump
- Duration will be ~1m (maybe ~3m?), so it should step down for ~6 seconds and then continue for the remaining duration

- While off, we will occassionally run the heartbeat
> - this turns on the pump, and resets the cycle timer
> - after the heartbeat, I want the pump to turn off and I guess it can reset the cycle timer again
> - so after heartbeat, it should go back to `state === 0`

- When in heartbeat mode, should IGNORE the normal "stop/start" triggers!!!
> - I don't want it to turn off when in heartbeat mode!
> - e.g. if state === 5 or state === 4, ignore the temp checks!
