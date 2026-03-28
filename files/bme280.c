#include "bme280.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>

/* BME280 register addresses */
#define REG_CALIB00   0x88
#define REG_CALIB26   0xE1
#define REG_CTRL_HUM  0xF2
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_PRESS_MSB 0xF7
#define REG_ID        0xD0
#define BME280_CHIP_ID 0x60

static int bme280_write_reg(int fd, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return write(fd, buf, 2) == 2 ? 0 : -1;
}

static int bme280_read_regs(int fd, uint8_t reg, uint8_t *buf, size_t len) {
    if (write(fd, &reg, 1) != 1) return -1;
    return read(fd, buf, len) == (ssize_t)len ? 0 : -1;
}

static void bme280_load_calib(bme280_dev_t *dev) {
    uint8_t c[26], h[7];
    bme280_read_regs(dev->fd, REG_CALIB00, c, 26);
    bme280_read_regs(dev->fd, REG_CALIB26, h, 7);

    dev->dig_T1 = (c[1]  << 8) | c[0];
    dev->dig_T2 = (c[3]  << 8) | c[2];
    dev->dig_T3 = (c[5]  << 8) | c[4];
    dev->dig_P1 = (c[7]  << 8) | c[6];
    dev->dig_P2 = (c[9]  << 8) | c[8];
    dev->dig_P3 = (c[11] << 8) | c[10];
    dev->dig_P4 = (c[13] << 8) | c[12];
    dev->dig_P5 = (c[15] << 8) | c[14];
    dev->dig_P6 = (c[17] << 8) | c[16];
    dev->dig_P7 = (c[19] << 8) | c[18];
    dev->dig_P8 = (c[21] << 8) | c[20];
    dev->dig_P9 = (c[23] << 8) | c[22];
    dev->dig_H1 = c[25];
    dev->dig_H2 = (h[1] << 8) | h[0];
    dev->dig_H3 = h[2];
    dev->dig_H4 = (h[3] << 4) | (h[4] & 0x0F);
    dev->dig_H5 = (h[5] << 4) | (h[4] >> 4);
    dev->dig_H6 = (int8_t)h[6];
}

int bme280_init(bme280_dev_t *dev, const char *bus, uint8_t addr) {
    uint8_t chip_id;

    dev->fd = open(bus, O_RDWR);
    if (dev->fd < 0) {
        perror("bme280: open i2c bus");
        return -1;
    }
    if (ioctl(dev->fd, I2C_SLAVE, addr) < 0) {
        perror("bme280: ioctl I2C_SLAVE");
        return -1;
    }
    bme280_read_regs(dev->fd, REG_ID, &chip_id, 1);
    if (chip_id != BME280_CHIP_ID) {
        fprintf(stderr, "bme280: unexpected chip id 0x%02X\n", chip_id);
        return -1;
    }

    bme280_load_calib(dev);

    /* Normal mode: oversampling x1 for T/P/H, t_standby=1000ms */
    bme280_write_reg(dev->fd, REG_CTRL_HUM,  0x01);  /* H oversampling x1  */
    bme280_write_reg(dev->fd, REG_CONFIG,    0xA0);  /* t_sb=1000ms        */
    bme280_write_reg(dev->fd, REG_CTRL_MEAS, 0x27);  /* T/P x1, normal mode*/

    usleep(10000); /* allow first measurement to complete */
    return 0;
}

int bme280_read(bme280_dev_t *dev, bme280_data_t *out) {
    uint8_t raw[8];
    if (bme280_read_regs(dev->fd, REG_PRESS_MSB, raw, 8) < 0) return -1;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  |  (int32_t)raw[7];

    /* Temperature compensation (Bosch datasheet algorithm) */
    int32_t var1, var2, t_fine;
    var1 = (((adc_T >> 3) - ((int32_t)dev->dig_T1 << 1)) * dev->dig_T2) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)dev->dig_T1) *
              ((adc_T >> 4) - (int32_t)dev->dig_T1)) >> 12) * dev->dig_T3) >> 14;
    t_fine = var1 + var2;
    out->temperature_c = ((t_fine * 5 + 128) >> 8) / 100.0;

    /* Pressure compensation */
    int64_t p1, p2;
    p1 = (int64_t)t_fine - 128000;
    p2 = p1 * p1 * dev->dig_P6;
    p2 += (p1 * dev->dig_P5) << 17;
    p2 += (int64_t)dev->dig_P4 << 35;
    p1 = ((p1 * p1 * dev->dig_P3) >> 8) + ((p1 * dev->dig_P2) << 12);
    p1 = ((((int64_t)1 << 47) + p1) * dev->dig_P1) >> 33;
    if (p1 != 0) {
        int64_t p = 1048576 - adc_P;
        p = (((p << 31) - p2) * 3125) / p1;
        p1 = ((int64_t)dev->dig_P9 * (p >> 13) * (p >> 13)) >> 25;
        p2 = ((int64_t)dev->dig_P8 * p) >> 19;
        p = ((p + p1 + p2) >> 8) + ((int64_t)dev->dig_P7 << 4);
        out->pressure_hpa = (double)p / 25600.0;
    } else {
        out->pressure_hpa = 0.0;
    }

    /* Humidity compensation */
    int32_t h = t_fine - 76800;
    h = (((adc_H << 14) - ((int32_t)dev->dig_H4 << 20) -
          (dev->dig_H5 * h)) + 16384) >> 15;
    h = h * ((((((h * dev->dig_H6) >> 10) *
                (((h * dev->dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
             dev->dig_H2 + 8192) >> 14;
    h -= ((((h >> 15) * (h >> 15)) >> 7) * dev->dig_H1) >> 4;
    h  = (h < 0) ? 0 : h;
    h  = (h > 419430400) ? 419430400 : h;
    out->humidity_pct = (double)(h >> 12) / 1024.0;

    return 0;
}

void bme280_close(bme280_dev_t *dev) {
    if (dev->fd >= 0) close(dev->fd);
}
