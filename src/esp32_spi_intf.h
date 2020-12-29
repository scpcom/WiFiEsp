#ifndef __ESP32_SPI_INTF_H
#define __ESP32_SPI_INTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utility/esp32_spi.h"

bool esp32_spi_init_soft(int cs, int rst, int rdy, int mosi, int miso, int sclk);
bool esp32_spi_init_hard(int cs, int rst, int rdy, int spi);

#ifdef __cplusplus
}
#endif

#endif
