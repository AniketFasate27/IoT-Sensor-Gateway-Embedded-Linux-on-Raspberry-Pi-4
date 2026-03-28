# Enable I2C, SPI, and hardware watchdog in kernel config
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://iot-gateway.cfg"
