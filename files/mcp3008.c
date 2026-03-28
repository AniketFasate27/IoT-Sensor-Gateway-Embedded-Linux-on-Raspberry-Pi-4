#include "mcp3008.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

int mcp3008_init(mcp3008_dev_t *dev, const char *spi_dev, uint32_t speed) {
    uint8_t mode  = SPI_MODE_0;
    uint8_t bits  = 8;

    dev->fd = open(spi_dev, O_RDWR);
    if (dev->fd < 0) { perror("mcp3008: open"); return -1; }

    ioctl(dev->fd, SPI_IOC_WR_MODE,          &mode);
    ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ,  &speed);

    dev->speed_hz = speed;
    return 0;
}

int mcp3008_read_channel(mcp3008_dev_t *dev, uint8_t ch, uint16_t *val) {
    if (ch > 7) return -1;

    uint8_t tx[3] = { 0x01, (0x08 | ch) << 4, 0x00 };
    uint8_t rx[3] = { 0 };

    struct spi_ioc_transfer tr = {
        .tx_buf        = (unsigned long)tx,
        .rx_buf        = (unsigned long)rx,
        .len           = 3,
        .speed_hz      = dev->speed_hz,
        .bits_per_word = 8,
        .cs_change     = 0,
    };

    if (ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("mcp3008: spi transfer"); return -1;
    }

    *val = ((rx[1] & 0x03) << 8) | rx[2];
    return 0;
}

double mcp3008_to_voltage(uint16_t raw, double vref) {
    return (raw / 1023.0) * vref;
}

void mcp3008_close(mcp3008_dev_t *dev) {
    if (dev->fd >= 0) close(dev->fd);
}
