/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#define RT_USING_HEAP
#define RT_USING_SLAB
#define RT_USING_MEMALIGN

/*
 * SECTION: Application
 *
 */
#define RT_USING_COMPONENTS_LINUX_ADAPTER
#define RT_USING_HELLOWORLD

// #define RT_USING_ENC28J60
#ifdef RT_USING_ENC28J60
#define ENC28J60_INT (7)                    /* GPIO number for interrupt */
#define ENC28J60_SPI_DEV ("ssi0_1")         /* SPI device name */
#endif

#include "platform_def.h"
#endif
