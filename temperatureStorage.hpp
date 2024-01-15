#include <ctime>

struct TemperatureStorage {
	TemperatureStorage(const time_t& stime) {
		temperatureSum = 0;
		counter = 0;
		startTime = stime;
		writedTime = stime;
	}

	void addTemperature(const int& temp) {
		temperatureSum += temp;
		counter++;
	}

	int getAverageTemp() {
		if (counter == 0) return 0;
		return temperatureSum / counter;
	}

	void clearData() {
		temperatureSum = 0;
		counter = 0;
	}

	int temperatureSum;
	int counter;
	time_t startTime;
	time_t writedTime; 
};