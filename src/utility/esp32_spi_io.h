#ifndef __ESP32_SPI_IO_H
#define __ESP32_SPI_IO_H

#include <stdbool.h>
#include <stdint.h>

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

void soft_spi_config_io(uint8_t mosi, uint8_t miso, uint8_t sclk);
uint8_t soft_spi_rw(uint8_t data);
void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);

bool hard_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, uint8_t _spiNum);
void hard_spi_config_io();
uint8_t hard_spi_rw(uint8_t data);
void hard_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
