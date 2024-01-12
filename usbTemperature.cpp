#include "my_serial.hpp"
#include <sstream>              // std::stringstream
#include <iostream>             // std::cout
#include <chrono>

#if !defined (WIN32)
#	include <unistd.h>          // pause()
#	include <time.h>            // nanosleep()
#endif

#define MAINLOGFILE "mainlog.txt"
#define HOURLOGFILE "hourlog.txt"
#define DAYLOGFILE  "daylog.txt"


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

bool isIntNumber(std::string& str) {
	if (str.length() == 0) return false;
	for (int i = 0; i < str.length(); i++) {
		if (isdigit(str[i]) == false)
    		return false;
	}
	return true;
}


void writeLog(const std::string& fileName, const std::string& text) {
		FILE* logFile = fopen(fileName.c_str(), "a");

		if (logFile) {
			fprintf(logFile, "%s\n", text.data());
			fflush(logFile);
			fclose(logFile);
		} else {
			std::cerr << "Unable to open log file" << std::endl;
    	}
}

void clearLog(const std::string& fileName) {
	fopen(fileName.c_str(), "w");
}

time_t getCurrentTime() {
	auto curTime = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(curTime);
}

void checkLogsTime(TemperatureStorage& mstorage,
  				   TemperatureStorage& hstorage,
   				   TemperatureStorage& dstorage) {
	auto currentTime = getCurrentTime();
	if(difftime(currentTime, mstorage.startTime)/60/60 >= 24.0) {
		clearLog(MAINLOGFILE);
		mstorage.startTime = currentTime;
	}

	if(difftime(currentTime, hstorage.startTime)/60/60 >= 24.0*30) {
		clearLog(HOURLOGFILE);
		hstorage.startTime = currentTime;
	}

	if(difftime(currentTime, dstorage.startTime)/60/60 >= 24.0*365) {
		clearLog(DAYLOGFILE);
		dstorage.startTime = currentTime;
	}

}

void csleep(double timeout) {
#if defined (WIN32)
	if (timeout <= 0.0)
        ::Sleep(INFINITE);
    else
        ::Sleep((DWORD)(timeout * 1e3));
#else
    if (timeout <= 0.0)
        pause();
    else {
        struct timespec t;
        t.tv_sec = (int)timeout;
        t.tv_nsec = (int)((timeout - t.tv_sec)*1e9);
        nanosleep(&t, NULL);
    }
#endif
}

int generateTemperature() {
	return rand() % 20 + 20;
}

int main(int argc, char** argv)
{
	TemperatureStorage mainStorage = TemperatureStorage(getCurrentTime());
	TemperatureStorage hourStorage = TemperatureStorage(getCurrentTime());
	TemperatureStorage dayStorage = TemperatureStorage(getCurrentTime());

	if (argc < 3) {
		std::cout << "Usage: sertest [port] [regime], where [regime] can be 'read' or 'write'" << std::endl;
		return -1;
	}
	bool read = true;
	if (!strcmp(argv[2],"write"))
		read = false;
	
	cplib::SerialPort smport(std::string(argv[1]),cplib::SerialPort::BAUDRATE_115200);
	if (!smport.IsOpen()) {
		std::cout << "Failed to open port '" << argv[1] << "'! Terminating..." << std::endl;
		return -2;
	}

	std::string mystr;
	if (read) {
		smport.SetTimeout(1.0);
		for (;;) {
			smport >> mystr;
			if (!mystr.empty() && isIntNumber(mystr)) {
				hourStorage.addTemperature(std::stoi(mystr));
				dayStorage.addTemperature(std::stoi(mystr));
				writeLog(MAINLOGFILE, "Temperature: " + mystr);

				if (difftime(getCurrentTime(), hourStorage.writedTime)/60/60 >= 1.0) {
					writeLog(HOURLOGFILE, "Average hour temperature: " + std::to_string(hourStorage.getAverageTemp()));
					hourStorage.writedTime = getCurrentTime();
					hourStorage.clearData();
				}

				if (difftime(getCurrentTime(), dayStorage.writedTime)/60/60 >= 24.0) {
					writeLog(DAYLOGFILE, "Average day temperature: " + std::to_string(dayStorage.getAverageTemp()));
					dayStorage.writedTime = getCurrentTime();
					dayStorage.clearData();
				}

				checkLogsTime(mainStorage, hourStorage, dayStorage);
			}
		}
	} else {
		for (int i = 0;;i++) {
			mystr = std::to_string(generateTemperature());
			smport << mystr;
			csleep(1.0);
		}
	}
    return 0;
}