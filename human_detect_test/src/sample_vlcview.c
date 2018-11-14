/*
 * File      : sample_vlcview.c
 * This file is part of SOCs BSP for RT-Thread distribution.
 *
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.
 * All rights reserved
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Visit http://www.fullhan.com to get contact with Fullhan.
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include <dfs_posix.h>
#include "types/type_def.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "isp/isp_api.h"
#include "bufCtrl.h"
#include "sample_common_isp.h"
#include "sample_common_dsp.h"
#include "sample_opts.h"
#include "isp_enum.h"
#include "multi_sensor.h"

#if defined(CONFIG_CHIP_FH8632) || defined(CONFIG_CHIP_FH8833) || defined(CONFIG_CHIP_FH8833T) || defined(CONFIG_CHIP_FH8856) || defined(CONFIG_CHIP_FH8620H)
#define CHANNEL_COUNT 1
#elif defined(CONFIG_CHIP_FH8852)
#define CHANNEL_COUNT 1
#else
#define CHANNEL_COUNT 3
#endif

#if defined(CONFIG_CHIP_FH8856) || defined(CONFIG_CHIP_FH8852)
#define USE_H265 1
#endif

struct channel_info
{
    FH_SINT32 channel;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT32 bps;
};

static struct channel_info g_channel_infos[] = {
    {
        .channel = 0,
        .width = CH0_WIDTH,
        .height = CH0_HEIGHT,
        .frame_count = CH0_FRAME_COUNT,
        .frame_time = CH0_FRAME_TIME,
        .bps = CH0_BIT_RATE
    },
#if (CHANNEL_COUNT > 1)
    {
        .channel = 1,
        .width = CH1_WIDTH,
        .height = CH1_HEIGHT,
        .frame_count = CH1_FRAME_COUNT,
        .frame_time = CH1_FRAME_TIME,
        .bps = CH1_BIT_RATE
    },
#endif
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630) || defined(CONFIG_CHIP_FH8630M)
    {
        .channel = 2,
        .width = CH2_WIDTH,
        .height = CH2_HEIGHT,
        .frame_count = CH2_FRAME_COUNT,
        .frame_time = CH2_FRAME_TIME,
        .bps = CH2_BIT_RATE
    },
#endif
};

static FH_BOOL g_stop_running = FH_TRUE;
static rt_thread_t g_thread_isp;
static rt_thread_t g_thread_stream;

#ifdef FH_USING_RTSP
#include "rtsp.h"
struct rtsp_server_context *g_rtsp_server[CHANNEL_COUNT];
#else
#include "pes_pack.h"
#endif

#ifdef FH_USING_COOLVIEW
#include "dbi/dbi_over_tcp.h"
#include "dbi/dbi_over_udp.h"
static struct dbi_tcp_config g_tcp_conf;
#endif

void sample_vlcview_exit(void)
{
    rt_thread_t exit_process;

    exit_process = rt_thread_find("vlc_get_stream");
    if (exit_process)
        rt_thread_delete(exit_process);

    API_ISP_Exit();
    exit_process = rt_thread_find("vlc_isp");
    if (exit_process)
        rt_thread_delete(exit_process);

#ifdef FH_USING_COOLVIEW
    exit_process = rt_thread_find("coolview");
    if (exit_process)
        rt_thread_suspend(exit_process);
#endif
    FH_SYS_Exit();
}

void sample_vlcview_get_stream_proc(void *arg)
{
    int fd = 0, c=0;

    FH_SINT32 ret, i;
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630) || defined(CONFIG_CHIP_FH8630M)
    FH_ENC_STREAM_ELEMENT stream;
#else
    FH_VENC_STREAM stream;
    unsigned int chan;
#endif
#ifndef FH_USING_RTSP
    struct vlcview_enc_stream_element stream_element;
#endif
    FH_SINT32 *cancel = (FH_SINT32 *)arg;




    while (!*cancel)
    {
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630) || defined(CONFIG_CHIP_FH8630M)
        for (i = 0; i < CHANNEL_COUNT; i++)
        {
            do
            {
                ret = FH_VENC_GetStream(i, &stream);
                if (ret == RETURN_OK)
                {
                    int j;
#ifdef FH_USING_RTSP
                    for (j = 0; j < stream.nalu_cnt; j++)
                        rtsp_push_data(g_rtsp_server[stream.chan], stream.nalu[j].start, stream.nalu[j].length, stream.time_stamp);
#else
                    stream_element.enc_type   = VLCVIEW_ENC_H264;
                    stream_element.frame_type = stream.frame_type == I_SLICE ? VLCVIEW_ENC_I_FRAME : VLCVIEW_ENC_P_FRAME;
                    stream_element.frame_len  = stream.length;
                    stream_element.time_stamp = stream.time_stamp;
                    stream_element.nalu_count = stream.nalu_cnt;
                    for (j = 0; j < stream_element.nalu_count; j++)
                    {
                        stream_element.nalu[j].start = stream.nalu[j].start;
                        stream_element.nalu[j].len = stream.nalu[j].length;
                    }

                    vlcview_pes_stream_pack(stream.chan, stream_element);
#endif
                    FH_VENC_ReleaseStream(i);
                }
            } while (ret == RETURN_OK);
        }
        rt_thread_delay(1);
#else
#if USE_H265
        ret = FH_VENC_GetStream_Block(FH_STREAM_H265, &stream);
        if (ret == RETURN_OK)
        {
#ifdef FH_USING_RTSP
            struct channel_format_info chan_format_info;

            chan = stream.h265_stream.chan;
            chan_format_info.video_data_flag = 1;
            chan_format_info.audio_enable_flag = 0;
            chan_format_info.channel_id = 0;
            chan_format_info.video_format = FH_VENC_H265;

            for (i = 0; i < stream.h265_stream.nalu_cnt; i++)
            {
                chan_format_info.video_data_flag = 1;
                rtsp_set_channel_info(g_rtsp_server[chan], &chan_format_info);
                rtsp_push_data(g_rtsp_server[chan], stream.h265_stream.nalu[i].start, stream.h265_stream.nalu[i].length,
                    stream.h265_stream.time_stamp);
                chan_format_info.video_data_flag = 0;
                rtsp_set_channel_info(g_rtsp_server[chan], &chan_format_info);
            }
#else
            stream_element.enc_type   = VLCVIEW_ENC_H265;
            stream_element.frame_type = stream.h265_stream.frame_type == FH_FRAME_I ? VLCVIEW_ENC_I_FRAME : VLCVIEW_ENC_P_FRAME;
            stream_element.frame_len  = stream.h265_stream.length;
            stream_element.time_stamp = stream.h265_stream.time_stamp;
            stream_element.nalu_count = stream.h265_stream.nalu_cnt;
            chan                      = stream.h265_stream.chan;
            fd = open("/h264.1", O_WRONLY|O_CREAT|O_APPEND);
            if(fd < 0)
                rt_kprintf("Open /tt.h264 fail.\n");
            for (i = 0; i < stream_element.nalu_count; i++)
            {
                stream_element.nalu[i].start = stream.h265_stream.nalu[i].start;
                stream_element.nalu[i].len   = stream.h265_stream.nalu[i].length;
                write(fd, (void *)stream.h265_stream.nalu[i].start, stream.h265_stream.nalu[i].length);
            }
            //write(fd, (void *)stream.h265_stream.start, stream.h265_stream.length);
            close(fd);
            vlcview_pes_stream_pack(chan, stream_element);
#endif
            FH_VENC_ReleaseStream(chan);
        }
#else
        ret = FH_VENC_GetStream_Block(FH_STREAM_H264, &stream);
        if (ret == RETURN_OK)
        {
#ifdef FH_USING_RTSP
            chan = stream.h264_stream.chan;
            for (i = 0; i < stream.h264_stream.nalu_cnt; i++)
            {
                rtsp_push_data(g_rtsp_server[chan], stream.h264_stream.nalu[i].start, stream.h264_stream.nalu[i].length,
                    stream.h264_stream.time_stamp);
            }
#else
            stream_element.enc_type   = VLCVIEW_ENC_H264;
            stream_element.frame_type = stream.h264_stream.frame_type == FH_FRAME_I ? VLCVIEW_ENC_I_FRAME : VLCVIEW_ENC_P_FRAME;
            stream_element.frame_len  = stream.h264_stream.length;
            stream_element.time_stamp = stream.h264_stream.time_stamp;
            stream_element.nalu_count = stream.h264_stream.nalu_cnt;
            chan = stream.h264_stream.chan;
            fd = open("/h264.1", O_WRONLY|O_CREAT|O_APPEND);
            if(fd < 0)
                rt_kprintf("Open /tt.h264 fail.\n");
            for (i = 0; i < stream_element.nalu_count; i++)
            {
                stream_element.nalu[i].start = stream.h264_stream.nalu[i].start;
                stream_element.nalu[i].len = stream.h264_stream.nalu[i].length;
            }
            write(fd, (void *)stream.h264_stream.start, stream.h264_stream.length);
            close(fd);
            vlcview_pes_stream_pack(chan, stream_element);
#endif
            FH_VENC_ReleaseStream(chan);
        }
#endif
#endif
    }
    close(fd);
}

#ifdef FH_USING_RTSP
static void force_iframe(void *param)
{
    int *p_chan = (int *)param;
    FH_VENC_RequestIDR(*p_chan);
}
#endif

int vlcview(char *dsp_ip, rt_uint32_t port_no)
{
    FH_VPU_SIZE vi_pic;
    FH_VPU_CHN_CONFIG chn_attr;
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630) || defined(CONFIG_CHIP_FH8630M)
    FH_CHR_CONFIG cfg_param;
#else
    FH_VENC_CHN_CAP cfg_vencmem;
    FH_VENC_CHN_CONFIG cfg_param;
#endif
    FH_SINT32 ret;
    FH_SINT32 i;
    FH_SINT32 port = port_no;

    if (!g_stop_running)
    {
        printf("vicview is running!\n");
        return 0;
    }

    bufferInit((unsigned char *)FH_SDK_MEM_START, FH_SDK_MEM_SIZE);
    media_driver_config();

    /******************************************
     step  1: init media platform
    ******************************************/
    ret = FH_SYS_Init();
    if (ret != RETURN_OK)
    {
        printf("Error: FH_SYS_Init failed with %d\n", ret);
        goto err_exit;
    }

    /******************************************
     step  2: set vpss resolution
    ******************************************/
    vi_pic.vi_size.u32Width = VIDEO_INPUT_WIDTH;
    vi_pic.vi_size.u32Height = VIDEO_INPUT_HEIGHT;
    ret = FH_VPSS_SetViAttr(&vi_pic);
    if (ret != RETURN_OK)
    {
        printf("Error: FH_VPSS_SetViAttr failed with %d\n", ret);
        goto err_exit;
    }

    /******************************************
     step  3: start vpss
    ******************************************/
    ret = FH_VPSS_Enable(0);
    if (ret != RETURN_OK)
    {
        printf("Error: FH_VPSS_Enable failed with %d\n", ret);
        goto err_exit;
    }

    /******************************************
     step  4: configure vpss channels
    ******************************************/
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
        chn_attr.vpu_chn_size.u32Width = g_channel_infos[i].width;
        chn_attr.vpu_chn_size.u32Height = g_channel_infos[i].height;
        ret = FH_VPSS_SetChnAttr(i, &chn_attr);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VPSS_SetChnAttr failed with %d\n", ret);
            goto err_exit;
        }

        /******************************************
         step  5: open vpss channel
        ******************************************/
        ret = FH_VPSS_OpenChn(i);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VPSS_OpenChn failed with %d\n", ret);
            goto err_exit;
        }

