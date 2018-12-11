#include <rtdef.h>
#include <rtthread.h>
#include "sadc.h"

#define MAX_CHANNEL_NUM 3
#define REF_VOLT 3300
#define DIGIT_MAX 0x3ff  /* / driver def */

static int sadc_run=0;

void sadc_demo_main(void *param)
{
    rt_device_t sadc_dev;
    SADC_INFO data;
    rt_uint32_t i, j;

    sadc_dev = rt_device_find("sadc");
    if (!sadc_dev)
    {
        rt_kprintf("cann't find the sadc dev\n");
        return;
    }

    sadc_dev->init(sadc_dev);
    sadc_dev->open(sadc_dev, 0);

    sadc_run = 1;
    j=0;
    while(sadc_run==1 && j<200)
    {
        rt_kprintf("SADC channel(0-3) volt:");
        for(i=0; i<= MAX_CHANNEL_NUM; i++)
        {
            data.channel = i;
            rt_device_control(sadc_dev, SADC_CMD_READ_VOLT, &data);
            rt_kprintf("\t%dmV", data.sadc_data);
            }
        rt_kprintf("\n");
        rt_thread_delay(20);
        j++;
    }

    rt_device_close(sadc_dev);
    return;
}

void sadc_exit()
{
    sadc_run = 0;
    return;
    }


void sadc_test(void)
{
    rt_thread_t threadSadc;

    threadSadc =
        rt_thread_create("sadc_test", sadc_demo_main, RT_NULL, 10 * 1024, 80, 20);

    if (threadSadc != RT_NULL)
        rt_thread_startup(threadSadc);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(sadc_test, sadc_test());
FINSH_FUNCTION_EXPORT(sadc_exit, sadc_exit());
#endif
