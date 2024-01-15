#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include "sqlite/sqlite3.h"

#define MAINTABLE "main_temperature_data"
#define HOURTABLE "hour_temperature_data"
#define DAYTABLE "day_temperature_data"


class DatabaseManager {
public:
    DatabaseManager(const char* dbName);
    ~DatabaseManager();

    void createTables();
    void clearTable(const std::string& tableName);
    void writeToDatabase(const int& temperature, const std::string& tableName);
    int getCurrentTemperature();
    std::pair<int, std::vector<int>> getTemperatureDataFromLastDays(int days);

private:
    sqlite3* db;
};
