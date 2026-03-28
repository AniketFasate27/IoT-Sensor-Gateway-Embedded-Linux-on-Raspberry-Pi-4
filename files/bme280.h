#ifndef BME280_H
#define BME280_H

#include <stdint.h>

typedef struct {
    double temperature_c;
    double humidity_pct;
    double pressure_hpa;
} bme280_data_t;

typedef struct {
    int    fd;
    /* Trimming compensation registers (from NVM) */
    uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
    uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3;
    int16_t  dig_P4; int16_t dig_P5; int16_t dig_P6;
    int16_t  dig_P7; int16_t dig_P8; int16_t dig_P9;
    uint8_t  dig_H1; int16_t dig_H2; uint8_t dig_H3;
    int16_t  dig_H4; int16_t dig_H5; int8_t  dig_H6;
} bme280_dev_t;

int  bme280_init(bme280_dev_t *dev, const char *i2c_bus, uint8_t addr);
int  bme280_read(bme280_dev_t *dev, bme280_data_t *data);
void bme280_close(bme280_dev_t *dev);

#endif
