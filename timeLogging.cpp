#include "my_shmem.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <time.h>


int getProcessId();
std::string getCurrentTime();

std::mutex mtx;

#define EXE_NAME "timeLogging.exe"
#define MEMO_TEXT "Init memo"
struct my_data
{
	my_data() {
		memset(memo,0,sizeof(memo));
        memcpy(memo,MEMO_TEXT,strlen(MEMO_TEXT));
		counter = 0;
		mainProcessid = getProcessId();

		writeLog(
			"Process ID: " + std::to_string(getProcessId()) +
			", Start time: " + getCurrentTime()
		);
	}
	void writeLog(const std::string& text) {
		std::lock_guard<std::mutex> lock(mtx);
		FILE* logFile = fopen("logfile.txt", "a");

		if (logFile) {
			fprintf(logFile, "%s\n", text.data());
			fflush(logFile);
			fclose(logFile);
		} else {
			std::cerr << "Unable to open log file" << std::endl;
    	}
	}
	char        memo[256];
	int         counter;
	int 		mainProcessid;
};

cplib::SharedMem<my_data> shared_data("mymemo");

int getProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

std::string getCurrentTime() {
	struct tm *u;
	char s1[40] = { 0 }, s2[40] = { 0 };
	const time_t timer = time(NULL);
	u = localtime(&timer);
	strftime(s1, 80, "%d.%m.%Y %H:%M:%S", u);
	auto now = std::chrono::system_clock::now();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 100000;
	return std::string(s1) + "." + std::to_string(milliseconds);
}

void mainThread() {
	while(true) {
		shared_data.Lock();
		shared_data.Data()->writeLog(
			"Current time: " + getCurrentTime() +
			", Process ID: " + std::to_string(getProcessId()) +
 			", Counter: " + std::to_string(shared_data.Data()->counter)
		);
		shared_data.Unlock();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void writerThread() {
	while(true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		shared_data.Lock();
		shared_data.Data()->counter++;
		shared_data.Unlock();
	}
}

void logFirstCopy() {
	shared_data.Lock();
	shared_data.Data()->writeLog(
		"Process ID: " + std::to_string(getProcessId()) +
		", Copy №1 start time: " + getCurrentTime()
	);
	shared_data.Data()->counter += 10;
	shared_data.Data()->writeLog("Copy №1 exiting time: " + getCurrentTime());
	shared_data.Unlock();
}

void logSecondCopy() {
	shared_data.Lock();
	shared_data.Data()->writeLog(
		"Process ID: " + std::to_string(getProcessId()) +
		", Copy №2 start time: " + getCurrentTime()
	);
	shared_data.Data()->counter *= 2;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	shared_data.Data()->counter /=2;
	shared_data.Data()->writeLog("Copy №2 exiting time: " + getCurrentTime());
	shared_data.Unlock();
}

void makeCopy(const std::string& copyName) {
#ifdef _WIN32
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
	ZeroMemory( &pi, sizeof(pi) );

	if (CreateProcess(EXE_NAME.c_str(),
                      copyName.c_str(),
                      NULL,
                      NULL,
                      FALSE,
                      0, 
                      NULL, 
                      NULL,
                      &si, 
                      &pi)
    ) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
	}
	else {
		shared_data.Lock();
		shared_data.Data()->writeLog("Error! Previous copy is still running.");
		shared_data.Unlock();
	}
#else
    pid_t pid = fork();

    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        return;
    }

    if (pid == 0) {
        system((EXE_NAME + " " + copyName).c_str());
        exit(EXIT_SUCCESS);
    }
#endif
}

void copyThreadMaker() {
#ifdef _WIN32
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	STARTUPINFO si2;
	PROCESS_INFORMATION pi2;
	
	ZeroMemory( &si, sizeof(si) );
	ZeroMemory( &pi, sizeof(pi) );
	ZeroMemory( &si2, sizeof(si2) );
	ZeroMemory( &pi2, sizeof(pi2) );
#endif
	while(true) {
		makeCopy("1");
		makeCopy("2");
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
}

bool isIntNumber(std::string& str) {
	if (str.length() == 0) return false;
	for (int i = 0; i < str.length(); i++) {
		if (isdigit(str[i]) == false)
    		return false;
	}
	return true;
}

int main(int argc, char *argv[]) {
    if (!shared_data.IsValid()) {
        std::cout << "Failed to create shared memory block!" << std::endl;
        return -1;
    }

	if (argc > 1) {
		if (strcmp(argv[1], "1") == 0) {
			logFirstCopy();
			return 0;
		}
		if (strcmp(argv[1], "2") == 0) {
			logSecondCopy();
			return 0;
		}
	}


	std::thread mthread;
	std::thread cthread;

	if (shared_data.Data()->mainProcessid == getProcessId()) {
		cthread = std::thread(copyThreadMaker);
		mthread = std::thread(mainThread);
	}

	std::thread wthread(writerThread);

	std::string userCounterVal;
	while(true) {
		std::cin >> userCounterVal;
		if (!isIntNumber(userCounterVal)) {
			std::cout << "Please, enter an integer number!" << std::endl;
		}
		else {
			shared_data.Lock();
			shared_data.Data()->counter = std::stoi(userCounterVal);
			shared_data.Unlock();
		}
	}

	mthread.join();
	wthread.join();
	cthread.join();

	return 0;
}