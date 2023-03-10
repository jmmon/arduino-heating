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

