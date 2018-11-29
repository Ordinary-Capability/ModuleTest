#include <stdio.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include "dsp/fh_audio_mpi.h"

#define RECORD_TIME 10            // in seconds
#define RECORD_TYPE FH_AC_MIC_IN  // FH_AC_MIC_IN or FH_AC_LINE_IN
#define ONE_FRAME_SIZE 1024       // how many audio samples in one frame...

static int samples_to_bytes(int n, int sample_bits)
{
	if (sample_bits == 24)
	{
		return (n*4);
	}

	return (n * (sample_bits>>3));
}

static void print_range(void)
{
	rt_kprintf("enc_type    [0:PCM, 1:G711A, 2:G711U, 3:G726_16K, 4:G726_32K, 5:AAC]\n");
	rt_kprintf("sample_rate [8000, 16000, 32000, 48000, 11025, 22050, 44100]\n");
	rt_kprintf("bit_width   [8, 16, 32]");
	rt_kprintf("volume      [0~31]");
}
	
static int check_range(int enc_type, int sample_rate, int bit_width, int volume)
{
	if ((unsigned int)enc_type > 5)
	{
		return -1;
	}

	if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 32000 && sample_rate != 48000 &&
	    sample_rate != 11025 && sample_rate != 22050 && sample_rate != 44100 )
	{
		return -1;
	}

	if (bit_width != 8 && bit_width != 16 && bit_width != 32)
	{
		return -1;
	}

	if (enc_type != FH_PT_LPCM && bit_width != 16)
	{
		return -1;
	}

	if (volume < 0 || volume > 31)
	{
		return -1;
	}

	return 0;
}

static void audio_cap(char* filename, int enc_type, int sample_rate, int bit_width, int volume)
{
	int fd = -1;
	int ret;
	int period_bytes;
	int tick1;
	int tickdiff;
	FH_UINT32 diff;
	FH_UINT64 pts;
	FH_UINT64 pts2;

	FH_AC_CONFIG cfg;
	FH_AC_FRAME_S frame_info;

	print_range();
	if (check_range(enc_type, sample_rate, bit_width, volume) != 0)
	{
		rt_kprintf("audio_cap: invalid parameter!\n");
		return;
	}

	cfg.io_type     = RECORD_TYPE;
	cfg.sample_rate = sample_rate;
	cfg.bit_width   = bit_width;
	cfg.enc_type    = enc_type;
	cfg.channels    = 1;
	cfg.period_size = ONE_FRAME_SIZE;
	cfg.volume      = volume;

	period_bytes = samples_to_bytes(cfg.period_size, cfg.bit_width);
	frame_info.data = rt_malloc(period_bytes);
	if (!frame_info.data)
	{
		rt_kprintf("rt_malloc: failed!\n");
		return;
	}

	ret = FH_AC_Init();
	if (ret != 0)
	{
		rt_kprintf("FH_AC_Init: failed ret=%d.\n", ret);
		goto Exit;
	}

	ret = FH_AC_Set_Config(&cfg);
	if (ret != 0)
	{
		rt_kprintf("FH_AC_Set_Config: failed ret=%d.\n", ret);
		goto Exit;
	}

	FH_AC_AI_MICIN_SetVol(2);
	ret = FH_AC_AI_Enable();
	if (ret != 0)
	{
		rt_kprintf("FH_AC_AI_Enable: failed ret=%d.\n", ret);
		goto Exit;
	}

	fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0);
	if (fd < 0)
	{
		rt_kprintf("open file for failed\n");
		goto Exit2;
	}

	tick1 = rt_tick_get();
	while (1)
	{
		frame_info.len = period_bytes;
		ret = FH_AC_AI_GetFrameWithPts(&frame_info, &pts2);
		if (ret != 0)
		{
			rt_kprintf("FH_AC_AI_GetFrame: failed, ret=%d.\n", ret);
			break;
		}

		diff = pts2 - pts;
		pts = pts2;
		rt_kprintf("%d.\n", diff);

		if (fd >= 0)
		{
			if (frame_info.len != write(fd, frame_info.data, frame_info.len))
			{
				rt_kprintf("audio write failed!\n");
				goto Exit;
			}
		}
		
		tickdiff = rt_tick_get() - tick1;
		if (tickdiff >= RECORD_TIME * RT_TICK_PER_SECOND)
			break;
	}

	rt_kprintf("audio capture finished.\n");

Exit:
	if (fd >= 0)
		close(fd);
Exit2:
	FH_AC_AI_Disable();
	FH_AC_DeInit();

	if (frame_info.data)
		rt_free(frame_info.data);
}

