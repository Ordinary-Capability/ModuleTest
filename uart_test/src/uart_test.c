#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <rtdevice.h>



#define DATA_LEN 1024

#define RT_SERIAL_CONFIG_DEFAULT           \
{                                          \
    BAUD_RATE_115200, /* 115200 bits/s */  \
    DATA_BITS_8,      /* 8 databits */     \
    STOP_BITS_1,      /* 1 stopbit */      \
    PARITY_NONE,      /* No parity  */     \
    BIT_ORDER_LSB,    /* LSB first sent */ \
    NRZ_NORMAL,       /* Normal mode */    \
    RT_SERIAL_RB_BUFSZ, /* Buffer size */  \
    0                                      \
} 

/* Test start with waiting for a solid bytes pattern. if the bytes recieved not
 * match to the pattern, then test endwith fail-print info. Peer will occur some
 * TIMEOUT issue to indicate test has failed. Otherwise, if the expected bytes
 * revieved, then send the same bytes pattern to the peer and wait for new bytes
 * coming in with another uart configuration. if peer recieves bytes correctly,
 * start sending patten bytes again with another uart configuration which is match
 * to the local one.
 *
 * Author: Zhengk315
 */



static unsigned char g_pattern[DATA_LEN] = {0};

struct serial_configure g_uart_conf_set[] = {
    {BAUD_RATE_57600, DATA_BITS_8, STOP_BITS_1, PARITY_NONE, BIT_ORDER_LSB, NRZ_NORMAL, RT_SERIAL_RB_BUFSZ, 0},
    {BAUD_RATE_115200, DATA_BITS_8, STOP_BITS_1, PARITY_NONE, BIT_ORDER_LSB, NRZ_NORMAL, RT_SERIAL_RB_BUFSZ, 0},
    {BAUD_RATE_115200, DATA_BITS_7, STOP_BITS_1, PARITY_NONE, BIT_ORDER_LSB, NRZ_NORMAL, RT_SERIAL_RB_BUFSZ, 0},
    {BAUD_RATE_115200, DATA_BITS_8, STOP_BITS_2, PARITY_NONE, BIT_ORDER_LSB, NRZ_NORMAL, RT_SERIAL_RB_BUFSZ, 0},
    {BAUD_RATE_115200, DATA_BITS_8, STOP_BITS_1, PARITY_EVEN, BIT_ORDER_LSB, NRZ_NORMAL, RT_SERIAL_RB_BUFSZ, 0},
    };


void init_transfer_pattern(int data_bits)
{
   int i = 0;
   switch(data_bits)
    {
        case DATA_BITS_8:
           for(i=0; i<DATA_LEN; i++) g_pattern[i] = i;
           break;
        case DATA_BITS_7:
           for(i=0; i<DATA_LEN; i++) g_pattern[i] = i&0x7f;
           break;
        case DATA_BITS_6:
           for(i=0; i<DATA_LEN; i++) g_pattern[i] = i&0x3f ;
           break;
        case DATA_BITS_5:
           for(i=0; i<DATA_LEN; i++) g_pattern[i] = i&0x1f;
           break;
        default:
           for(i=0; i<DATA_LEN; i++) g_pattern[i]=i;
        }
    //rt_kprintf("UART transfer bytes pattern:\n");
    //for(i=0; i<DATA_LEN; i++)rt_kprintf("0x%x", g_pattern[i]); 
    //rt_kprintf("\n");
    return;
    }

struct rt_semaphore rx_sem;

static rt_err_t uart_intput(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
    }


int uart_config(const char *name, struct serial_configure *conf)
{
    const char *device_name = name;
    rt_device_t uart_dev = RT_NULL;

    rt_kprintf("Config %s baudrate:%d databits:%d stopbits:%d parity:%d \n",
                name, conf->baud_rate, conf->data_bits, conf->stop_bits, conf->parity);
    uart_dev = rt_device_find(name);
    if(uart_dev == RT_NULL)
    {
        rt_kprintf("Can not find device %s.\n", name);
        return -1;
        }
    if(rt_device_set_rx_indicate(uart_dev, uart_intput) != RT_EOK)
    {
        rt_kprintf("Set rx indicate fail.\n"); 
        return -1;
        }
    if(rt_device_open(uart_dev, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("Open device fail.\n");
        return -1;
        }
    if(RT_EOK!=rt_device_control(uart_dev, RT_DEVICE_CTRL_CONFIG, conf))
	{
		rt_kprintf("Config %s fail.\n", name);
        return -1;
		}
    rt_device_close(uart_dev);
    return 0;
    }


int uart_transfer(const char *name)
{
    rt_device_t uart_dev = RT_NULL;
    char recv_data[DATA_LEN] = {0};
    int count = 0, heel=0, i=0, j=0, ret;


    uart_dev = rt_device_find(name);
    if(uart_dev == RT_NULL)
    {
        rt_kprintf("Can not find device %s.\n", name);
        return -1;
        }
    if(rt_device_open(uart_dev, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("Open device fail.\n");
        return -1;
        }
    rt_sem_init(&rx_sem, "rx", 0, 0);
    if(rt_sem_take(&rx_sem, 1000)<0)
    {
        rt_kprintf("No data recieved.\n");
        rt_device_close(uart_dev);
        return -1;
        }
    count = rt_device_read(uart_dev, 0, &recv_data, DATA_LEN);
    heel = DATA_LEN - count;
    while(heel>0 && rt_sem_take(&rx_sem, 10)==0)
    {
        count = count + rt_device_read(uart_dev, 0, &recv_data[count], DATA_LEN-count);
        heel = DATA_LEN - count;
        }

    if(rt_memcmp(recv_data, g_pattern, DATA_LEN) != 0)
    {
        for(i=0; i<DATA_LEN; i++)
            rt_kprintf("0x%x",recv_data[i]);
        rt_kprintf("\n");
        rt_kprintf("Recived data is uncorrect.\n");
        rt_device_close(uart_dev);
        return -1;
        }
    if(rt_device_write(uart_dev, 0, g_pattern, DATA_LEN) != DATA_LEN)
    {
        rt_kprintf("Send data error.\n");
        rt_device_close(uart_dev);
        return -1;
        }
    rt_thread_delay(10);
    rt_device_close(uart_dev);
    
    return 0;
    }



int uart_test(const char *name)
{
    rt_uint32_t i, n;

    if(name == RT_NULL)
    {
        rt_kprintf("Pass uart device name as func input para, eg. uart1.\n");
        return -1;
        }
    /*
    rt_device_t uart_dev = RT_NULL;
    uart_dev = rt_device_find(name);
    if(rt_device_open(uart_dev, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("Open device fail.\n");
        return -1;
        }
    rt_device_close(uart_dev);*/
    rt_kprintf("=============== UART %s test start ===========\n", name);
    n = sizeof(g_uart_conf_set)/sizeof(struct serial_configure);
    for(i=0; i<n; i++)
    {
        init_transfer_pattern(g_uart_conf_set[i].data_bits);
        if(0 != uart_config(name, &g_uart_conf_set[i]))
        {
            rt_kprintf("Uart %s test fail.\n", name);
            return -1;
            }

        if(0 != uart_transfer(name))
        {
            rt_kprintf("UART %s data transfer failed.\n", name);
            return -1;
            }
        rt_kprintf("UART %s data transfer sucessfully.\n", name);
        }
    rt_kprintf("=============== UART %s test end ===========\n", name);

    return 0;
    }


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(uart_test, uart_test());
#endif

