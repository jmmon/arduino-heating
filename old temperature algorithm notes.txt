Heat_4.0.0

Old radiant heating temperature reading/control:::

Every 2000ms (2 seconds): checkTemperature

    And:

Every 10 seconds, adjust trend
    if current EMA < medium EMA (or long EMA...) do trend--
    else if current EMA > medium EMA (or long EMA...) do trend++
    
    Then, limit the trend to +/- 16

    So: 10s * 16 = 160s = 2m40s. So if the temp is warming up for 2m40s, the pump will shut off 0.32* cooler than set temperature


And how the trend is used:
    We want the pump OFF if:
        temperature is reached,
	OR
	trend > 0 (getting warmer) && temperature > (setpoint - trendOffset), where trendOffset == 0.02 * avgTrend

	So 0.02 * 16 = 0.32 degrees. So if temp gets to (setpoint - 0.32*F) the pump turns off.


4.0.1:
if current EMA < medium EMA (we're dropping temperature):
   if medium EMA (prev EMA) < weighted average temp {
       if (trend < -10) add 10
       else if > -10 set to 0
    } else if (medium EMA >= weighted average temp{
        (last EMA is > average temperature): trend -= 1
    }

if (current EMA > medium/prev EMA (we're rising temperature):
    if prev EMA > weighted average temp{
        if (trend >= 10) subtract 10
	else if (< 10) set to 0
    else if (medium <= weighted average temp) {
        trend + 1
    }


Then cap the trends to min or max : 360

And then:
    if temp reached, or if trending up && temp > (setpoint - 0.003 * trend)
    So 0.003 * 360 = 1.080 degrees 
    So if trending up and temp > setpoint - 1.08, we turn off the pump because we assume it will coast to above the target.

    How often does trend change? 10 seconds
    How long would temp need to be rising to reach 360?? 10s * 360 = 3600s == 1 hour
    If temp rises, then falls, then rises again, what might the trend do?
        Rises to 180 over 1800 seconds, then falls over 600 seconds (60 occurrances) => -= (60 * 10) == -= 600 so back to 0 or goes - 180 to 0 then - 42 * 1 => -42 trend.. Then rises again over 1800 seconds (180 occurances) => 5 brings to 0, then +1 * 175 => ending with 175 trend!


So Maybe it's better to plan it out this way:
    If temp is rising,
        // moving towards 0 goes fast, moving away from 0 goes slow
        if trend is negative, add 10 to the trend
	else add 1 to the trend

    else if temp is falling
        if trend is positive, subtract 10 from the trend
	else subtract 1 from the trend


Or another way:
    If temp is rising, (trend should be increasing)
        If current is a lot higher than the previous, the temp is increasing fast
	    so then if moving towards 0, move trend 8x
	    else if moving away from 0, move trend 2x
	Else if it's just a little higher, the temp is increasing slower
	    So then if moving towards 0, move trend 8x?
	    else if moving away from 0, move trend 1x

    If temp is falling (trend should be decreasing)
        If current is a lot lower than the previous, the temp is decreasing fast
	    So if moving towards 0, move trend 8x
	    Else move trend 2x
	Else if it's a little lower than previous, it's decreasing slowly
	    So if moving towards 0, move trend 8x?
	    else if moving away from 0, move trend 1x





So: I need to take into account the heat of the floor (the heat releasing over time).
It should also depend on the temperature trend: if the trend is slow, the floor 'battery' should be considered to not last as long (or not give enough 'push' up on the temperature)

 i.e. if floor is warm, and temp rises really quickly, that means we need to cut the pump off early.
    But if temp rose slowly, we might need to cut the pump off only a little before.

    And/or maybe some ratio of floor reading : reduction in target temperature
    maybe range ?? 500-650 => 0-3 deg F? Linear relationship, if floor is 650+ then we need to only reach setpoint - 3 and then the pump switches to off. This means that 
        1. I need to make sure the pump doesn't turn on if dropping 3 deg; I think I have a setting there for emergency on temperature drop
	2. as the floor cools, the "feels like" temperature will drop so the pump will want to turn back on after the off cycle timer ends.
	    Need to see how it goes and adjust if it just cycles on and off (3m and 30m) repeatedly

	








Jan 18 2023:

Should implement the temperature trend (or adjust it):
		As the temperature drops, if the trend is slow-dropping, 
				the setpoint should be lowered (turns on later)
		As the temperature drops, if the trend is fast-dropping,
				the setpoint should be raised (turns on sooner)
		
		As the temperature rises, if the trend is slow-rising,
				the setpoint should be raised (stays on longer)
		As the temperature rises, if the trend is fast-rising,
				the setpoint should be lowered (stays on shorter)
				
				
				
				
				
				
				
FEB 7th 2023:
	I ran out of water and the water guage said (310 - 298) and had a perecntage level of 9%.
	This should be adjusted so that this is the 0 mark, because at this level the pump was running continuously as it ran out of water in the tank.