#if USE_H265
        FH_VPSS_SetVOMode(i, VPU_VOMODE_SCAN);
#endif

        /******************************************
         step  6: create venc channel
        ******************************************/
#if defined(CONFIG_CHIP_FH8830) || defined(CONFIG_CHIP_FH8630) || defined(CONFIG_CHIP_FH8630M)
        cfg_param.chn_attr.size.u32Width = g_channel_infos[i].width;
        cfg_param.chn_attr.size.u32Height = g_channel_infos[i].height;
        cfg_param.rc_config.bitrate = g_channel_infos[i].bps;
        cfg_param.rc_config.FrameRate.frame_count = g_channel_infos[i].frame_count;
        cfg_param.rc_config.FrameRate.frame_time  = g_channel_infos[i].frame_time;
        cfg_param.chn_attr.profile = FH_PRO_MAIN;
        cfg_param.chn_attr.i_frame_intterval = 50;
        cfg_param.init_qp = 35;
        cfg_param.rc_config.ImaxQP = 42;
        cfg_param.rc_config.IminQP = 28;
        cfg_param.rc_config.PmaxQP = 42;
        cfg_param.rc_config.PminQP = 28;
        cfg_param.rc_config.RClevel = FH_RC_LOW;
        cfg_param.rc_config.RCmode = FH_RC_VBR;
        cfg_param.rc_config.max_delay = 8;

        ret = FH_VENC_CreateChn(i, &cfg_param);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VENC_CreateChn failed with %d\n", ret);
            goto err_exit;
        }