static void audio_play(char* filename, int enc_type, int sample_rate, int bit_width, int volume)
{
	int ret;
	int fd = -1;
	int period_bytes;

	FH_AC_CONFIG  cfg;
	FH_AC_FRAME_S frame_info;

	print_range();
	if (check_range(enc_type, sample_rate, bit_width, volume) != 0)
	{
		rt_kprintf("audio_play: invalid parameter!\n");
		return;
	}

	if (enc_type == FH_PT_AAC)
	{
		rt_kprintf("AAC play is not supported now!\n");
		return;
	}

	fd = open(filename, O_RDWR, 0);
	if (fd < 0)
	{
		rt_kprintf("open file failed!\n");
		return;
	}

	cfg.io_type     = FH_AC_LINE_OUT;
	cfg.sample_rate = sample_rate;
	cfg.bit_width   = bit_width;
	cfg.enc_type    = enc_type;
	cfg.channels    = 1;
	cfg.period_size = ONE_FRAME_SIZE;
	cfg.volume      = volume;

	period_bytes = samples_to_bytes(cfg.period_size, cfg.bit_width);
	frame_info.data = rt_malloc(period_bytes);
	if (!frame_info.data)
	{
		rt_kprintf("rt_malloc: failed!\n");
		goto Exit;
	}

	if ((ret=FH_AC_Init()) != 0)
	{
		rt_kprintf("FH_AC_Init: failed, ret=%d!\n", ret);
		goto Exit;
	}

	if ((ret=FH_AC_Set_Config(&cfg)) != 0)
	{
		rt_kprintf("FH_AC_Set_Config: failed, ret=%d!\n", ret);
		goto Exit;
	}

	if ((ret=FH_AC_AO_Enable()) != 0)
	{
		rt_kprintf("FH_AC_AO_Enable: failed, ret=%d!\n", ret);
		goto Exit;
	}

	while (1)
	{
		if (period_bytes != read(fd, frame_info.data, period_bytes))
		{
			rt_kprintf("Read file finished!\n");
			break;
		}

		frame_info.len = period_bytes;
		ret = FH_AC_AO_SendFrame(&frame_info);
		if (ret != 0)
		{
			rt_kprintf("FH_AC_AO_SendFrame: failed, ret=%d!\n", ret);
			break;
		}
	}

	printf("play over!\n");
	rt_thread_delay(5*RT_TICK_PER_SECOND); //wailt playing the remained buffered audio data...


Exit:
	if (fd >= 0)
		close(fd);

	FH_AC_AO_Disable();
	FH_AC_DeInit();

	if (frame_info.data)
		rt_free(frame_info.data);
}

static void audio_loopback(int sample_rate, int volume)
{
	int ret;
	int period_bytes;
	int tick1;
	int tickdiff;

	FH_AC_CONFIG cfg;
	FH_AC_FRAME_S frame_info;

	cfg.io_type     = RECORD_TYPE;
	cfg.sample_rate = sample_rate;
	cfg.bit_width   = AC_BW_16;
	cfg.enc_type    = FH_PT_LPCM;
	cfg.channels    = 1;
	cfg.period_size = ONE_FRAME_SIZE;
	cfg.volume      = volume;

	period_bytes = samples_to_bytes(cfg.period_size, cfg.bit_width);
	frame_info.data = rt_malloc(period_bytes);
	if (!frame_info.data)
	{
		return;
	}

	if (FH_AC_Init() != 0)
		goto Exit;

	//capture config
	if (FH_AC_Set_Config(&cfg) != 0)
		goto Exit;
	
	//play config
	cfg.io_type     = FH_AC_LINE_OUT;
	cfg.volume      = 31;
	if (FH_AC_Set_Config(&cfg) != 0)
		goto Exit;

	FH_AC_AI_MICIN_SetVol(3);

	if (FH_AC_AI_Enable() != 0)
		goto Exit;

	if (FH_AC_AO_Enable() != 0)
		goto Exit;

	tick1 = rt_tick_get();
	while (1)
	{
		frame_info.len = period_bytes;
		ret = FH_AC_AI_GetFrame(&frame_info);
		if (ret != 0)
			break;
		
		FH_AC_AO_SendFrame(&frame_info);

		tickdiff = rt_tick_get() - tick1;
		if (tickdiff >= RECORD_TIME * RT_TICK_PER_SECOND)
			break;
	}

	rt_kprintf("audio loopback test finished.\n");

Exit:
	FH_AC_AI_Disable();
	FH_AC_AO_Disable();
	FH_AC_DeInit();

	if (frame_info.data)
		rt_free(frame_info.data);
}
void audio_demo_main(void *para)
{
    rt_kprintf("audio demo start:\n");
    rt_thread_delay(1000);
    audio_cap("1.dat",0,16000,16,28);
    //audio_loopback("8000",20);

    return;
}

int audio_demo_init(void)
{
    rt_thread_t threadaudio;
    threadaudio = rt_thread_create("audio",audio_demo_main,RT_NULL,10*1024,8,20);
    if (threadaudio != RT_NULL)
        rt_thread_startup(threadaudio);
    return 0;
}

