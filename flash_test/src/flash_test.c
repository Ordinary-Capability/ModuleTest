#include <rtdef.h>
#include <rtthread.h>
#include <dfs_posix.h>
#include <mbedtls/md5.h>
#include "mtd/mtd-user.h"

/*Use mtd5 as the test flash block. Start up two threads, erase/write/read simultaneously.
 *Every thread, initial a 512k buffer with solid data pattern. First, erase the target
 *sectors(thread-1 0-128sector, thread-2 512-640sector). Second, write pattern data to
 *sectors respectively. Third, read out the data from the sectors and varify if data is 
 *correct md5 digest.
 *Author: Zhengk315
 */

#define PATTERN_LEN 0x100000

int spi_flash()
{
    rt_device_t mtd_dev;
    struct erase_info_user einfo32;
    unsigned char *data_pattern;
    unsigned char md5_digest[16] = {0}, md5_digest_result[16]={0};
    unsigned char test[4096]={0};
    rt_uint32_t i=0;

    data_pattern = rt_malloc(PATTERN_LEN);
    if(data_pattern == RT_NULL)
    {
        rt_kprintf("Malloc fail.\n");
        return -1;
        }
    for(i=0; i<PATTERN_LEN; i++)*(data_pattern+i) = i;    
    mbedtls_md5(data_pattern, PATTERN_LEN, md5_digest);
    rt_kprintf("Pattern data md5 digest:\n");
    for(i=0; i<16; i++)
        rt_kprintf("0x%x", md5_digest[i]);
    rt_kprintf("\n");

    mtd_dev = rt_device_find("mtd5");
    if(mtd_dev == RT_NULL)
    {
        rt_kprintf("Can not find device fail.\n");
        rt_free(data_pattern);
        return -1;
        }

    einfo32.start = 0;
    einfo32.length = PATTERN_LEN;
    if(RT_EOK != rt_device_control(mtd_dev, MEMERASE, &einfo32))
    {
        rt_kprintf("Erase mtd5 block data fail. Start sector %d, length 0x%x\n",
                    einfo32.start, einfo32.length);
        rt_free(data_pattern);
        return -1;
        }

    if(rt_device_open(mtd_dev, 0x003))
    {
        rt_kprintf("Open device fail.\n");
        rt_free(data_pattern);
        return -1;
        }

    if(rt_device_write(mtd_dev, 0, data_pattern, PATTERN_LEN/0x1000)<=0)
    {
        rt_kprintf("Write data to block device fail.\n");
        rt_device_close(mtd_dev);
        rt_free(data_pattern);
        return -1;
        }
    
    rt_memset(data_pattern, 0, PATTERN_LEN);

    if(rt_device_read(mtd_dev, 0, data_pattern, PATTERN_LEN/0x1000)<=0)
    {
        rt_kprintf("Read data from block device fail.\n");
        rt_device_close(mtd_dev);
        rt_free(data_pattern);
        return -1;
        }
    
    mbedtls_md5(data_pattern, PATTERN_LEN, md5_digest_result);
    if(rt_memcmp(md5_digest, md5_digest, 16) != RT_EOK)
    {
        rt_kprintf("Read block data error, md5 result %s.\n", md5_digest_result);
        rt_device_close(mtd_dev);
        rt_free(data_pattern);
        return -1;
        }

    rt_free(data_pattern);
    rt_device_close(mtd_dev);
    return 0;
    }





#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(spi_flash, "Test spi via flash read/write");
#endif
