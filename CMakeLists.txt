cmake_minimum_required(VERSION 3.25)

PROJECT(usbTemperature)

find_library(SQLITE3_LIBRARY
    NAMES sqlite3
    PATHS sqlite
    REQUIRED
)
ADD_EXECUTABLE(usbTemperatureWriter my_serial.hpp usbTemperatureWriter.cpp)
ADD_EXECUTABLE(usbTemperatureReader my_serial.hpp temperatureStorage.hpp db_manager.h db_manager.cpp server.h server.cpp usbTemperatureReader.cpp)

target_link_libraries(usbTemperatureReader PRIVATE ${SQLITE3_LIBRARY} ws2_32)