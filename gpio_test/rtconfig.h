/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

/********************
 *SECTION:	APP
 ********************/
// sensor choice only one
//#define RT_USING_OV9712
// #define RT_USING_OV9732
//#define RT_USING_IMX138
//#define RT_USING_AR0130
#define RT_USING_GPIO_DEMO
// #define WIFI_USING_AP6181

#define RT_USING_COMPONENTS_LINUX_ADAPTER

#define RT_USING_SPISLAVE

#include "platform_def.h"
#endif
