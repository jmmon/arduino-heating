/* ==========================================================================
 * AirSensor class
 * ========================================================================== */
class Air_C {
private:
	DHT *sensor;

public:
	float tempC = 0;
	float tempF = 0;
	float WEIGHT = 1;
	float humid = 0;

	float highest = -200;
	float lowest = 200;
	float currentEMA[3]; // {short, medium, long} EMA
	float lastEMA[3];		 // {short, medium, long}

	bool working = true;

  /**
   * Constructor fn
   * @param {DHT} *sensor - DHT sensor
   * @param {float} weight - weight
   * */
	Air_C(DHT *sensor, float weight) {
		sensor = sensor;
		WEIGHT = weight;
	}

  /**
   * Read this temperature and save to state
   * */
	void readTemp()	{
		// read temp/humid
		tempC = sensor->readTemperature();
		tempF = sensor->readTemperature(true);
		humid = sensor->readHumidity();
	};

  /**
   * get current temp, weighted if necessary
   * @param {bool} weighted - weight the EMA when returning
   * */
	float getTempEma(bool weighted = false)	{
		return currentEMA[0] * (weighted ? WEIGHT : 1);
	}

  /**
   * update high/low records and calculate the EMAs
   * runs every 2.5s
   * */
	void updateRecords()	{
		// update highest / lowest
		if (highest < tempF)
			highest = tempF;

		if (lowest > tempF)
			lowest = tempF;

		// update EMAs (short, medium, long)
		for (uint8_t i = 0; i < 3; i++) { 
      // save as lastEMA before updating the currentEMA
			lastEMA[i] = lastEMA[i] == 0 ? tempF : currentEMA[i];
			// currentEMA[i] = calcEMAFromFloats(tempF, lastEMA[i], EMA_MULT_PERIODS[i]);
			currentEMA[i] = calcEma(tempF, lastEMA[i], EMA_MULT_PERIODS[i]);
		}
	};

} air[] = {
		Air_C(&dht[0], 1), //"MAIN"
		Air_C(&dht[1], 2), //"UPSTAIRS"
		Air_C(&dht[2], 2), //"OUTSIDE"
		Air_C(&dht[3], 2), //"GREENHOUSE"
};
