cmake_minimum_required(VERSION 3.16)
project(sensor-gateway C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Device ID injected by BitBake via EXTRA_OECMAKE
if(NOT DEFINED DEVICE_ID)
    set(DEVICE_ID "rpi4-dev-001")
endif()
add_definitions(-DDEVICE_ID="${DEVICE_ID}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PAHO     REQUIRED paho-mqtt3cs)
pkg_check_modules(OPENSSL  REQUIRED openssl)
pkg_check_modules(CJSON    REQUIRED libcjson)

add_executable(sensor-gateway
    sensor_gateway.c
    bme280.c
    mcp3008.c
    mqtt_client.c
)

target_include_directories(sensor-gateway PRIVATE
    ${PAHO_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
)

target_link_libraries(sensor-gateway
    ${PAHO_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CJSON_LIBRARIES}
)

install(TARGETS sensor-gateway DESTINATION ${CMAKE_INSTALL_BINDIR})
