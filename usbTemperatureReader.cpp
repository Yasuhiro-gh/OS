#include "db_manager.h"
#include "server.h"
#include "my_serial.hpp"
#include "temperatureStorage.hpp"


bool isIntNumber(std::string& str) {
	if (str.length() == 0) return false;
	for (int i = 0; i < str.length(); i++) {
		if (isdigit(str[i]) == false)
    		return false;
	}
	return true;
}

time_t getCurrentTime() {
	auto curTime = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(curTime);
}

void checkLogsTime(TemperatureStorage& mstorage,
  				   TemperatureStorage& hstorage,
   				   TemperatureStorage& dstorage,
                   DatabaseManager& dbManager) {
	auto currentTime = getCurrentTime();
	if(difftime(currentTime, mstorage.startTime)/60/60 >= 24.0) {
		dbManager.clearTable(MAINTABLE);
		mstorage.startTime = currentTime;
	}

	if(difftime(currentTime, hstorage.startTime)/60/60 >= 24.0*30) {
		dbManager.clearTable(HOURTABLE);
		hstorage.startTime = currentTime;
	}

	if(difftime(currentTime, dstorage.startTime)/60/60 >= 24.0*365) {
		dbManager.clearTable(DAYTABLE);
		dstorage.startTime = currentTime;
	}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: reader [COM port] [server_port]" << std::endl;
        return -1;
    }

    std::cout << std::string(argv[1]) << std::endl;

    cplib::SerialPort smport(std::string(argv[1]), cplib::SerialPort::BAUDRATE_115200);
    if (!smport.IsOpen()) {
        std::cout << "Failed to open port '" << argv[1] << "'! Terminating..." << std::endl;
        return -2;
    }

    TemperatureStorage mainStorage = TemperatureStorage(getCurrentTime());
	TemperatureStorage hourStorage = TemperatureStorage(getCurrentTime());
	TemperatureStorage dayStorage = TemperatureStorage(getCurrentTime());

    DatabaseManager dbManager("temperature_data.db");
    dbManager.createTables();

#if defined(WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    TemperatureServer server(&dbManager);
    SOCKET serverSocket = server.Listen(atoi(argv[2]));
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Failed to start server! Terminating..." << std::endl;
        return -3;
    }
    server.StartThread(serverSocket);

    std::string mystr;
    smport.SetTimeout(1.0);
    for (;;) {
        smport >> mystr;
        if (!mystr.empty() && isIntNumber(mystr)) {
            hourStorage.addTemperature(std::stoi(mystr));
            dayStorage.addTemperature(std::stoi(mystr));
            dbManager.writeToDatabase(std::stoi(mystr), MAINTABLE);

            if (difftime(getCurrentTime(), hourStorage.writedTime)/60/60 >= 1.0) {
                dbManager.writeToDatabase(hourStorage.getAverageTemp(), HOURTABLE);
                hourStorage.writedTime = getCurrentTime();
                hourStorage.clearData();
            }

            if (difftime(getCurrentTime(), dayStorage.writedTime)/60/60 >= 24.0) {
                dbManager.writeToDatabase(dayStorage.getAverageTemp(), DAYTABLE);
                dayStorage.writedTime = getCurrentTime();
                dayStorage.clearData();
            }

            checkLogsTime(mainStorage, hourStorage, dayStorage, dbManager);
        }
    }

    return 0;
}