cmake_minimum_required(VERSION 3.10.0)
project(OOPproject VERSION 0.1.0 LANGUAGES C CXX)

set(SOURCE_FILE main.cpp)
# set(SOURCE_FILE test.cpp)

add_executable(OOPproject ${SOURCE_FILE})

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(OOPproject ${OpenCV_LIBS})

find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Network)
target_link_libraries(OOPproject SFML::Graphics SFML::Audio SFML::Network)