#else
#if USE_H265
        cfg_vencmem.support_type = FH_NORMAL_H265;
        cfg_vencmem.max_size.u32Width = g_channel_infos[i].width;
        cfg_vencmem.max_size.u32Height = g_channel_infos[i].height;

        ret = FH_VENC_CreateChn(i, &cfg_vencmem);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VENC_CreateChn failed with %d\n", ret);
            goto err_exit;
        }

        cfg_param.chn_attr.enc_type = FH_NORMAL_H265;
        cfg_param.chn_attr.h265_attr.profile = H265_PROFILE_MAIN;
        cfg_param.chn_attr.h265_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h265_attr.size.u32Width = g_channel_infos[i].width;
        cfg_param.chn_attr.h265_attr.size.u32Height = g_channel_infos[i].height;

        cfg_param.rc_attr.rc_type = FH_RC_H265_VBR;
        cfg_param.rc_attr.h265_vbr.init_qp = 35;
        cfg_param.rc_attr.h265_vbr.bitrate = g_channel_infos[i].bps;
        cfg_param.rc_attr.h265_vbr.ImaxQP = 42;
        cfg_param.rc_attr.h265_vbr.IminQP = 28;
        cfg_param.rc_attr.h265_vbr.PmaxQP = 42;
        cfg_param.rc_attr.h265_vbr.PminQP = 28;
        cfg_param.rc_attr.h265_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h265_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h265_vbr.IP_QPDelta = 0;
        cfg_param.rc_attr.h265_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h265_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h265_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_count = g_channel_infos[i].frame_count;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_time  = g_channel_infos[i].frame_time;
