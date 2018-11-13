#include <rtthread.h>
#include <rtdevice.h>

extern void sadc_demo_init(void);
void user_main(void)
{
    sadc_demo_init();
}
