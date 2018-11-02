#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <rtdevice.h>



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


struct rt_semaphore rx_sem;

static rt_err_t uart_intput(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
    }


void uart_test()
{
  rt_device_t uart_dev = RT_NULL;
  struct rt_serial_device *serial;
  char send_data[] = "HelloWorld!\n";
  char recv_data[10] = {0};
  rt_err_t ret = 0;
  int i = 0, size = 0;


    uart_dev = rt_device_find("uart1");
    if(uart_dev == RT_NULL)
    {
        rt_kprintf("Can not find device uart1.\n");
        return;
        }

    if(rt_device_set_rx_indicate(uart_dev, uart_intput) != RT_EOK)
    {
        rt_kprintf("Set rx indicate fail.\n"); 
        }

    serial = (struct rt_serial_device *)uart_dev;
	(serial->config).baud_rate = 57600;
    if(serial->ops->configure(serial, &(serial->config)) != RT_EOK)
	{
		rt_kprintf("Config uart1 fail.\n");
		}

    if(rt_device_open(uart_dev, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("Open device fail.\n");
        return;
        }

   
    rt_sem_init(&rx_sem, "rx", 0, 0);
    

    char ch;
    while(1)
    {
        if (rt_sem_take(&rx_sem, RT_WAITING_FOREVER) != RT_EOK) continue;
        while(rt_device_read(uart_dev, 0, &ch, 1) == 1)
        {
            rt_kprintf("%c", ch);
            }
    //rt_device_write(uart_dev, 0, send_data, strlen(send_data));
        }


    if(rt_device_close(uart_dev) != RT_EOK)
    {
        rt_kprintf("Close device fail.\n");
        return;
        }

    rt_kprintf("Uart test exit.\n");
  

    return;
    }



#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(uart_test, uart_test());
#endif

