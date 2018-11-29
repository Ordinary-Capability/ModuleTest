#include <stdio.h>
#include <rtthread.h>
#include <rthw.h>

#define CPU_USAGE_CALC_TICK    10
#define CPU_USAGE_LOOP        100

static rt_uint32_t total_count = 0;
static rt_uint32_t total_tick=0;
float idle_tick=0.0;
static cpu_usage_run = 0;

static void cpu_usage_idle_hook(void)
{
    rt_tick_t tick;
    rt_uint32_t count;
    volatile rt_uint32_t loop;

    if (total_count == 0)
    {
        /* get total count */
        rt_enter_critical();
        tick = rt_tick_get();
        while(rt_tick_get() - tick < CPU_USAGE_CALC_TICK)
        {
            total_count ++;
            loop = 0;

            while (loop < CPU_USAGE_LOOP) loop ++;
        }
        rt_exit_critical();
    }

    count = 0;
    /* get CPU usage */
    tick = rt_tick_get();
    while (rt_tick_get() - tick < CPU_USAGE_CALC_TICK)
    {
        count ++;
        loop  = 0;
        while (loop < CPU_USAGE_LOOP) loop ++;
    }

    idle_tick = (float)count*10/total_count + idle_tick;

    if(count >= total_count) total_count = count;
}



void cpu_usage_entry()
{
    rt_uint32_t last_tick, delta_tick;
    float last_idle_tick, delta_busy_tick;
	
    rt_thread_idle_sethook(cpu_usage_idle_hook);
    cpu_usage_run = 1;

	while(cpu_usage_run)
	{
        last_tick = rt_tick_get();
        last_idle_tick = idle_tick;
		rt_thread_delay(200);	
        delta_tick = rt_tick_get()-last_tick;
        delta_busy_tick = delta_tick - (idle_tick - last_idle_tick);
        printf("CPU usage: %.2f%%\n", delta_busy_tick*100/delta_tick);
		}

}

void cpu_usage()
{
    rt_thread_t cpu_usage_thread;

    cpu_usage_thread = rt_thread_create("cpu_usage", cpu_usage_entry, RT_NULL, 10*1024, 80, 20);
    if(cpu_usage_thread != RT_NULL)
        rt_thread_startup(cpu_usage_thread);

    }

void cpu_usage_stop()
{
    cpu_usage_run = 0;
    }



#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(cpu_usage, cpu_usage());
FINSH_FUNCTION_EXPORT(cpu_usage_stop, cpu_usage_stop());
#endif