#else
        cfg_vencmem.support_type = FH_NORMAL_H264;
        cfg_vencmem.max_size.u32Width = g_channel_infos[i].width;
        cfg_vencmem.max_size.u32Height = g_channel_infos[i].height;

        ret = FH_VENC_CreateChn(i, &cfg_vencmem);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VENC_CreateChn failed with %d\n", ret);
            goto err_exit;
        }

        cfg_param.chn_attr.enc_type = FH_NORMAL_H264;
        cfg_param.chn_attr.h264_attr.profile = H264_PROFILE_MAIN;
        cfg_param.chn_attr.h264_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h264_attr.size.u32Width = g_channel_infos[i].width;
        cfg_param.chn_attr.h264_attr.size.u32Height = g_channel_infos[i].height;

        cfg_param.rc_attr.rc_type = FH_RC_H264_VBR;
        cfg_param.rc_attr.h264_vbr.init_qp = 35;
        cfg_param.rc_attr.h264_vbr.bitrate = CH0_BIT_RATE;
        cfg_param.rc_attr.h264_vbr.ImaxQP = 42;
        cfg_param.rc_attr.h264_vbr.IminQP = 28;
        cfg_param.rc_attr.h264_vbr.PmaxQP = 42;
        cfg_param.rc_attr.h264_vbr.PminQP = 28;
        cfg_param.rc_attr.h264_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h264_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h264_vbr.IP_QPDelta = 0;
        cfg_param.rc_attr.h264_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h264_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h264_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_count = g_channel_infos[i].frame_count;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_time = g_channel_infos[i].frame_time;
#endif
        ret = FH_VENC_SetChnAttr(i, &cfg_param);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VENC_SetChnAttr failed with %d\n", ret);
            goto err_exit;
        }
#endif
        /******************************************
         step  7: start venc channel
        ******************************************/
        ret = FH_VENC_StartRecvPic(i);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_VENC_StartRecvPic failed with %d\n", ret);
            goto err_exit;
        }

        /******************************************
         step  8: bind vpss channel to venc channel
        ******************************************/
        ret = FH_SYS_BindVpu2Enc(i, i);
        if (ret != RETURN_OK)
        {
            printf("Error: FH_SYS_BindVpu2Enc failed with %d\n", ret);
            goto err_exit;
        }
    }

    /******************************************
     step  9: init ISP, and then start ISP process thread
    ******************************************/
    g_stop_running = FH_FALSE;

    if (sample_isp_init() != 0)
    {
        goto err_exit;
    }
    g_thread_isp = rt_thread_create("vlc_isp", sample_isp_proc, &g_stop_running,
                                    3 * 1024, RT_APP_THREAD_PRIORITY, 10);
    rt_thread_startup(g_thread_isp);

    /******************************************
     step  10: init stream pack
    ******************************************/
#ifdef FH_USING_RTSP
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
# if FH_USING_RTSP_RTP_TCP
        g_rtsp_server[i] = rtsp_start_server(RTP_TRANSPORT_TCP, port + i * 2);
# else
        g_rtsp_server[i] = rtsp_start_server(RTP_TRANSPORT_UDP, port + i * 2);
# endif
        rtsp_play_sethook(g_rtsp_server[i], force_iframe, &g_channel_infos[i].channel);
    }
