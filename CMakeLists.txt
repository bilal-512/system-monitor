cmake_minimum_required(VERSION 3.10)
project(sysmon_cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/Config.cpp
    src/Logger.cpp
    src/SystemInfo.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/DiskMonitor.cpp
    src/ProcessManager.cpp
    src/AlertSystem.cpp
    src/ReportGenerator.cpp
    src/PerformanceStats.cpp
    src/Display.cpp
)

add_executable(sysmon ${SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(sysmon Threads::Threads)

if(WIN32)
    # psapi  – GetProcessMemoryInfo
    # advapi32 – RegOpenKeyEx (CPU model from registry)
    target_link_libraries(sysmon psapi advapi32)
endif()

# Copy config directory next to the binary so the app can find config.ini
add_custom_command(TARGET sysmon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:sysmon>/config"
    COMMENT "Copying config directory to build output"
)

install(TARGETS sysmon DESTINATION bin)
install(DIRECTORY config/   DESTINATION bin/config)
install(DIRECTORY reports/  DESTINATION bin/reports)
