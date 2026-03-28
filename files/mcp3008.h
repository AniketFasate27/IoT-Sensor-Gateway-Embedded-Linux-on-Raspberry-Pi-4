#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>

#define MCP3008_CHANNELS 8

typedef struct {
    int fd;
    uint32_t speed_hz;
} mcp3008_dev_t;

int     mcp3008_init(mcp3008_dev_t *dev, const char *spi_dev, uint32_t speed);
int     mcp3008_read_channel(mcp3008_dev_t *dev, uint8_t ch, uint16_t *val);
double  mcp3008_to_voltage(uint16_t raw, double vref);
void    mcp3008_close(mcp3008_dev_t *dev);

#endif