void sample_audio_cap_proc(void *arg)
{
	FH_SINT32 *cancel = (FH_SINT32 *)arg;

	int enc_type = 0; 
	int sample_rate = 16000; 
	int bit_width = 16;
	int volume = 28;
	unsigned long long file_size = 0;
	unsigned long long file_max_size = 0;
	
	int fd = -1;
	int ret;
	int period_bytes;
	int tick1;
	int tickdiff;
	FH_UINT32 diff;
	FH_UINT64 pts = 0;
	FH_UINT64 pts2;

	FH_AC_CONFIG cfg;
	FH_AC_FRAME_S frame_info;

	print_range();
	if (check_range(enc_type, sample_rate, bit_width, volume) != 0)
	{
		rt_kprintf("audio_cap: invalid parameter!\n");
		return;
	}
	file_max_size = 2048*1024;
	file_max_size = file_max_size*1024;

	cfg.io_type     = RECORD_TYPE;
	cfg.sample_rate = sample_rate;
	cfg.bit_width   = bit_width;
	cfg.enc_type    = enc_type;
	cfg.channels    = 1;
	cfg.period_size = ONE_FRAME_SIZE;
	cfg.volume      = volume;

	period_bytes = samples_to_bytes(cfg.period_size, cfg.bit_width);
	frame_info.data = rt_malloc(period_bytes);
	if (!frame_info.data)
	{
		rt_kprintf("rt_malloc: failed!\n");
		return;
	}

	ret = FH_AC_Init();
	if (ret != 0)
	{
		rt_kprintf("FH_AC_Init: failed ret=%d.\n", ret);
		goto Exit;
	}

	ret = FH_AC_Set_Config(&cfg);
	if (ret != 0)
	{
		rt_kprintf("FH_AC_Set_Config: failed ret=%d.\n", ret);
		goto Exit;
	}

	FH_AC_AI_MICIN_SetVol(2);
	ret = FH_AC_AI_Enable();
	if (ret != 0)
	{
		rt_kprintf("FH_AC_AI_Enable: failed ret=%d.\n", ret);
		goto Exit;
	}

	fd = open("/mnt/audio_0.dat", O_RDWR | O_CREAT | O_TRUNC, 0);
	if (fd < 0)
	{
		rt_kprintf("open file for failed\n");
		goto Exit2;
	}

	tick1 = rt_tick_get();
	while (!*cancel)
	{
		frame_info.len = period_bytes;
		ret = FH_AC_AI_GetFrameWithPts(&frame_info, &pts2);
		if (ret != 0)
		{
			rt_kprintf("FH_AC_AI_GetFrame: failed, ret=%d.\n", ret);
			break;
		}

		diff = pts2 - pts;
		pts = pts2;

		if (fd >= 0)
		{
			if (frame_info.len != write(fd, frame_info.data, frame_info.len))
			{
				rt_kprintf("audio write failed!\n");
				goto Exit;
			}
			file_size += frame_info.len;
			if (file_size > file_max_size)
			{
				close(fd);
				fd = open("/mnt/audio_1.dat", O_RDWR | O_CREAT | O_TRUNC, 0);
				if (fd < 0)
				{
					rt_kprintf("open file for failed\n");
					goto Exit2;
				}
				file_size = 0;
			}
		}
	}

	rt_kprintf("audio capture finished.\n");

Exit:
	if (fd >= 0)
		close(fd);
Exit2:
	FH_AC_AI_Disable();
	FH_AC_DeInit();

	if (frame_info.data)
		rt_free(frame_info.data);
}

void log_simulation(void)
{
    int fd;
    char data[9]="testtest";
    while(1){
        fd = open("/log.txt", O_RDWR | O_APPEND | O_CREAT, 0);
	    if (fd < 0)
	    {
		    rt_kprintf("open file for failed\n");
        }
	    if (fd >= 0)
        {
            write(fd, data, sizeof(data));
	    }
        close(fd);
        rt_thread_delay(100);
    }
}

int log_simu_init(void)
{
    rt_thread_t threadlog;
    threadlog = rt_thread_create("log_test", log_simulation, RT_NULL, 10*1024, 8, 20);
    if (threadlog != RT_NULL)
        rt_thread_startup(threadlog);
    return 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(audio_cap, audio_cap(char* filename, int enc_type, int sample_rate, int bit_width, int volume));
FINSH_FUNCTION_EXPORT(audio_play, audio_play(char* filename, int enc_type, int sample_rate, int bit_width, int volume));
FINSH_FUNCTION_EXPORT(audio_loopback, audio_loopback(8000, 20));
FINSH_FUNCTION_EXPORT(log_simu_init, log_simu_init());
#endif
