#ifndef WiFiEspDrv_h
#define WiFiEspDrv_h

#ifndef WIFI_ESP_AT
#ifndef WIFI_ESP32_SPI
#ifdef K210
#define WIFI_ESP32_SPI
#else
#define WIFI_ESP_AT
#endif
#endif
#endif

#ifdef WIFI_ESP32_SPI
#include "EspSpiDrv.h"

#define WIFIDRV EspSpiDrv

#else
#include "EspDrv.h"

#define WIFIDRV EspDrv
#endif

#endif
