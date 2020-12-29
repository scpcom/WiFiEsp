#ifndef WiFiEspDrv_h
#define WiFiEspDrv_h

#define WIFI_ESP32_SPI

#ifdef WIFI_ESP32_SPI
#include "EspSpiDrv.h"

#define WIFIDRV EspSpiDrv

#else
#include "EspDrv.h"

#define WIFIDRV EspDrv
#endif

#endif
