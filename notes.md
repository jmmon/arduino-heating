/**   Okay, so what this does is it checks the temperature every 2 seconds. 
 *      check if heat is needed:
   *      if yes, check if floor is cold:   //note, shouldBeInState is always != 2
   *        if yes, set shouldBeInState = 3; // warm up mode
   *        else set shouldBeInState = 1; // normal operation
   *      else set shouldBeInState = 0; // off
 * 
 *    now also, every timer seconds check if the pump state needs to be changed.
 *    
 *    function : every timer time or when TemperatureCheck calls a check
 *      if (isInState == shouldBeInState) { //  continue phases
 *        //then it's a continuation so just add 30 sec to the timer
 *        //set timer to continuation_time
 *      } else {
 *        switch(shouldBeInState) { //    new phases
 *          case(0):  //turning off
 *            //set timer to min_off
 *            isInState = 0;
 *            break;
 *          case(1): //turning on,check if it has been in 2_startUp
 *            if (isInState == 2) { //  this is when the pump has gone thru the startup phase
 *                //  so add on min_start - startupPhaseTime to the timer and
 *                isInState = 1;  //  set to on state
 *            } else {
 *                //if last state was not startup then it needs to startup
 *                isInState = 2;
 *                //  add on startupPhaseTime  to timer
 *            }
 *            break;
 *          case(3):  //  should be on in the warm up phase
 *            //set timer to min_WarmUp
 *            isInState = 3;
 *            break;
 *        }
 *      }
 * 
 *      buttons: detect button presses and trigger extra pump checks
 */





So, add a buffer for when floor is warm and air suddenly drops. Trying to negate "windows open + floor on = too hot."

Or, implement proper formula (requires testing) to regulate time and/or speed. . .

    The 3 parameter formula.

    It could regulate time, or it could regulate time and extend into motor speed; (and motor speed of course based on floor temp)

    So:  0-60 + slow-med-fast? or flexible pwm

    OR, just regulate PWM.
        //0(198), 199-255
        if (n < 199) {
            pwm = 0;
        } else if (n > 255) {
            pwm = 255;
        } else {
            pwm = n;
        }
        
    Have value restricted from 0-255? Or, have 0 == off, and go positive for on and negative for overflow. Easier if range matches pwm.

    One timer to measeure and track temp and request on/off

    Second timer to run motor with built in minimum cycle times. Turns on (to whatever PWM) when minimum cycle is over and when temp timer allows.



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


So. I do not want the PID output to directly regulate PWM of the motor. This causes continuous fluctuation in PWM (loudness). It would be better to have PID control my time cycle. 

    Each hour I'm on for n seconds.

But I also need a way for the code to integrate the higher speeds for maximum BTU output.

One way is to allow the PID to continue past 60 minutes mark per hour. When it goes past that mark, there could be (a few tiers, or, a fluid PWM adjustment) up to, say, 90 "minutes per hour", where any minutes past 60 are converted and added as PWM for the circulation motor.

    0-60 minutes == 0-60 minutes per hour at PWM = 199 PWM
    
    90 minutes   ==   60 minutes per hour at PWM = 255 PWM

Still want to integrate minimum cycle times:

    5m minimum "on" cycle time would mean:

        0-2.5m rounds down to 0, 2.5-5m rounds up to 5, or similar. Maybe err on side of less heat.



So: timer for reading sensors every 2~2.5 seconds

I think the other timer for the pump will be flowing, not set 60 minute blocks. So if the pump starts and initially is supposed to be on for 30 minutes but then the house heats faster than expected, the time should be reduced automatically.


PID output should set up around 0. Negative for off, positive for motor on. 





TODO: 
play more with PID;


integrate other temp sensors - not quite working




notess: 
need to detect when switches from 0 to 1 PID output, this is the trigger to turn on the motor cycle

the motor loop waits for this signal, then it turns the motor on for 3 minutes (or minimum time), adjusting the PWM higher if necessary based on PID output.

At the end of the minimum time, if the PID output is still > 0, the motor stays on at whatever PWM-PID.

Once the PID switches from >0 to <=0, this is the trigger to switch to the off cycle.

The motor loop then keeps the motor off for a minimum time, and until the signal to turn back on.


        Pump class: ?

        setPwm signals out whatever PWM to the appropriate pin


motor loop: loops every second, updating the PWM of the motor based on its own timer restrictions, the startup phase special case, and the PID output









Not sure PID is best. Could use PID for basic timing, and then use floor sensor to detect when high flow is necessary (to jump start the heating).

    What happens if Output == outputMax ? means requires most heat. Do we go max speed?



water lvl stabilization:
    As lvl drops, keep lowest number. If it raises above lowest number >= n, keep highest number.

    Or, boolean to track if it's being used or being filled (going down or up). If going down, always keep lowest number. If going up, always keep highest number. And to trigger the switching of the boolean, have rules something like:
        if (cur > (EMA + n)) filling = true;
        if (cur < EMA) filling = false;



Add in caring abt flr tmp done

if on, don't need to "start" motor done



Seemed to not run start phase when I turned up the temp earlier. (From (off, preheated, above setpoint) to (on, missing startup))

TODO: 
    Motor overheats. Need to fix motor cooling (or add a sensor). Might be good to figure out some sort of 250ms break/reset every so often, turn off motor and back on where it was.

    ~ Add in old algorithm stuff so I can switch between the two and decide if I can make PID better






