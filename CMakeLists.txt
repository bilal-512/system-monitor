cmake_minimum_required(VERSION 3.10)
project(sysmon_cpp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES src/*.cpp)

add_executable(sysmon ${SOURCES})

install(TARGETS sysmon DESTINATION bin)
