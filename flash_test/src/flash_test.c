#include <rtdef.h>
#include <rtthread.h>
#include <dfs_posix.h>
#include "drivers/spi_fh_adapt.h"
#include "drivers/mtd_nor.h"

int fs_write(const char file_name[], const char content[])
{
    int fd, size, ret = 0;

    rt_kprintf("Write to file %s\n", file_name);

    fd = open(file_name, O_WRONLY | O_CREAT);
    if(fd<0)
    {
        rt_kprintf("Open file %s fail.\n", file_name);
        return -1;
        }

    write(fd, content, strlen(content));
    close(fd);

    return 0;
   
}

int fs_erase()
{
    /*
    struct spi_flash_device * flash_dev = RT_NULL;
    flash_dev = (struct spi_flash_device *)rt_device_find("fh_flash");

    if(flash_dev)
    (flash_dev->flash_device).control((rt_device_t)flash_dev, RT_DEVICE_CTRL_BLK_ERASE, RT_NULL);
    else
        rt_kprintf("Can not find device winbond.\n");
    */

    char buffer[8192] = {0};
    int c = 0;
    rt_device_t mtd_dev;
    struct rt_mtd_nor_device *mtd5;

    mtd_dev = rt_device_find("mtd5");
    mtd5 = (struct rt_mtd_nor_device *)mtd_dev;
    rt_kprintf("Sector start: 0x%x, end: 0x%x, size: 0x%x erase block size: 0x%x.\n",
        mtd5->sector_start, mtd5->sector_end, mtd5->sector_size, mtd5->erase_block_size);

    if(mtd_dev == RT_NULL)
    {
        rt_kprintf("Can not find device fail.\n");
        return -1;
        }
    if(rt_device_open(mtd_dev, 0x003))
    {
        rt_kprintf("Open device fail.\n");
        return -1;
        }
    /*
    if(rt_device_write(mtd_dev, 0, buffer, 1)<=0)
    {
       rt_kprintf("Write data to block device fail.\n");
       rt_device_close(mtd_dev);
       return -1;
        }
    */
    if(rt_device_read(mtd_dev, 0, buffer, 2)<=0)
    {
        rt_kprintf("Read data from block device fail.\n");
        rt_device_close(mtd_dev);
        return -1;
        }
    int i=0;
    for(i=0; i<64; i++)
        rt_kprintf("0x%x", buffer[i]);
    rt_kprintf("\n");
    for(i=4096-64; i<4096+64; i++)
        rt_kprintf("0x%x", buffer[i]);
    rt_kprintf("\n");

    rt_device_close(mtd_dev);
    return 0 ;
    }


int fs_read(const char file_name[])
{
    int fd;
    char buffer[64] = {0};

    rt_kprintf("Read from file %s\n", file_name);

    fd = open(file_name, O_WRONLY);
    if(fd < 0)
    {
        rt_kprintf("Open file %s fail.\n", file_name);
        return -1;
        }

    read(fd, buffer, 64);
    close(fd);
    rt_kprintf("Buffer: %s\n", buffer);
    return 0;
    }


void helloworld_init(void)
{
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(fs_write, fs_write("/test.txt", "helloworld"));
FINSH_FUNCTION_EXPORT(fs_read, fs_read("/test.txt"));
FINSH_FUNCTION_EXPORT(fs_erase, fs_erase());
#endif
