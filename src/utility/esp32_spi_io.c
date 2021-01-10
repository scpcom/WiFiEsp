#include <stdio.h>
#include <stdlib.h>
#include "esp32_spi_io.h"

#include "spi.h"

#include "fpioa.h"

static uint8_t _mosi_pin = -1;
static uint8_t _miso_pin = -1;
static uint8_t _sclk_pin = -1;

static spi_device_num_t _spi_num;
static spi_chip_select_t _chip_select;

extern int8_t setSs(uint8_t spi, int8_t pin);
extern int8_t getSsByPin(uint8_t spi, int8_t pin);

extern void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, int8_t chip_select, const uint8_t *tx_buff,uint8_t *rx_buff,  size_t len);

/* SPI端口初始化 */
//should check io value
void soft_spi_config_io(uint8_t mosi, uint8_t miso, uint8_t sclk)
{
    //clk
    pinMode(sclk, OUTPUT);
    digitalWrite(sclk, LOW);
    //mosi
    pinMode(mosi, OUTPUT);
    digitalWrite(mosi, LOW);
    //miso
    pinMode(miso, INPUT_PULLUP);

    _mosi_pin = mosi;
    _miso_pin = miso;
    _sclk_pin = sclk;
}

uint8_t soft_spi_rw(uint8_t data)
{
    // uint8_t tmp = data;
    uint8_t i;
    uint8_t temp = 0;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            digitalWrite(_mosi_pin, HIGH);
        }
        else
        {
            digitalWrite(_mosi_pin, LOW);
        }
        data <<= 1;
        digitalWrite(_sclk_pin, HIGH);

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");

        temp <<= 1;
        if (digitalRead(_miso_pin))
        {
            temp++;
        }
        digitalWrite(_sclk_pin, LOW);

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
    }
    // printf("soft_spi_rw %02X:%02X \r\n", tmp, temp);
    return temp;
}

void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len)
{
    // printf("soft_spi_rw_len\r\n");
    // printf("\r\nsend %p %d", send, len);
    // if (send != NULL) {
    //     for (int i = 0; i < len; i++) {
    //         printf(" %02X", send[i]);
    //     }
    // }
    // printf("\r\nrecv %p %d", recv, len);
    // if (recv != NULL) {
    //     for (int i = 0; i < len; i++) {
    //         printf(" %02X", recv[i]);
    //     }
    // }
    // printf("\r\n");

    if (send == NULL && recv == NULL)
    {
        printf(" buffer is null\r\n");
        return;
    }

#if 0
    uint32_t i = 0;
    do
    {
        *(recv + i) = soft_spi_rw(*(send + i));
        i++;
    } while (--len);
#else

    uint32_t i = 0;
    uint8_t *stmp = NULL, sf = 0;

    if (send == NULL)
    {
        stmp = (uint8_t *)malloc(len * sizeof(uint8_t));
        // memset(stmp, 'A', len);
        sf = 1;
    }
    else
        stmp = send;

    if (recv == NULL)
    {
        do
        {
            soft_spi_rw(*(stmp + i));
            i++;
        } while (--len);
    }
    else
    {
        do
        {
            *(recv + i) = soft_spi_rw(*(stmp + i));
            i++;
        } while (--len);
    }

    if (sf)
        free(stmp);
#endif
}


bool hard_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, uint8_t _spiNum)
{
    //hardware SPI
    if(_spiNum == 0)
    {
        _spi_num = SPI_DEVICE_0;
        fpioa_set_function(sck, FUNC_SPI0_SCLK);
        if( ss >= 0)
        {
            fpioa_function_t a = (fpioa_function_t)(FUNC_SPI0_SS0+setSs(_spiNum, ss));
            fpioa_set_function(ss, a);
        }
        fpioa_set_function(mosi, FUNC_SPI0_D0);
        if(miso>=0)
            fpioa_set_function(miso, FUNC_SPI0_D1);
    }
    else if(_spiNum == 1)
    {
        _spi_num = SPI_DEVICE_1;
        fpioa_set_function(sck, FUNC_SPI1_SCLK);
        if( ss >= 0)
        {
            fpioa_set_function(ss, (fpioa_function_t)(FUNC_SPI1_SS0+setSs(_spiNum, ss)));
        }
        fpioa_set_function(mosi, FUNC_SPI1_D0);
        if(miso>=0)
            fpioa_set_function(miso, FUNC_SPI1_D1);
    }
    else
    {
        return false;
    }
    _chip_select = getSsByPin(_spiNum, ss);
    return true;
}

/* SPI端口初始化 */
// void soft_spi_init(void)
void hard_spi_config_io()
{
    printf("hard spi\r\n");
    //init SPI_DEVICE_1
    // spi_init(_spi_num, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    uint32_t ret = spi_set_clk_rate(_spi_num, 1000000 * 9); /*set clk rate*/
    printf("esp32 set hard spi clk:%d\r\n", ret);

    // fpioa_set_function(27, FUNC_SPI1_SCLK);
    // fpioa_set_function(28, FUNC_SPI1_D0);
    // fpioa_set_function(26, FUNC_SPI1_D1);
}

uint8_t hard_spi_rw(uint8_t data)
{
    uint8_t c;
    spi_init(_spi_num, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    sipeed_spi_transfer_data_standard(_spi_num, _chip_select, &data, &c, 1);
    return c;
}

void hard_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len)
{
    // if (send != NULL) {
    //     for (int i = 0; i < len; i++) {
    //         printf("%02X ", send[i]);
    //     }
    // }
    // printf("hard_spi_rw_len\r\n");
    // printf("\r\nsend %p %d", send, len);
    // if (send != NULL) {
    //     for (int i = 0; i < len; i++) {
    //         printf(" %02X", send[i]);
    //     }
    // }
    // printf("\r\nrecv %p %d", recv, len);
    // if (recv != NULL) {
    //     for (int i = 0; i < len; i++) {
    //         printf(" %02X", recv[i]);
    //     }
    // }
    // printf("\r\n");

    // printf("soft_spi_rw_len %p %p %d\r\n", send, recv, len);
    if (send == NULL && recv == NULL)
    {
        printf(" buffer is null\r\n");
        return;
    }

#if 0
    spi_init(_spi_num, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
#endif

    //only send
    if (send && recv == NULL)
    {
        spi_send_data_standard(_spi_num, _chip_select, NULL, 0, send, len);
        return;
    }

    //only recv
    if (send == NULL && recv)
    {
        spi_receive_data_standard(_spi_num, _chip_select, NULL, 0, recv, len);
        return;
    }

    //send and recv
    if (send && recv)
    {
        sipeed_spi_transfer_data_standard(_spi_num, _chip_select, send, recv, len);
        return;
    }
    return;
}
