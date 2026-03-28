SUMMARY = "IoT Sensor Gateway - MQTT publisher for BME280 and MCP3008"
DESCRIPTION = "Reads I2C/SPI sensors and publishes JSON telemetry to AWS IoT Core via MQTT/TLS"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sensor_gateway.c \
           file://bme280.c \
           file://bme280.h \
           file://mcp3008.c \
           file://mcp3008.h \
           file://mqtt_client.c \
           file://mqtt_client.h \
           file://config.h \
           file://CMakeLists.txt"

S = "${WORKDIR}"

DEPENDS = "paho-mqtt-c openssl cjson"
RDEPENDS:${PN} = "paho-mqtt-c openssl cjson"

inherit cmake

# Embed device ID from MACHINE variable
EXTRA_OECMAKE += "-DDEVICE_ID=${MACHINE}"

FILES:${PN} += "${bindir}/sensor-gateway"

# Install AWS IoT certs (provisioned via separate recipe in production)
FILES:${PN} += "${sysconfdir}/sensor-gateway/certs/"
