/******************************************************************************

                  版权所有 (C),上海富瀚微电子股份有限公司

 ******************************************************************************
  文 件 名   : human_detect.c
  版 本 号   : v0.0
  作    者  : jinzq272
  生成日期   : 2018年7月23日
  功能描述   : 人形检测功能实现
  函数列表   :
*
*
  修改历史   :
  1.日    期: 2018年07月23日
    作    者: jinzq272
    版    本: v0.0
  1.日    期: 2018年8月17日
    作    者: jinzq272
    版    本: v0.1

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <rtthread.h>
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "sample_opts.h"
#include "human_detect.h"
#include "motionDetect.h"
#include "fh_objdetect_vbcli.h"
#include "fh_imgprocess_vbcli.h"
#include "fh_gvbus_client.h"
#include "types/bufCtrl.h"
#include "model_human_head_40572366.h"
#include "model_human_body_40570666.h"

#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630)
#include <dsp/fh_bgm_mpi.h>
#else
#if USE_BGM
#include <dsp/fh_bgm_mpi.h>
#else
#include "FHAdv_MD_mpi.h"
#endif
#endif

#if GOSD_BOX_ON
#include "FHAdv_GOSD_mpi.h"
#endif

/* 全局数据定义 */
extern int g_stop_running;

/*****************************************************************************
 函 数 名: sample_hd_task
 功能描述  : 移动侦测检测函数
 输入参数  : void *args
 输出参数  : 无
 返 回 值: void*
*****************************************************************************/
void *sample_hd_roi_task(void  *args)
{
    int ret               = 0;      /*返回值*/
    int i                 = 0;
    int j                 = 0;
    int detect_count_down = 0; /*检测到人形后丢掉detect_count_down帧后再次检测*/
    int hd_result         = 0;

    double w_ratio        = 1;
    double h_ratio        = 1;

    FH_VPU_STREAM vpuframe; /*帧数据*/

    MOtion_BGM_RUNTB_RECT det;/*联通域分析结果*/
    Detect_Rect last_dt_rect;

    
    ret = FHAdv_MD_Ex_Init();
    if (ret != 0)
    {
        printf("[ERRO]: FHAdv_MD_Ex_Init failed, ret=%d\n", ret);
        goto exit;
    }

    FHT_MDConfig_Ex_t md_config;

    md_config.threshold = 80;
    md_config.framedelay = 1;
    md_config.enable = 1;

    ret = FHAdv_MD_Ex_SetConfig(&md_config);
    if (ret != 0)
    {
        printf("[ERRO]: FHAdv_MD_Ex_SetConfig failed, ret=%d\n", ret);
        goto exit;
        }

    FILE *fp;/*人形模式参数文件*/
    FH_ODET_cfg_t          cfg;/*人形句柄配置*/
    struct FH_ODET_Handle *odet = NULL;/*人形句柄*/
    FH_imgY8_t *frame_data      = NULL;/*人形检测数据*/
    FH_subImgY8_t roi, ext;/*roi信息*/
    int dt_x, dt_y, dt_w, dt_h;
    MEM_DESC FRAME_BUF;
    MEM_DESC MODEL_FILE_1, MODEL_FILE_2;

#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630)
    FH_BGM_SW_STATUS  bgm_status;/*bgm检测结果*/
    int imgWidth          = 0;
    int imgHeight         = 0;/*bgm图像幅面*/
    int zoomlen = 16;
    FH_UINT8 *confidence_level = NULL; /*存放当前帧bgm前景检测结果*/
    confidence_level = (FH_UINT8 *)malloc(8192);
    if (confidence_level == NULL)
    {
        printf("Error:failed to allocate confidence_level buffer\n");
        goto exit;
    }
    memset(confidence_level, 0, 8192);
#else
#if USE_BGM
    FH_BGM_SW_STATUS  bgm_status;/*bgm检测结果*/
    int imgWidth          = 0;
    int imgHeight         = 0;/*bgm图像幅面*/
    int zoomlen = 16;
    FH_UINT8 *confidence_level = NULL; /*存放当前帧bgm前景检测结果*/
    confidence_level = (FH_UINT8 *)malloc(8192);
    if (confidence_level == NULL)
    {
        printf("Error:failed to allocate confidence_level buffer\n");
        goto exit;
    }
    memset(confidence_level, 0, 8192);
#else
    FHT_MDConfig_Ex_Result_t result;
    int zoomlen = 32;
#endif
#endif

#if GOSD_BOX_ON
    FHT_GOSD_BASECFG_t gboxs[15];/*gosd框*/
    ret = FHAdv_Gosd_Init(VIDEO_INPUT_WIDTH, VIDEO_INPUT_HEIGHT);
    if (ret != 0)
    {
        printf("FHAdv_Gosd_Init failed\n");
    }
#endif

#if GBOX_ON
    FH_VPU_GBOX gbox;
    gbox.chan = 0;
#endif

    /*设置联通域分析幅面*/
    det.base_w = ALIGNTO(CH0_WIDTH, 16) / zoomlen;
    det.base_h = ALIGNTO(CH0_HEIGHT, 16) / zoomlen;

    /*计算幅面映射比例*/
    w_ratio = ((double)CH1_WIDTH / (double)CH0_WIDTH);
    h_ratio = ((double)CH1_HEIGHT / (double)CH0_HEIGHT);

    /*分配检测通道yuv vmm用于码流推送至arc*/
    ret = buffer_malloc(&FRAME_BUF, CH1_WIDTH * CH1_HEIGHT, DEFALT_ALIGN);
    if (ret)
    {
        printf("frame buffer malloc error=%d\n", ret);
        return -1;
    }

    FH_GVBUSCLI_init();/*初始化vbus client功能*/

        /*获取人形检测模型  head: 头肩  body: 全身*/
    cfg.modelFileLen[0] = sizeof(hd_head);
    ret = buffer_malloc(&MODEL_FILE_1, cfg.modelFileLen[0], DEFALT_ALIGN);
    if (ret)
    {
        printf("model file 1 buffer malloc error=%d\n", ret);
        return -1;
    }

    cfg.modelFile[0] = (uint8_t *)MODEL_FILE_1.base;
    memcpy(cfg.modelFile[0], hd_head, cfg.modelFileLen[0]);

    cfg.modelFileLen[1] = sizeof(hd_body);

    ret = buffer_malloc(&MODEL_FILE_2, cfg.modelFileLen[1], DEFALT_ALIGN);
    if (ret)
    {
        printf("model file 2 buffer malloc error=%d\n", ret);
        return -1;
    }

    cfg.modelFile[1] = (uint8_t *)MODEL_FILE_2.base;
    memcpy(cfg.modelFile[1], hd_body, cfg.modelFileLen[1]);

    cfg.maxImageWidth  = CH1_WIDTH;/*人形检测最大幅面*/
    cfg.maxImageHeight = CH1_HEIGHT;
    cfg.modelCnt       = 2; /*人形检测参考模型数量，最大为4*/
    cfg.rotateAngle    = PIC_ROTATE;
    cfg.detectMode     = FH_OBJDET_ROI_ONLY;
    cfg.modelThres[0]  = 50;
    cfg.modelThres[1]  = 40;

    odet = VB_ODET_create(&cfg);/*创建人形检测句柄*/
    if (odet == NULL)
    {
        printf("VB_ODET_create failed\n");
        goto exit;
    }

    fh_objdetect_ver(1);

    frame_data = VB_IM_createBufferY8(CH1_WIDTH, CH1_HEIGHT);/*创建检测数据buffer*/
    if (frame_data == NULL)
    {
        goto exit;
    }
#if 1
    while (!g_stop_running)
    {
        while (detect_count_down > 0)/*如果检测出人形，等待1s后再次检测*/
        {
            detect_count_down--;
            usleep(40*1000);
            continue;
        }
#if GOSD_BOX_ON
        FHAdv_Gosd_Clear(0, 0, VIDEO_INPUT_WIDTH, VIDEO_INPUT_HEIGHT);
#endif
        hd_result = 0;

        ret = FH_VPSS_GetChnFrame(1, &vpuframe);/*获取yuv数据*/
        if (ret != RETURN_OK)
        {
            printf("FH_VPSS_GetChnFrame failed with %d\n", ret);
            usleep(40*1000);
            continue;
        }
        memcpy((unsigned char *)FRAME_BUF.vbase, vpuframe.yluma.vbase, vpuframe.yluma.size);

#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630)
        while (imgWidth != ALIGNTO(CH0_WIDTH, 16) && imgHeight != ALIGNTO(CH0_HEIGHT, 16))/*等待背景建模稳定*/
        {
            ret = FH_BGM_GetSWStatus(&bgm_status);
            imgWidth = bgm_status.size.u32Width;
            imgHeight = bgm_status.size.u32Height;
        }
        ret = FH_BGM_GetSWStatus(&bgm_status);/*获取背景建模信息*/
        memcpy(confidence_level, bgm_status.confidence_level.addr, bgm_status.confidence_level.size);
        getOrdFromGau(confidence_level, 7, &det);/*获取联通域信息*/
#else
#if USE_BGM
        while (imgWidth != ALIGNTO(CH0_WIDTH, 16) && imgHeight != ALIGNTO(CH0_HEIGHT, 16))/*等待背景建模稳定*/
        {
            ret = FH_BGM_GetSWStatus(&bgm_status);
            imgWidth = bgm_status.size.u32Width;
            imgHeight = bgm_status.size.u32Height;
        }
        ret = FH_BGM_GetSWStatus(&bgm_status);/*获取背景建模信息*/
        memcpy(confidence_level, bgm_status.confidence_level.addr, bgm_status.confidence_level.size);
        getOrdFromGau(confidence_level, 7, &det);/*获取联通域信息*/
#else
        FHAdv_MD_CD_Check();
        ret = FHAdv_MD_Ex_GetResult(&result);
        if (ret != FH_SUCCESS)
        {
            printf("\nError: FHAdv_MD_Ex_GetResult failed, ret=%d\n", ret);
            break;
        }
        getOrdFromGau(result.start, 1, &det);/*获取联通域信息*/
#endif
#endif

        for (i = 0; i < det.rect_num; i++)
        {
            /*将motion detect结果映射到子码流幅面*/
            dt_x = (int)(zoomlen * det.rect[i].u32X * w_ratio);
            dt_y = (int)(zoomlen * det.rect[i].u32Y * h_ratio);
            dt_w = (int)(zoomlen * det.rect[i].u32Width * w_ratio);
            dt_h = (int)(zoomlen * det.rect[i].u32Height * h_ratio);
            ret = VB_IM_fillBufferY8(frame_data, (uint8_t *)FRAME_BUF.base);
            if (ret != FH_IM_SUCCESS)
            {
                printf("VB_IM_fillBufferY8 failed\n");
            }
            ret = VB_IM_cropImageY8(frame_data, dt_x, dt_y, dt_w, dt_h, &roi);/*切割roi框*/
            if (ret != FH_IM_SUCCESS)
            {
                printf("VB_IM_cropImageY8 failed\n");
            }
            int padLenH  = MIN(dt_w, dt_h) >> 2;
            int padLenW  = padLenH << 1;
            ret = VB_IM_extendBorderY8(&roi, padLenW, padLenH, &ext);/*扩展roi框*/
            if (ret != FH_IM_SUCCESS)
            {
                printf("VB_IM_extendBorderY8 failed\n");
            }

            ret = VB_ODET_hasObjectInRoi(odet, &ext);/*在roi进行人形检测*/
            if (ret)
            {
                hd_result              = 1;
                detect_count_down      = 25; /*检测出人形后等待1s后再次检测*/
                last_dt_rect.u32X      = ext.x;
                last_dt_rect.u32Y      = ext.y;
                last_dt_rect.u32Width  = ext.w;
                last_dt_rect.u32Height = ext.h;
                printf("%d %d %d %d\n", ext.x, ext.y, ext.w, ext.h);

                /*检测出人形画红（绿）色框*/
#if GOSD_BOX_ON
                dt_x = zoomlen * det.rect[i].u32X;
                dt_y = zoomlen * det.rect[i].u32Y;
                dt_w = zoomlen * det.rect[i].u32Width;
                dt_h = zoomlen * det.rect[i].u32Height;
                gboxs[i].gosdstartPosition.pos_x   = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gboxs[i].gosdstartPosition.pos_y   = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gboxs[i].sizeofbox.gosd_w          = gboxs[i].gosdstartPosition.pos_x + dt_w > CH0_WIDTH ?
                                                         CH0_WIDTH - gboxs[i].gosdstartPosition.pos_x : dt_w;
                gboxs[i].sizeofbox.gosd_h          = gboxs[i].gosdstartPosition.pos_y + dt_h > CH0_HEIGHT ?
                                                         CH0_HEIGHT - gboxs[i].gosdstartPosition.pos_y : dt_h;
                gboxs[i].gosdboxColor.norcolor.fRed         = 255;
                gboxs[i].gosdboxColor.norcolor.fGreen       = 0;
                gboxs[i].gosdboxColor.norcolor.fBlue        = 0;
                gboxs[i].gosdboxColor.norcolor.fAlpha       = 255;
                gboxs[i].gosdboxColor.invcolor.fRed         = 0;
                gboxs[i].gosdboxColor.invcolor.fGreen       = 255;
                gboxs[i].gosdboxColor.invcolor.fBlue        = 0;
                gboxs[i].gosdboxColor.invcolor.fAlpha       = 255;
                ret = FHAdv_Gosd_SetGbox(&gboxs[i]);
                if (ret != 0)
                {
                    printf("FHAdv_Gosd_SetGbox %d failed\n", i);
                }
#endif
#if GBOX_ON
                dt_x = zoomlen * det.rect[i].u32X;
                dt_y = zoomlen * det.rect[i].u32Y;
                dt_w = zoomlen * det.rect[i].u32Width;
                dt_h = zoomlen * det.rect[i].u32Height;
                gbox.gbox[i].enable         = 1;
                gbox.gbox[i].color          = 0xffffffff;
                gbox.gbox[i].area.u32X      = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gbox.gbox[i].area.u32Y      = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gbox.gbox[i].area.u32Width  = gbox.gbox[i].area.u32X + dt_w > CH0_WIDTH ?
                                                         CH0_WIDTH - gbox.gbox[i].area.u32X : dt_w;
                gbox.gbox[i].area.u32Height = gbox.gbox[i].area.u32Y + dt_h > CH0_HEIGHT ?
                                                         CH0_HEIGHT - gbox.gbox[i].area.u32Y : dt_h;
#endif
                goto detect_end;
            }
            else
            {
                /*未检测出人形画黑（白）色框*/
#if GOSD_BOX_ON
                dt_x = zoomlen * det.rect[i].u32X;
                dt_y = zoomlen * det.rect[i].u32Y;
                dt_w = zoomlen * det.rect[i].u32Width;
                dt_h = zoomlen * det.rect[i].u32Height;
                gboxs[i].gosdstartPosition.pos_x   = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gboxs[i].gosdstartPosition.pos_y   = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gboxs[i].sizeofbox.gosd_w          = gboxs[i].gosdstartPosition.pos_x + dt_w > CH0_WIDTH ?
                                                     CH0_WIDTH - gboxs[i].gosdstartPosition.pos_x : dt_w;
                gboxs[i].sizeofbox.gosd_h          = gboxs[i].gosdstartPosition.pos_y + dt_h > CH0_HEIGHT ?
                                                     CH0_HEIGHT - gboxs[i].gosdstartPosition.pos_y : dt_h;
                gboxs[i].gosdboxColor.norcolor.fRed         = 0;
                gboxs[i].gosdboxColor.norcolor.fGreen       = 0;
                gboxs[i].gosdboxColor.norcolor.fBlue        = 0;
                gboxs[i].gosdboxColor.norcolor.fAlpha       = 255;
                gboxs[i].gosdboxColor.invcolor.fRed         = 255;
                gboxs[i].gosdboxColor.invcolor.fGreen       = 255;
                gboxs[i].gosdboxColor.invcolor.fBlue        = 255;
                gboxs[i].gosdboxColor.invcolor.fAlpha       = 255;

                ret = FHAdv_Gosd_SetGbox(&gboxs[i]);
                if (ret != 0)
                {
                    printf("FHAdv_Gosd_SetGbox %d failed\n", i);
                }
#endif
#if GBOX_ON
                dt_x = zoomlen * det.rect[i].u32X;
                dt_y = zoomlen * det.rect[i].u32Y;
                dt_w = zoomlen * det.rect[i].u32Width;
                dt_h = zoomlen * det.rect[i].u32Height;
                gbox.gbox[i].enable         = 1;
                gbox.gbox[i].color          = 0xff008080;
                gbox.gbox[i].area.u32X      = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gbox.gbox[i].area.u32Y      = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gbox.gbox[i].area.u32Width  = gbox.gbox[i].area.u32X + dt_w > CH0_WIDTH ?
                                                         CH0_WIDTH - gbox.gbox[i].area.u32X : dt_w;
                gbox.gbox[i].area.u32Height = gbox.gbox[i].area.u32Y + dt_h > CH0_HEIGHT ?
                                                         CH0_HEIGHT - gbox.gbox[i].area.u32Y : dt_h;
#endif
            }

        }

        /*如果当前帧没有检测到运动区域或者运动区域内没有检测到人形，那么检测上次出现人形的区域*/
        if (!hd_result)
        {
            VB_IM_fillBufferY8(frame_data, (uint8_t *)FRAME_BUF.base);
            VB_IM_cropImageY8(frame_data, last_dt_rect.u32X, last_dt_rect.u32Y, last_dt_rect.u32Width, last_dt_rect.u32Height, &roi);/*切割roi框*/
            ret = VB_ODET_hasObjectInRoi(odet, &roi);
            if (ret)
            {
                detect_count_down      = 25;
                printf("last gosd box%d %d %d %d\n", last_dt_rect.u32X, last_dt_rect.u32Y, last_dt_rect.u32Width, last_dt_rect.u32Height);

                /*检测出人形画红（绿）色框*/
#if GOSD_BOX_ON
                dt_x = last_dt_rect.u32X / w_ratio;
                dt_y = last_dt_rect.u32Y / h_ratio;
                dt_w = last_dt_rect.u32Width / w_ratio;
                dt_h = last_dt_rect.u32Height / h_ratio;;
                gboxs[i+1].gosdstartPosition.pos_x   = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gboxs[i+1].gosdstartPosition.pos_y   = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gboxs[i+1].sizeofbox.gosd_w          = gboxs[i+1].gosdstartPosition.pos_x + dt_w > CH0_WIDTH ?
                                                         CH0_WIDTH - gboxs[i+1].gosdstartPosition.pos_x : dt_w;
                gboxs[i+1].sizeofbox.gosd_h          = gboxs[i+1].gosdstartPosition.pos_y + dt_h > CH0_HEIGHT ?
                                                         CH0_HEIGHT - gboxs[i+1].gosdstartPosition.pos_y : dt_h;
                gboxs[i+1].gosdboxColor.norcolor.fRed         = 255;
                gboxs[i+1].gosdboxColor.norcolor.fGreen       = 0;
                gboxs[i+1].gosdboxColor.norcolor.fBlue        = 0;
                gboxs[i+1].gosdboxColor.norcolor.fAlpha       = 255;
                gboxs[i+1].gosdboxColor.invcolor.fRed         = 0;
                gboxs[i+1].gosdboxColor.invcolor.fGreen       = 255;
                gboxs[i+1].gosdboxColor.invcolor.fBlue        = 0;
                gboxs[i+1].gosdboxColor.invcolor.fAlpha       = 255;
                ret = FHAdv_Gosd_SetGbox(&gboxs[i+1]);
                if (ret != 0)
                {
                    printf("FHAdv_Gosd_SetGbox %d failed\n", i);
                }
#endif
#if GBOX_ON
                dt_x = last_dt_rect.u32X / w_ratio;
                dt_y = last_dt_rect.u32Y / h_ratio;
                dt_w = last_dt_rect.u32Width / w_ratio;
                dt_h = last_dt_rect.u32Height / h_ratio;;
                gbox.gbox[i].enable         = 1;
                gbox.gbox[i].color          = 0xffffffff;
                gbox.gbox[i].area.u32X      = dt_x > CH0_WIDTH ? CH0_WIDTH : dt_x;
                gbox.gbox[i].area.u32Y      = dt_y > CH0_HEIGHT ? CH0_HEIGHT : dt_y;
                gbox.gbox[i].area.u32Width  = gbox.gbox[i].area.u32X + dt_w > CH0_WIDTH ?
                                                         CH0_WIDTH - gbox.gbox[i].area.u32X : dt_w;
                gbox.gbox[i].area.u32Height = gbox.gbox[i].area.u32Y + dt_h > CH0_HEIGHT ?
                                                         CH0_HEIGHT - gbox.gbox[i].area.u32Y : dt_h;
#endif
            }
        }

detect_end:
#if GBOX_ON
        if (hd_result != 0 || det.rect_num != 0)
            i++;
        while (i < MAX_GBOX_AREA)
        {
            gbox.gbox[i].enable         = 0;
            i++;
        }
        ret = FH_VPSS_SetGBox(&gbox);
        if (ret != RETURN_OK)
            printf("FH_VPSS_SetGBox failed with %d\n", ret);
#endif
        usleep(40 * 1000);
    }

exit:
if (frame_data != NULL)
    VB_IM_destroyBufferY8(frame_data);
if (odet != NULL)
    ret = VB_ODET_destroy(odet);
    if (ret != FH_ODET_SUCCESS)
        printf("FH_ODET_destroy failed with %d\n", ret);
#endif
    FH_GVBUSCLI_deinit();
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8830)
    if (confidence_level != NULL)
        free(confidence_level);
#else
#if USE_BGM
    if (confidence_level != NULL)
        free(confidence_level);
#endif
#endif

    return NULL;
}

int hd_roi_init(void)
{
    rt_thread_t thread_hd_roi;
    thread_hd_roi = rt_thread_create("human_detect_roi", sample_hd_roi_task, RT_NULL, 4 * 1024, RT_APP_THREAD_PRIORITY, 10);
    if (thread_hd_roi != RT_NULL)
        rt_thread_startup(thread_hd_roi);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(hd_roi_init, hd_roi_init());
#endif
