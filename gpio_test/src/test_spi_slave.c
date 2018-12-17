#include <rtthread.h>
#include "spi_slave.h"

static __attribute__((aligned(32))) rt_uint8_t master_tx[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
};
rt_uint8_t master_rx[ARRAY_SIZE(master_tx)] = {0, };

static __attribute__((aligned(32))) rt_uint8_t slave_tx[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0D,
        0x55, 0xaa, 0xcc, 0xdd, 0xee, 0x12, 0x13, 0x17,
};
static __attribute__((aligned(32))) rt_uint8_t slave_rx[ARRAY_SIZE(slave_tx)] = {0, };

struct rt_spi_device *rt_spi_device;
rt_device_t spi_slave_device;
struct rt_completion test_complete;



void test_cb(void *p_v)
{
    rt_completion_done(&test_complete);
}

static void master_slave_loop_test(void)
{
    int ret;
    int failed_count = 0;
    struct spi_slave_xfer xfer = {
        .rx_buf_addr = (rt_uint32_t)slave_rx,
        .tx_buf_addr = (rt_uint32_t)slave_tx,
        .transfer_length = ARRAY_SIZE(slave_tx),
        .call_back = test_cb,
        .cb_para = 0,
    };

    rt_completion_init(&test_complete);
    /*slave write first .... */
    rt_kprintf("start slave write!\n");
    rt_kprintf("start master write and read!\n");

    /*slave set data .. */
    rt_device_control(spi_slave_device, SPI_SLAVE_IOCTL_SET_DATA, &xfer);

    /*slave xfer data.. */
    rt_kprintf("1\n");
    rt_device_control(spi_slave_device, SPI_SLAVE_IOCTL_XFER_DATA, &xfer);
    rt_kprintf("2\n");
    ret = rt_spi_transfer(rt_spi_device, master_tx, master_rx, ARRAY_SIZE(slave_tx));
    rt_kprintf("3\n");
    rt_device_control(spi_slave_device, SPI_SLAVE_IOCTL_WAIT_DATA_DONE, &xfer);
    rt_kprintf("4\n");
    rt_completion_wait(&test_complete, RT_WAITING_FOREVER);
    rt_kprintf("5\n");
    for (ret = 0; ret < ARRAY_SIZE(master_tx); ret++)
    {
        if (slave_tx[ret] != master_rx[ret])
        {
            rt_kprintf("slave write master read ::rx[%d]=0x%02x NOT equal to tx[%d]=0x%02x\n",\
            ret, master_rx[ret], ret, slave_tx[ret]);
            failed_count++;
        }
    }

    rt_kprintf("compare data!\n");
    /* 5: compare master tx and slave read.. */
    for (ret = 0; ret < ARRAY_SIZE(master_tx); ret++)
    {
        if (master_tx[ret] != slave_rx[ret])
        {
            rt_kprintf("slave read master write ::rx[%d]=0x%02x NOT equal to tx[%d]=0x%02x\n",\
            ret, slave_rx[ret], ret, master_tx[ret]);
            failed_count++;
         }
    }
    if (failed_count > 0)
        rt_kprintf("\nFAIL -> spi master to slave test\n\n");
    else
        rt_kprintf("\nPASS -> spi master to slave test\n\n");
}



int spi_master_prepare(void)
{
    int ret = 0;

    rt_spi_device = (struct rt_spi_device *)rt_device_find("ssi1_0");
    if (rt_spi_device == RT_NULL)
    {
        rt_kprintf("%s spi device %s not found!\r\n", __func__, "ssi1_0");
        return -RT_ENOSYS;
    }

    /* config spi */
    struct rt_spi_configuration cfg;

    cfg.data_width = 8;
    cfg.mode =
    RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible: Mode 0 and Mode 3 */
    cfg.max_hz = 12.5 * 1000 * 1000; /* 100M/8 */
    ret = rt_spi_configure(rt_spi_device, &cfg);
    return ret;

}

int spi_slave_prepare(void)
{
    int ret = 0;

    spi_slave_device = rt_device_find("spi_slave_0");
    if (spi_slave_device == RT_NULL)
    {
        rt_kprintf("find slave_flash device error\n");
        return -1;
    }

    ret = rt_device_init(spi_slave_device);
    if (ret != 0)
        {
        rt_kprintf("init slave_flash device error\n");
                return ret;
    }

    ret = rt_device_open(spi_slave_device, 0);
    if (ret != 0)
    {
        rt_kprintf("open slave_flash device error\n");
                return ret;
    }

    return 0;
}


int test_spi_m2s(void)
{
    if(RT_EOK != spi_master_prepare())
    {
        rt_kprintf("master spi prepare fail.\n");
        return -1;
        }
    if(RT_EOK != spi_slave_prepare())
    {
        rt_kprintf("slave spi prepare fail.\n");
        return -1;
        }
    /* transfer data to each other.. */
    master_slave_loop_test();
    rt_device_close(spi_slave_device);

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(test_spi_m2s, test_spi_m2s());
#endif
