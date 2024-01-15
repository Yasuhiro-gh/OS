#include "db_manager.h"


DatabaseManager::DatabaseManager(const char* dbName) {
    if (sqlite3_open(dbName, &db) != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }
    std::cout << "Database " << dbName << " is open" << std::endl;
}

DatabaseManager::~DatabaseManager() {
    sqlite3_close(db);
}

void DatabaseManager::createTables() {
    const char* createTableQuery =
        "CREATE TABLE IF NOT EXISTS main_temperature_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "temperature REAL NOT NULL);";

    if (sqlite3_exec(db, createTableQuery, 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Error creating table: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }

    createTableQuery =
        "CREATE TABLE IF NOT EXISTS hour_temperature_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "temperature REAL NOT NULL);";

    if (sqlite3_exec(db, createTableQuery, 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Error creating table: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }

    createTableQuery =
        "CREATE TABLE IF NOT EXISTS day_temperature_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "temperature REAL NOT NULL);";

    if (sqlite3_exec(db, createTableQuery, 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Error creating table: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }
}

void DatabaseManager::writeToDatabase(const int& temperature, const std::string& tableName) {
    std::string insertQuery = "INSERT INTO " + tableName + " temperature "
                              "VALUES (?, ?);";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, temperature);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error inserting data into database: " << sqlite3_errmsg(db) << std::endl;
            exit(-1);
        }

        sqlite3_finalize(stmt);
    }
    else {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }
}

void DatabaseManager::clearTable(const std::string& tableName) {
    const std::string delquery = "DELETE FROM " + tableName;

    if (sqlite3_exec(db, delquery.c_str(), 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Error clearing table: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }

    const char* vacquery = "VACUUM";

    if (sqlite3_exec(db, vacquery, 0, 0, 0) != SQLITE_OK) {
        std::cerr << "Error clearing table: " << sqlite3_errmsg(db) << std::endl;
        exit(-1);
    }
}

int DatabaseManager::getCurrentTemperature() {
    const char* query = "SELECT temperature "
                        "FROM main_temperature_data " 
                        "ORDER BY id DESC "
                        "LIMIT 1;";

    bool dbIsNull = db == nullptr;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            float currentTemperature = static_cast<float>(sqlite3_column_double(stmt, 0));
            sqlite3_finalize(stmt);
            return currentTemperature;
        }
        sqlite3_finalize(stmt);
        
    } else {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
    }

    return 0.0f;
}

std::pair<int, std::vector<int>> DatabaseManager::getTemperatureDataFromLastDays(int days) {
    const char* query = "SELECT temperature "
                        "FROM day_temperature_data "
                        "LIMIT ?;";
    std::vector<int> loggedDays;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        int totalTemperature = 0;
        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int temperature = static_cast<int>(sqlite3_column_int(stmt, 0));
            totalTemperature += temperature;
            loggedDays.push_back(count++);
        }
        sqlite3_finalize(stmt);
        int averageTemperature = (count > 0) ? (totalTemperature / count) : 0;
        return {averageTemperature, loggedDays};
    } else {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
    }
    return {0, {}};
}