#else
    ret = vlcview_pes_init(CHANNEL_COUNT);
    if (ret != 0)
    {
        printf("Error: vlcview_pes_init failed with %d\n", ret);
        goto err_exit;
    }
    for (i = 0; i < CHANNEL_COUNT; i++)
        vlcview_pes_send_to_vlc(i, dsp_ip, port + i);
#endif

    /******************************************
     step  11: get stream, pack as PES stream and then send to vlc
    ******************************************/
    g_thread_stream =
        rt_thread_create("vlc_get_stream", sample_vlcview_get_stream_proc,
                         &g_stop_running, 3 * 1024, RT_APP_THREAD_PRIORITY, 10);
    if (g_thread_stream != RT_NULL)
    {
        rt_thread_startup(g_thread_stream);
    }

#ifdef FH_USING_COOLVIEW
    rt_thread_t thread_dbg;

    g_tcp_conf.cancel = &g_stop_running;
    g_tcp_conf.port = 8888;

    thread_dbg = rt_thread_find("coolview");
    if (thread_dbg == RT_NULL)
    {
        thread_dbg = rt_thread_create("coolview", (void *)tcp_dbi_thread, &g_tcp_conf,
                4 * 1024, RT_APP_THREAD_PRIORITY + 10, 10);
        if (thread_dbg != RT_NULL)
        {
            rt_thread_startup(thread_dbg);
        }
    }
    else
    {
        rt_thread_resume(thread_dbg);
    }
#endif

    return 0;

err_exit:
    g_stop_running = FH_TRUE;
    sample_vlcview_exit();
#ifdef FH_USING_RTSP
    for (i = 0; i < CHANNEL_COUNT; i++)
        rtsp_stop_server(g_rtsp_server[i]);
#else
    vlcview_pes_uninit();
    deinit_stream_pack();
#endif

    return -1;
}

int vlcview_exit(void)
{
    if (!g_stop_running)
    {
        g_stop_running = FH_TRUE;
        sample_vlcview_exit();
#ifdef FH_USING_RTSP
        int i;
        for (i = 0; i < CHANNEL_COUNT; i++)
            rtsp_stop_server(g_rtsp_server[i]);
#else
        vlcview_pes_uninit();
        deinit_stream_pack();
#endif
    }
    else
    {
        printf("vicview is not running!\n");
    }

    return 0;
}

#ifdef RT_USING_MN34425_MIPI
extern char isp_param_buff[FH_ISP_PARA_SIZE * 2];
extern char isp_param_buff_wdr[FH_ISP_PARA_SIZE * 2];

int load_param_buff(mode)
{

    switch(mode)
    {
    case 1:
        API_ISP_LoadIspParam(isp_param_buff);
        break;
    case 2:
        API_ISP_LoadIspParam(isp_param_buff_wdr);
        break;
    }

}

void reset_sensor()
{
    FH_SYS_SetReg(0xf0300004,0x4260);
    FH_SYS_SetReg(0xf0300000,0x0200);
    usleep(1000);
    FH_SYS_SetReg(0xf0300000,0x4200);
    printf("gpio reset sensor\n");
}

int change_mode(format)
{
    switch(format)
    {
    case FORMAT_1080P25:
        format = FORMAT_1080P25_WDR;
        load_param_buff(2); 
        break;
    case FORMAT_1080P25_WDR:
        format = FORMAT_1080P25;
        load_param_buff(1);
        break;
    }
    reset_sensor();
    API_ISP_SetSensorFmt(format);
    API_ISP_SoftReset();

}

void print_change_help()
{
    printf("Available change:\n");
    printf("   change(\"h\")    print help info\n");
    printf("   change(\"4\")    change wrd mode\n");
    printf("   change(\"18\")    change nor wdr mode\n");
}

int change(char *cmd)
{
    if (strcmp(cmd, "h") == 0)
    {
        print_change_help();
    }
    else if (strcmp(cmd, "4") == 0)
    {
        API_ISP_Pause();
        FH_VPSS_Disable();
        change_mode(4);
        FH_VPSS_Enable(0);
        API_ISP_Resume();
        return 0;
    }
    else if (strcmp(cmd, "18") == 0)
    {
        API_ISP_Pause();
        FH_VPSS_Disable();
        change_mode(18);
        FH_VPSS_Enable(0);
        API_ISP_Resume();
        return 0;
    }
}
#endif


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(vlcview, vlcview(dst_ip, port));
FINSH_FUNCTION_EXPORT(vlcview_exit, vlcview_exit());
#ifdef RT_USING_MN34425_MIPI
FINSH_FUNCTION_EXPORT(change, change(mode));
#endif
#endif
