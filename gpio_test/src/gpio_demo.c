#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "gpio.h"
#include "iomux.h"

/*GPIO TEST solution:
 *To test gpio pull UP/DOWN function, set target gpio pin as output pin,
 *choice a reference pin(eg #0) as input pin, connect both pins with a 
 *dupon wire. Then pull UP/DOWN the target pin and get status from the 
 *reference pin to check result.
 *
 *GPIO irq function test solution is similar to the previous one. 
 *Set the target pin as input pin, choice a reference pin as output pin.
 *Then pull UP/DOWN the reference pin to triger irq callbck function tp varify
 *if the function is ok.
 *
 *Author: zhengk315
 */



static int g_cnt = 0;
static int g_irq_type = 0;
static int g_refer_gpio = 0;
struct rt_completion dma_complete;

void gpio_irq_cbk(int vector, void *param)
{
    rt_kprintf("\tGpio irq occured %d. type %d.\n", ++g_cnt, g_irq_type);
    //PULL UP the reference pin or there'll be consecutive irq callbck
    if(g_irq_type == IRQ_TYPE_LEVEL_LOW) 
        gpio_direction_output(g_refer_gpio, 1);
    //PULL DWON the reference pin or there'll be consecutive irq callbck
    else if(g_irq_type == IRQ_TYPE_LEVEL_HIGH)
        gpio_direction_output(g_refer_gpio, 0);
    rt_completion_done(&dma_complete);
    }


int init_refer_gpio(int gpio_num)
{
    gpio_release(gpio_num);
    fh_select_gpio(gpio_num);
    if(gpio_request(gpio_num)<0)
    {
        rt_kprintf("GPIO %d unavaliable.\n");
        return -1;
        };
    gpio_direction_output(gpio_num, 0);
    return 0; 
    }


int gpio_irq(int gpio_num, int irq_type)
{
    int i =0, init_status = 0;

    g_irq_type = irq_type;
    g_cnt = 0;

    fh_select_gpio(gpio_num);
    if(gpio_request(gpio_num)<0)
    {
        rt_kprintf("Request GPIO %d fail.\n", gpio_num);
        return -1;
        };
    gpio_direction_input(gpio_num);

    fh_select_gpio(g_refer_gpio);
    if(gpio_request(g_refer_gpio)<0)
    {
        rt_kprintf("Request GPIO %d fail.\n", g_refer_gpio);
        gpio_release(gpio_num);
        return -1;
        };

    switch(irq_type)
    {
        case IRQ_TYPE_EDGE_RISING: 
            rt_kprintf("GPIO %d irq type IRQ_TYPE_EDGE_RISING test...\n", gpio_num);
            init_status = 0;
            break;
        case IRQ_TYPE_EDGE_FALLING:
            rt_kprintf("GPIO %d irq type IRQ_TYPE_EDGE_FALLING test...\n", gpio_num);
            init_status = 1;
            break;
        case IRQ_TYPE_EDGE_BOTH:
            init_status = 0;
            rt_kprintf("GPIO %d irq type IRQ_TYPE_EDGE_BOTH test...\n", gpio_num);
            break;
        case IRQ_TYPE_LEVEL_LOW:
            rt_kprintf("GPIO %d irq type IRQ_TYPE_LEVEL_LOW test...\n", gpio_num);
            init_status = 1;
            break;
        case IRQ_TYPE_LEVEL_HIGH:
            rt_kprintf("GPIO %d irq type IRQ_TYPE_LEVEL_HIGH test...\n", gpio_num);
            init_status = 0;
            break;
        default:
            rt_kprintf("GPIO %d irq type unknown ...\n");
            return -1;
        }
    gpio_direction_output(g_refer_gpio, init_status);
    gpio_set_irq_type(gpio_num, irq_type);
    rt_hw_interrupt_install(gpio_to_irq(gpio_num), gpio_irq_cbk, RT_NULL, "gpio_test");
    gpio_irq_enable(gpio_to_irq(gpio_num));

    for(i=0; i<10; i++)
    {
        rt_completion_init(&dma_complete);
        init_status ^= 1;
        gpio_set_value(g_refer_gpio, init_status);
        rt_thread_delay(1); 
        if(rt_completion_wait(&dma_complete, 100)<0)
        {
            rt_kprintf("Fail to triger GPIO %d irq %d.\n", gpio_num, irq_type);
            gpio_irq_disable(gpio_to_irq(gpio_num));
            gpio_release(gpio_num);
            gpio_release(g_refer_gpio);
            return -1;
            }
        //reset status 
        if(irq_type==IRQ_TYPE_EDGE_RISING 
            || irq_type==IRQ_TYPE_EDGE_FALLING
            || irq_type==IRQ_TYPE_LEVEL_LOW
            || irq_type==IRQ_TYPE_LEVEL_HIGH)
        {
            init_status ^= 1;
            gpio_set_value(g_refer_gpio, init_status);
            }
        }        

    gpio_irq_disable(gpio_to_irq(gpio_num));
    gpio_release(gpio_num);
    gpio_release(g_refer_gpio);
    return 0;
    }

