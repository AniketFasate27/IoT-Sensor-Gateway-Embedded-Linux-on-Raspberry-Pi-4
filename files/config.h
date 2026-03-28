#ifndef CONFIG_H
#define CONFIG_H

/* Sensor interfaces */
#define I2C_BUS          "/dev/i2c-1"
#define BME280_ADDR      0x76
#define SPI_DEV          "/dev/spidev0.0"
#define SPI_SPEED_HZ     1350000

/* MQTT / AWS IoT Core */
#define MQTT_HOST        "YOUR_ENDPOINT.iot.us-east-1.amazonaws.com"
#define MQTT_PORT        8883
#define MQTT_CLIENT_ID   DEVICE_ID          /* injected at build time */
#define MQTT_TOPIC_FMT   "gw/%s/telemetry"
#define MQTT_QOS         1
#define MQTT_KEEPALIVE   60

/* TLS certificates (provisioned at image flash time) */
#define TLS_CA_CERT      "/etc/sensor-gateway/certs/AmazonRootCA1.pem"
#define TLS_CLIENT_CERT  "/etc/sensor-gateway/certs/device.crt"
#define TLS_PRIVATE_KEY  "/etc/sensor-gateway/certs/device.key"

/* Timing */
#define PUBLISH_INTERVAL_SEC  10
#define RECONNECT_DELAY_SEC   5
#define MAX_RECONNECT_RETRIES 10

/* Watchdog */
#define WATCHDOG_DEV     "/dev/watchdog"
#define WATCHDOG_TIMEOUT 30

#endif /* CONFIG_H */
