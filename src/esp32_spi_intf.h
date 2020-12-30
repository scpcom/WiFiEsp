#ifndef __ESP32_SPI_INTF_H
#define __ESP32_SPI_INTF_H

#include "wiring_digital.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "utility/esp32_spi.h"

#ifdef K210
#ifndef WIFI_CS
#define WIFI_CS   SPI0_CS1
#endif
#ifndef WIFI_MISO
#define WIFI_MISO SPI0_MISO
#define WIFI_SCLK SPI0_SCLK
#define WIFI_MOSI SPI0_MOSI
#endif
#ifndef WIFI_RDY
#define WIFI_RDY ORG_PIN_MAP(9)
#define WIFI_RST ORG_PIN_MAP(8)
#define WIFI_RX  ORG_PIN_MAP(7)
#define WIFI_TX  ORG_PIN_MAP(6)
#endif
#endif

bool esp32_spi_init_soft(int cs, int rst, int rdy, int mosi, int miso, int sclk);
bool esp32_spi_init_hard(int cs, int rst, int rdy, int spi);

#ifdef __cplusplus
}
#endif

#endif
