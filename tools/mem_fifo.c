#include <rtdef.h>
#include <rtthread.h>
#include <stdio.h>
#include "fh_def.h"


static ddr_reg[5] = 
    {0xed000178,0xed000178,0xed00017c,0xed000180,0xed000184};

static reg_offset[5] = {0, 24, 16, 8, 0};

static mem_fifo_run = 0;


int mem_fifo_entry(char name[])
{
    rt_uint32_t i, j, v[5], sum[5], cnt=100;
    mem_fifo_run = 1;
    
    printf("\n                DDR_P0 DDR_P1 DDR_P2 DDR_P3 DDR_P4\n");
    while(mem_fifo_run)
    {
        sum[0]=sum[1]=sum[2]=sum[3]=sum[4]=0;
        for(i=0; i<cnt; i++)
        {
            sum[0] += (GET_REG(0xed000178)>>0)&0x7f; 
            sum[1] += (GET_REG(0xed000178)>>24)&0x7f; 
            sum[2] += (GET_REG(0xed00017c)>>16)&0x7f; 
            sum[3] += (GET_REG(0xed000180)>>8)&0x7f; 
            sum[4] += (GET_REG(0xed000184)>>0)&0x7f; 
            rt_thread_delay(1);
            }
        printf("AVG FIFO LEN: %d %d %d %d %d \n",
            sum[0]/cnt, sum[1]/cnt, sum[2]/cnt, sum[3]/cnt, sum[4]/cnt);
        }
    return 0;
}

void mem_fifo()
{
    rt_thread_t hello_thread=
        rt_thread_create("mem_fifo_length", (void (*)(void* ))mem_fifo_entry, RT_NULL, 10*1024, 80, 20);

    if(hello_thread != RT_NULL)
        rt_thread_startup(hello_thread);
    }

void mem_fifo_stop()
{
    mem_fifo_run = 0;
    }


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(mem_fifo_stop, mem_fifo_stop());
FINSH_FUNCTION_EXPORT(mem_fifo, mem_fifo());
#endif
