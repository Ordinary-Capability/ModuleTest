1.function
light led by gpio & light led by switch

2.how to compile
exeute scons in the current directory

3.how to exeute
gpio_output(gpioNum)
eg. gpio_output(7) gpio7 light led

gpio_input(gpioNum , trigType , gpioNumOutput)
eg. gpio_input(6,5) gpio6 as irq input ,gpio5 light led for 5s

trig map table
value           type
1		IRQ_TYPE_EDGE_RISING

2		IRQ_TYPE_EDGE_FALLING

3		IRQ_TYPE_EDGE_BOTH

4		IRQ_TYPE_LEVEL_HIGH

8		IRQ_TYPE_LEVEL_LOW

