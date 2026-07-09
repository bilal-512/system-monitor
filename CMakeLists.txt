cmake_minimum_required(VERSION 3.16)
project(sysmon_cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Charts REQUIRED)

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
    
    # Qt UI sources
    src/ui/MainWindow.cpp
    src/ui/ChartWidget.cpp
    src/core/MonitorWorker.cpp
)

# Optional: define a Qt resource file for styling
set(RESOURCES
    src/ui/resources.qrc
)

add_executable(sysmon WIN32 ${SOURCES} ${RESOURCES})

find_package(Threads REQUIRED)
target_link_libraries(sysmon PRIVATE Threads::Threads Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts)

if(WIN32)
    # psapi  – GetProcessMemoryInfo
    # advapi32 – RegOpenKeyEx (CPU model from registry)
    target_link_libraries(sysmon PRIVATE psapi advapi32)
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
