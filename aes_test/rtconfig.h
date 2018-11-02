/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

/*
 * SECTION: Application
 *
 */
#define RT_USING_ALGORITHM
#define RT_USING_AES
#define RT_USING_HIK_DEMO
/* #define RT_USING_ENC28J60 */
#ifdef RT_USING_ENC28J60
#define ENC28J60_INT (7)                    /* GPIO number for interrupt */
#define ENC28J60_SPI_DEV ("ssi0_1")         /* SPI device name */
#endif

#define RT_USING_COMPONENTS_LINUX_ADAPTER

#include "platform_def.h"
#endif