int gpio_level_test(int gpio_num)
{
    int i=0, level = 0;

    rt_kprintf("GPIO %d level test...\n", gpio_num);
    fh_select_gpio(gpio_num);
    if(gpio_request(gpio_num)<0)
    {
        rt_kprintf("Request GPIO %d fail.\n", gpio_num);
        return -1;
        };

    fh_select_gpio(g_refer_gpio);
    if(gpio_request(g_refer_gpio)<0)
    {
        rt_kprintf("Request GPIO %d fail.\n", g_refer_gpio);
        gpio_release(gpio_num);
        return -1;
        };

    gpio_direction_input(g_refer_gpio);
    gpio_direction_output(gpio_num, 0);

    for(i=0; i<10; i++)
    {
        gpio_set_value(gpio_num, level);
        if( gpio_get_value(g_refer_gpio) != level) //check value of the input gpio pin
        {
            rt_kprintf("Check GPIO %d level %d fail.\n", gpio_num, level);
            gpio_release(gpio_num);
            gpio_release(g_refer_gpio);
            return -1;
            }
        rt_kprintf("Toggle GPIO %d level to %d successfully.\n", gpio_num, level);
        level ^= 1;
        }

    gpio_release(gpio_num);
    gpio_release(g_refer_gpio);
    return 0;
    }


int gpio_func_test(int gpio_num)
{
    if(gpio_level_test(gpio_num))
    {
        rt_kprintf("GPIO %d level test fail.\n", gpio_num);
        return -1;
        }

    if(gpio_irq(gpio_num, IRQ_TYPE_EDGE_RISING)
        ||gpio_irq(gpio_num, IRQ_TYPE_EDGE_FALLING)
        ||gpio_irq(gpio_num, IRQ_TYPE_EDGE_BOTH)
        ||gpio_irq(gpio_num, IRQ_TYPE_LEVEL_LOW)
        ||gpio_irq(gpio_num, IRQ_TYPE_LEVEL_HIGH))
    {
        rt_kprintf("Gpio %d irq test fail.\n", gpio_num);
        return -1;
        }
        
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
    rt_kprintf("Gpio %d, value %d\n", gpio_num, toggle);

    while (1)
    {
        toggle = !(toggle);

        gpio_set_value(gpio_num, toggle);

        rt_thread_delay(10);

        if (gpio_get_value(gpio_num) != toggle)
        {
            return -RT_ERROR;
            rt_kprintf("Set gpio %d to %d fail.\n", gpio_num, toggle);
        }
    }
    return RT_EOK;
}

static void gpio_blink_main(void *parameter) { gpio_blink(1); }
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
FINSH_FUNCTION_EXPORT(gpio_func_test, gpio_func_test(gpioNum));
#endif
