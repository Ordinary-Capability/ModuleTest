#include <rtdef.h>
#include <rtthread.h>
#include "gpio.h"
#include "iomux.h"

/*
function: gpio trig led blink
gpio_num: which gpio to light
*/

rt_isr_handler_t gpio_irq_cbk(int vector, void *param)
{
    rt_kprintf("Gpio irq occured.\n");
    }


int gpio_irq(int gpio_num, int irq_type)
{
    
    gpio_release(gpio_num);
    fh_select_gpio(gpio_num);
    if(gpio_request(gpio_num)<0)
    {
        rt_kprintf("GPIO %d unavaliable.\n");
        return -1;
        };
   gpio_direction_input(gpio_num);
   gpio_set_irq_type(gpio_num, irq_type);
  
   //fh_gpio_register_interrupt(gpio_to_irq(gpio_num), irq_type,
   //                         gpio_irq_cbk, RT_NULL, "gpio_test");

   rt_hw_interrupt_install(gpio_to_irq(gpio_num), gpio_irq_cbk, RT_NULL, "gpio_test");
   gpio_irq_enable(gpio_to_irq(gpio_num));
   //rt_hw_interrupt_umask(gpio_to_irq(gpio_num));

   return 0;
   }

static int gpio_blink(rt_uint32_t gpio_num)
{
    int status;
    int toggle = 0;

    rt_kprintf("Testing gpio %d for %s\n", gpio_num, "output");
    fh_select_gpio(gpio_num);
    status = gpio_request(gpio_num);  /* / tab key for 8 char */
    if (status < 0)
    {
        rt_kprintf("ERROR can not open GPIO %d\n", gpio_num);
        return status;
    }

    gpio_direction_output(gpio_num, 0);

    toggle = gpio_get_value(gpio_num);

    while (1)
    {
        toggle = !(toggle);

        gpio_set_value(gpio_num, toggle);

        rt_thread_delay(10);

        if (gpio_get_value(gpio_num) != toggle)
        {
            return -RT_ERROR;
        }
    }
    return RT_EOK;
}

static void gpio_blink_main(void *parameter) { gpio_blink(7); }
/*
function: gpio trig led blink
gpio_num: input gpio
gpio_num_out: output gpio to light led
*/
static int gpio_light(int gpio_num, int gpio_num_out)
{
    int ret          = 0;
    int PreValue     = 0;
    int CurrentValue = 0;
    int status;

    fh_select_gpio(gpio_num);
    status = gpio_request(gpio_num);
    if (status < 0)
    {
        rt_kprintf("ERROR can not open GPIO %d\n", gpio_num);
        return status;
    }

    fh_select_gpio(gpio_num_out);
    status = gpio_request(gpio_num_out);
    if (status < 0)
    {
        rt_kprintf("ERROR can not open GPIO %d\n", gpio_num_out);
        return status;
    }

    gpio_direction_input(gpio_num);

    gpio_direction_output(gpio_num_out, 0);

    while (1)
    {
        CurrentValue = gpio_get_value(gpio_num);

        rt_thread_delay(5);

        if (CurrentValue == PreValue)
            continue;

        gpio_set_value(gpio_num_out, CurrentValue);

        PreValue = CurrentValue;
    }
    return ret;
}

static void gpio_light_main(void *parameter) { gpio_light(6, 5); }
void gpio_demo_init(void)
{
    //////start gpio demo task/////2015.11.24//trj////
    rt_thread_t threadBlink;
    

    threadBlink =
        rt_thread_create("blink", gpio_blink_main, RT_NULL, 10 * 1024, 8, 20);

    if (threadBlink != RT_NULL)
        rt_thread_startup(threadBlink);

    rt_thread_t threadLight;

    threadLight =
        rt_thread_create("light", gpio_light_main, RT_NULL, 10 * 1024, 8, 20);

    if (threadLight != RT_NULL)
        rt_thread_startup(threadLight);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(gpio_blink, gpio_blink(gpioNum));
FINSH_FUNCTION_EXPORT(gpio_blink_main, gpio_blink_main(gpioNum));
FINSH_FUNCTION_EXPORT(gpio_light_main, gpio_light_main(gpioNum, gpioNumOut));
FINSH_FUNCTION_EXPORT(gpio_irq, gpio_irq(gpioNum));
#endif
