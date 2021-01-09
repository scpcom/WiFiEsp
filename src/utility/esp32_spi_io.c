#include <stdlib.h>
#include "esp32_spi_io.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"

#include <utils.h>
#include "spi.h"

#include "fpioa.h"

#define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
#define GPIOHS_OUT_LOWX(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))

#define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)

static uint8_t _mosi_num = -1;
static uint8_t _miso_num = -1;
static uint8_t _sclk_num = -1;

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
    gpiohs_set_drive_mode(sclk, GPIO_DM_OUTPUT);
    gpiohs_set_pin(sclk, GPIO_PV_LOW);
    //mosi
    gpiohs_set_drive_mode(mosi, GPIO_DM_OUTPUT);
    gpiohs_set_pin(mosi, GPIO_PV_LOW);
    //miso
    gpiohs_set_drive_mode(miso, GPIO_DM_INPUT_PULL_UP);

    _mosi_num = mosi;
    _miso_num = miso;
    _sclk_num = sclk;
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
            GPIOHS_OUT_HIGH(_mosi_num);
        }
        else
        {
            GPIOHS_OUT_LOWX(_mosi_num);
        }
        data <<= 1;
        GPIOHS_OUT_HIGH(_sclk_num);

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");

        temp <<= 1;
        if (GET_GPIOHS_VALX(_miso_num))
        {
            temp++;
        }
        GPIOHS_OUT_LOWX(_sclk_num);

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


int gpiohs_register(int8_t fpio_pin)
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



uint64_t get_millis(void)
{
    return sysctl_get_time_us() / 1000;
}
