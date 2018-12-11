#ifndef __RTTHREAD_APP_H__
#define __RTTHREAD_APP_H__

/*
 * SECTION: Application
 *
 * NOTE: choice one sensor only if not multi_sensor
 */
#define FH_USING_COOLVIEW
/* #define FH_USING_MULTI_SENSOR */
#ifdef CONFIG_CHIP_FH8856
#define RT_USING_JXF22_MIPI
#else
#ifdef CONFIG_CHIP_FH8833T
#define RT_USING_JXF22
#elif defined(CONFIG_CHIP_FH8620H)
#define RT_USING_JXH62_MIPI
#elif defined(CONFIG_CHIP_FH8852)
/*#define RT_USING_OVOS05_MIPI*/
#define RT_USING_JXF22_MIPI
#else
#define RT_USING_JXF22_MIPI
#endif
#endif

#define RT_USING_LIBVLC
#define RT_USING_SAMPLE_OVERLAY
#define RT_USING_HUMAN_DETECT
#ifdef RT_USING_HUMAN_DETECT
/*#define RT_USING_HUMAN_DETECT_FULL*/
#define RT_USING_HUMAN_DETECT_ROI
#endif
#define RT_USING_SAMPLE_VENC
#define RT_USING_SAMPLE_VLCVIEW
#define RT_USING_SAMPLE_SMART_ENC
#define RT_USING_SAMPLE_MJPEG
#define RT_APP_THREAD_PRIORITY 130
#define FH_USING_ADVAPI_ISP
#define FH_USING_ADVAPI_OSD
#define FH_USING_ADVAPI_MD
#define FH_USING_ADVAPI_GOSD
#define RT_USING_COMPONENTS_RSHELL

#define RT_USING_DSP
#define RT_USING_ISP

/* #define FH_USING_RTSP */
#ifdef FH_USING_RTSP
# define FH_USING_RTSP_RTP_TCP 1
# define FH_USING_RTSP_RTP_UDP 0
#else
# define FH_USING_PES_PACK
#endif

#ifdef RT_USING_SOFTCORE
#ifndef RT_USING_VBUS
#error "softcore depends on VBUS, you must define RT_USING_VBUS."
#endif
#endif
#define FH_DGB_DSP_PROC
#define FH_DGB_ISP_PROC

#define RT_USING_COMPONENTS_LINUX_ADAPTER

#define FH_BOOT_IN_2STAGE
#define RT_USING_AUDIO_DEMO

#include "platform_def.h"
#endif /* end of rtconfig.h */
