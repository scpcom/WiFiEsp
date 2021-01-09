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


static spi_transfer_width_t sipeed_spi_get_frame_size(size_t data_bit_length)
{
    if (data_bit_length < 8)
        return SPI_TRANS_CHAR;
    else if (data_bit_length < 16)
        return SPI_TRANS_SHORT;
    return SPI_TRANS_INT;
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
        _spi_num = SPI_DEVICE_1;
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
    _chip_select = SPI_CHIP_SELECT_0;
    return true;
}

static void hard_spi_set_tmod(uint8_t spi_num, uint32_t tmod)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    volatile spi_t *spi_handle = spi[spi_num];
    uint8_t tmod_offset = 0;
    switch (spi_num)
    {
    case 0:
    case 1:
        tmod_offset = 8;
        break;
    case 2:
        configASSERT(!"Spi Bus 2 Not Support!");
        break;
    case 3:
    default:
        tmod_offset = 10;
        break;
    }
    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}

static void hard_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *tx_buff, size_t tx_len, uint8_t *rx_buff, size_t rx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    configASSERT(tx_len > 0);
    size_t index, fifo_len;
    // size_t rx_len = tx_len;
    // size_t tx_len = rx_len;
    hard_spi_set_tmod(spi_num, SPI_TMOD_TRANS_RECV);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8_t dfs_offset;
    switch (spi_num)
    {
    case 0:
    case 1:
        dfs_offset = 16;
        break;
    case 2:
        configASSERT(!"Spi Bus 2 Not Support!");
        break;
    case 3:
    default:
        dfs_offset = 0;
        break;
    }
    uint32_t data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = sipeed_spi_get_frame_size(data_bit_length);
    spi_handle->ctrlr1 = (uint32_t)(tx_len / frame_width - 1);
    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t i = 0;
    while (tx_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        switch (frame_width)
        {
        case SPI_TRANS_INT:
            fifo_len = fifo_len / 4 * 4;
            for (index = 0; index < fifo_len / 4; index++)
                spi_handle->dr[0] = ((uint32_t *)tx_buff)[i++];
            break;
        case SPI_TRANS_SHORT:
            fifo_len = fifo_len / 2 * 2;
            for (index = 0; index < fifo_len / 2; index++)
                spi_handle->dr[0] = ((uint16_t *)tx_buff)[i++];
            break;
        default:
            for (index = 0; index < fifo_len; index++)
                spi_handle->dr[0] = tx_buff[i++];
            break;
        }
        tx_len -= fifo_len;
    }

    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    i = 0;
    while (rx_len)
    {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        switch (frame_width)
        {
        case SPI_TRANS_INT:
            fifo_len = fifo_len / 4 * 4;
            for (index = 0; index < fifo_len / 4; index++)
                ((uint32_t *)rx_buff)[i++] = spi_handle->dr[0];
            break;
        case SPI_TRANS_SHORT:
            fifo_len = fifo_len / 2 * 2;
            for (index = 0; index < fifo_len / 2; index++)
                ((uint16_t *)rx_buff)[i++] = (uint16_t)spi_handle->dr[0];
            break;
        default:
            for (index = 0; index < fifo_len; index++)
                rx_buff[i++] = (uint8_t)spi_handle->dr[0];
            break;
        }

        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
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
    hard_spi_transfer_data_standard(_spi_num, _chip_select, &data, 1, &c, 1);
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
        hard_spi_transfer_data_standard(_spi_num, _chip_select, send, len, recv, len);
        return;
    }
    return;
}



uint64_t get_millis(void)
{
    return sysctl_get_time_us() / 1000;
}
