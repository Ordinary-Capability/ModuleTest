/******************************************************************************

                  版权所有 (C),上海富瀚微电子股份有限公司

 ******************************************************************************
  文 件 名   : human_detect.h
  版 本 号   : 初稿
  作    者   : jinzq272
  生成日期   : 2018年07月23日
  最近修改   :
  功能描述   : 人形检测数据函数定义
******************************************************************************/
#ifndef __HUMAN_DETECT_H__
#define __HUMAN_DETECT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(CONFIG_CHIP_FH8632) || defined(CONFIG_CHIP_FH8856) || defined(CONFIG_CHIP_FH8852)
#define USE_BGM       0  /*sensor旋转角度*/
#endif
#define GOSD_BOX_ON     0  /*是否画框,仅用于FH8632或FH8856平台 1: 画框  0: 不画框*/
#define GBOX_ON         0  /*是否画框,仅用于FH8830或FH8630平台 1: 画框  0: 不画框*/
#define PIC_ROTATE      0  /*sensor旋转角度 0: 0度 1: 90度 2: 180度 3: 270度*/

#define ALIGNTO(addr, edge)  ((addr + edge - 1) & ~(edge - 1)) /* 数据结构对齐定义 */

typedef struct
{
  unsigned int u32X;
  unsigned int u32Y;
  unsigned int u32Width;
  unsigned int u32Height;
} Detect_Rect;

/* 人形检测相关函数定义 */
void *sample_hd_task(void *args);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


