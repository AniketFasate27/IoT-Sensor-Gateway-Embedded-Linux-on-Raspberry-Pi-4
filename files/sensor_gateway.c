/*
 * sensor_gateway.c
 * IoT Sensor Gateway — reads BME280 (I2C) + MCP3008 (SPI),
 * publishes JSON telemetry to AWS IoT Core via MQTT/TLS.
 *
 * Build: cmake .. -DDEVICE_ID="rpi4-gw-001"
 */

#include "config.h"
#include "bme280.h"
#include "mcp3008.h"
#include "mqtt_client.h"
#include "cjson/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <syslog.h>

/* ── Globals ── */
static volatile sig_atomic_t g_running = 1;
static int  g_wdt_fd = -1;

/* ── Signal handler ── */
static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
    syslog(LOG_INFO, "sensor-gateway: shutdown requested");
}

/* ── Watchdog ── */
static int wdt_open(void) {
    int fd = open(WATCHDOG_DEV, O_RDWR);
    if (fd < 0) { perror("watchdog: open"); return -1; }
    int timeout = WATCHDOG_TIMEOUT;
    ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
    syslog(LOG_INFO, "watchdog: armed, timeout=%ds", timeout);
    return fd;
}

static void wdt_kick(int fd) {
    if (fd >= 0) write(fd, "\0", 1);
}

static void wdt_close(int fd) {
    if (fd >= 0) {
        /* Magic close disarms hardware watchdog */
        write(fd, "V", 1);
        close(fd);
    }
}

/* ── ISO-8601 timestamp ── */
static void iso8601_now(char *buf, size_t len) {
    time_t  t  = time(NULL);
    struct tm *tm = gmtime(&t);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", tm);
}

/* ── Build JSON telemetry payload ── */
static char *build_payload(const bme280_data_t *env,
                            const uint16_t *adc, int n_ch,
                            const char *device_id) {
    cJSON *root    = cJSON_CreateObject();
    cJSON *env_obj = cJSON_CreateObject();
    cJSON *adc_arr = cJSON_CreateArray();
    char   ts[32];
    iso8601_now(ts, sizeof(ts));

    cJSON_AddStringToObject(root, "device_id",  device_id);
    cJSON_AddStringToObject(root, "timestamp",  ts);
    cJSON_AddStringToObject(root, "fw_version", "1.0.0");

    cJSON_AddNumberToObject(env_obj, "temperature_c",  env->temperature_c);
    cJSON_AddNumberToObject(env_obj, "humidity_pct",   env->humidity_pct);
    cJSON_AddNumberToObject(env_obj, "pressure_hpa",   env->pressure_hpa);
    cJSON_AddItemToObject(root, "environment", env_obj);

    for (int i = 0; i < n_ch; i++) {
        cJSON *ch = cJSON_CreateObject();
        cJSON_AddNumberToObject(ch, "channel", i);
        cJSON_AddNumberToObject(ch, "raw",     adc[i]);
        cJSON_AddNumberToObject(ch, "voltage", mcp3008_to_voltage(adc[i], 3.3));
        cJSON_AddItemToArray(adc_arr, ch);
    }
    cJSON_AddItemToObject(root, "adc_channels", adc_arr);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out; /* caller must free() */
}

/* ── Main ── */
int main(void) {
    bme280_dev_t  bme = {0};
    mcp3008_dev_t mcp = {0};
    mqtt_ctx_t    mqtt = {0};
    bme280_data_t env;
    uint16_t      adc[MCP3008_CHANNELS];
    int           retries = 0;

    openlog("sensor-gateway", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "sensor-gateway starting, device_id=%s", MQTT_CLIENT_ID);

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    /* Watchdog */
    g_wdt_fd = wdt_open();

    /* Init sensors */
    if (bme280_init(&bme, I2C_BUS, BME280_ADDR) < 0) {
        syslog(LOG_ERR, "bme280 init failed"); return EXIT_FAILURE;
    }
    if (mcp3008_init(&mcp, SPI_DEV, SPI_SPEED_HZ) < 0) {
        syslog(LOG_ERR, "mcp3008 init failed"); return EXIT_FAILURE;
    }

    /* Init MQTT */
    if (mqtt_init(&mqtt, MQTT_CLIENT_ID) < 0) {
        syslog(LOG_ERR, "mqtt init failed"); return EXIT_FAILURE;
    }

    /* ── Main publish loop ── */
    while (g_running) {
        /* Reconnect if needed */
        if (!mqtt_is_connected(&mqtt)) {
            if (retries >= MAX_RECONNECT_RETRIES) {
                syslog(LOG_CRIT, "max reconnect retries reached, exiting");
                break;
            }
            syslog(LOG_WARNING, "mqtt: reconnecting (attempt %d)...", ++retries);
            if (mqtt_connect(&mqtt) == 0) {
                retries = 0;
            } else {
                sleep(RECONNECT_DELAY_SEC);
                continue;
            }
        }

        /* Read sensors */
        if (bme280_read(&bme, &env) < 0) {
            syslog(LOG_WARNING, "bme280 read error, skipping cycle");
            goto next_cycle;
        }
        for (int ch = 0; ch < MCP3008_CHANNELS; ch++) {
            if (mcp3008_read_channel(&mcp, ch, &adc[ch]) < 0) {
                syslog(LOG_WARNING, "mcp3008 ch%d read error", ch);
                adc[ch] = 0xFFFF; /* sentinel for error */
            }
        }

        /* Build & publish JSON */
        char *payload = build_payload(&env, adc, MCP3008_CHANNELS, MQTT_CLIENT_ID);
        if (payload) {
            if (mqtt_publish(&mqtt, payload) == 0) {
                syslog(LOG_DEBUG, "published: %.80s...", payload);
            } else {
                syslog(LOG_WARNING, "publish failed");
            }
            free(payload);
        }

next_cycle:
        wdt_kick(g_wdt_fd);
        sleep(PUBLISH_INTERVAL_SEC);
    }

    /* Graceful shutdown */
    mqtt_disconnect(&mqtt);
    bme280_close(&bme);
    mcp3008_close(&mcp);
    wdt_close(g_wdt_fd);
    syslog(LOG_INFO, "sensor-gateway stopped cleanly");
    closelog();
    return EXIT_SUCCESS;
}
