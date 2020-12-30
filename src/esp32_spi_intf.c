#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "utility/esp32_spi.h"
#include "utility/esp32_spi_io.h"
#include "esp32_spi_intf.h"
#include "fpioa.h"

#define wifi_esp32_spi_ValueError(m) { printf(m); return false; }

static bool hard_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, uint8_t _spiNum)
{
    //hardware SPI
    if(_spiNum == 0)
    {
        fpioa_set_function(sck, FUNC_SPI0_SCLK);
        /*if( ss >= 0)
        {
            fpioa_function_t a = (fpioa_function_t)(FUNC_SPI0_SS0+setSs(_spiNum, ss));
            fpioa_set_function(ss, a);
        }*/
        fpioa_set_function(mosi, FUNC_SPI0_D0);
        if(miso>=0)
            fpioa_set_function(miso, FUNC_SPI0_D1);
    }
    else if(_spiNum == 1)
    {
        fpioa_set_function(sck, FUNC_SPI1_SCLK);
        /*if( ss >= 0)
        {
            fpioa_set_function(ss, (fpioa_function_t)(FUNC_SPI1_SS0+setSs(_spiNum, ss)));
        }*/
        fpioa_set_function(mosi, FUNC_SPI1_D0);
        if(miso>=0)
            fpioa_set_function(miso, FUNC_SPI1_D1);
    }
    else
    {
        return false;
    }
    return true;
}

static int gpiohs_register(int8_t fpio_pin)
{
    if ((fpio_pin < 0) || (fpio_pin > 47)) {
       return -1;
    }
    int gpionum = get_gpio(fpio_pin);
    if(gpionum >= 0){
        fpioa_function_t function = FUNC_GPIOHS0 + gpionum;
        fpioa_set_function(fpio_pin, function);
        //gpiohs_set_drive_mode((uint8_t)gpionum, (gpio_drive_mode_t)dwMode);
    }
    return gpionum;
}

static bool esp32_spi_begin(int cs, int rst, int rdy, int mosi, int miso, int sclk, int spi)
{
    //cs
    if (cs == -1 || cs > 31 || cs < 0)
    {
        wifi_esp32_spi_ValueError("gpiohs cs value error!");
    }

    //rst
    if (rst != -1)//no rst we will use soft reset
    {
        if (rst > 31 || rst < 0)
        {
            wifi_esp32_spi_ValueError("gpiohs rst value error!");
        }
    }

    //rdy
    if (rdy == -1 || rdy > 31 || rdy < 0)
    {
        wifi_esp32_spi_ValueError("gpiohs rdy value error!");
    }

    //hard_spi
    if (spi > 0)
    {
        printf("[esp32_spi] use hard spi(%d)\r\n", spi);
	hard_spi_begin(MD_PIN_MAP(WIFI_SCLK), MD_PIN_MAP(WIFI_MISO), MD_PIN_MAP(WIFI_MOSI), -1, spi);
        hard_spi_config_io();
    }
    else
    {
        printf("[esp32_spi] use soft spi\r\n");
        
        //mosi
        if (mosi == -1 || mosi > 31 || mosi < 0)
        {
            wifi_esp32_spi_ValueError("gpiohs mosi value error!");
        }

        //miso
        if (miso == -1 || miso > 31 || miso < 0)
        {
            wifi_esp32_spi_ValueError("gpiohs miso value error!");
        }

        //sclk
        if (sclk == -1 || sclk > 31 || sclk < 0)
        {
            wifi_esp32_spi_ValueError("gpiohs sclk value error!");
        }

        soft_spi_config_io(mosi, miso, sclk);
    }

    esp32_spi_init(gpiohs_register(cs), gpiohs_register(rst), gpiohs_register(rdy), spi > 0);

    return true;
}

bool esp32_spi_init_soft(int cs, int rst, int rdy, int mosi, int miso, int sclk)
{
    return esp32_spi_begin(cs, rst, rdy, mosi, miso, sclk, 0);
}

bool esp32_spi_init_hard(int cs, int rst, int rdy, int spi)
{
    return esp32_spi_begin(cs, rst, rdy, -1, -1, -1, spi);
}
