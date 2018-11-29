#include <rtdef.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <time.h>
#include <mmu.h>

#include <dma.h>
#include <fh_dma.h>


#define ALIGN_DMA(x) ((x + 3)&(~3))
#define MEM_POOL_SIZE 0x400000  //4M
#define MAX_DMA_CHAN_NUM 5


extern void do_gettimeofday(struct timeval *tv);

struct dma_control {
    rt_uint32_t channel;
    unsigned char *src;
    unsigned char *dst;
    rt_uint32_t len;
    dma_complete_callback cbk;
    struct rt_completion *complete;
    struct timeval t_start;
    };

struct rt_dma_device *gst_dma_device = RT_NULL;
struct rt_completion dma_complete0, dma_complete1, dma_complete2;

static void *addr_src = RT_NULL;
static void *addr_dst = RT_NULL;

void dma_callback0(void *para)
{
    struct timeval t_end;
    struct dma_control *ctl = (struct dma_control *)para;
    rt_uint32_t delta;

    do_gettimeofday(&t_end);
    delta = 1000000*(t_end.tv_sec - (ctl->t_start).tv_sec) + t_end.tv_usec - (ctl->t_start).tv_usec;
    rt_kprintf("Mem copy size 0x%x bytes, Time consume: %d us.\n", ctl->len, delta);
    rt_completion_done(&dma_complete0);
    }
void dma_callback1(void *para)
{
    struct timeval t_end;
    struct dma_control *ctl = (struct dma_control *)para;
    rt_uint32_t delta;

    do_gettimeofday(&t_end);
    delta = 1000000*(t_end.tv_sec - (ctl->t_start).tv_sec) + t_end.tv_usec - (ctl->t_start).tv_usec;
    rt_kprintf("Mem copy size 0x%x bytes, Time consume: %d us.\n", ctl->len, delta);
    rt_completion_done(&dma_complete1);
    }
void dma_callback2(void *para)
{
    struct timeval t_end;
    struct dma_control *ctl = (struct dma_control *)para;
    rt_uint32_t delta;

    do_gettimeofday(&t_end);
    delta = 1000000*(t_end.tv_sec - (ctl->t_start).tv_sec) + t_end.tv_usec - (ctl->t_start).tv_usec;
    rt_kprintf("Mem copy size 0x%x bytes, Time consume: %d us.\n", ctl->len, delta);
    rt_completion_done(&dma_complete2);
    }

int sem_i = 0;



struct dma_transfer *dma_transfer_construct(struct dma_control *st_dma_ctl)
{
    struct dma_transfer * pst_dma_transfer = RT_NULL;
   /* 
    if(addr_src == RT_NULL || addr_dst == RT_NULL
        || trans_len == 0)
    {
        rt_kprintf("Invalid input para.\n");
        return RT_NULL;
        }*/

    pst_dma_transfer = rt_malloc(sizeof(struct dma_transfer));
    if(pst_dma_transfer == RT_NULL)
    {
        rt_kprintf("Rt malloc mem for dam_transfer fail.\n");
        return RT_NULL;
        }
    rt_memset(pst_dma_transfer, 0, sizeof(struct dma_transfer));

    pst_dma_transfer->dma_number   = 0;
    pst_dma_transfer->fc_mode      = DMA_M2M;
    pst_dma_transfer->src_msize    = DW_DMA_SLAVE_MSIZE_8;
    pst_dma_transfer->dst_msize    = DW_DMA_SLAVE_MSIZE_8;
    pst_dma_transfer->src_width    = DW_DMA_SLAVE_WIDTH_32BIT;
    pst_dma_transfer->dst_width    = DW_DMA_SLAVE_WIDTH_32BIT;
    pst_dma_transfer->channel_number = st_dma_ctl->channel;
    pst_dma_transfer->src_add      = (rt_uint32_t)(st_dma_ctl->src);
    pst_dma_transfer->dst_add      = (rt_uint32_t)(st_dma_ctl->dst);
    pst_dma_transfer->src_inc_mode = DW_DMA_SLAVE_INC;
    pst_dma_transfer->dst_inc_mode = DW_DMA_SLAVE_INC;
    pst_dma_transfer->complete_callback = st_dma_ctl->cbk;
    pst_dma_transfer->complete_para = (void *)st_dma_ctl;
    pst_dma_transfer->trans_len    = (st_dma_ctl->len>>2);

    return pst_dma_transfer;
    }


int mem_init()
{
    addr_src = rt_malloc(MEM_POOL_SIZE);
    if(addr_src == RT_NULL)
    {
        rt_kprintf("Rt malloc mem for addr_src fail.\n");
        return -1;
        }

    rt_memset(addr_src, 0, MEM_POOL_SIZE);
    addr_dst = rt_malloc(MEM_POOL_SIZE);
    if(addr_dst == RT_NULL)
    {
        rt_kprintf("Rt malloc mem for addr_dst fail.\n");
        return -1;
        }
    
    rt_memset(addr_dst, 0, MEM_POOL_SIZE);
    return 0;
    }



void src_buffer_build(unsigned char * addr_src, rt_uint32_t len)
{
    int i=0;
    for(i=0; i<len; i++){*(addr_src+i) = i;}
    mmu_clean_dcache((rt_uint32_t)addr_src, len);
    return;
    }

int dst_buffer_varify(void *addr_src, void *addr_dts, rt_uint32_t len)
{
    mmu_invalidate_dcache((rt_uint32_t)addr_dts, len);
    if(rt_memcmp(addr_src, addr_dts, len))return -1;

    return 0;
    }


int dma_m2m_copy(struct dma_control *st_dma_ctl)
{
    struct dma_transfer * pst_dma_transfer = RT_NULL;
    struct timeval t_start, t_end;
    rt_int32_t delta = 0;
    unsigned int i=0;

    rt_kprintf("================= DMA channel %d m2m test ========\n", st_dma_ctl->channel);
    pst_dma_transfer = dma_transfer_construct(st_dma_ctl);
    if(pst_dma_transfer == RT_NULL)
    {
        rt_kprintf("Construct dma transfer fail.\n");
        return -1;
        }

    gst_dma_device =  (struct rt_dma_device*)rt_device_find("fh_dma0");
    if(gst_dma_device == RT_NULL)
    {
        rt_kprintf("Can not find device: fh_dma0.\n");
        return -1;
        }

    if(gst_dma_device->ops->init(gst_dma_device))
    {
        rt_kprintf("Rt dma device init fail.\n");
        return -1;
        }

    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_OPEN,
                                    RT_NULL))
    {
        rt_kprintf("Open dma device fail.\n");
        rt_free(pst_dma_transfer);
        gst_dma_device->ops->control(gst_dma_device,
                                        RT_DEVICE_CTRL_DMA_CLOSE,
                                        pst_dma_transfer);
        return -1;
        }

    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                    pst_dma_transfer))
    {
        rt_kprintf("Request dma device channel %d fail.\n", st_dma_ctl->channel);
        rt_free(pst_dma_transfer);
        gst_dma_device->ops->control(gst_dma_device,
                                        RT_DEVICE_CTRL_DMA_CLOSE,
                                        pst_dma_transfer);
        return -1;
        }

    rt_completion_init(st_dma_ctl->complete);
    do_gettimeofday(&(st_dma_ctl->t_start));
    gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                    pst_dma_transfer); 
    rt_completion_wait(st_dma_ctl->complete, RT_WAITING_FOREVER);
    //do_gettimeofday(&t_end);
    //delta = 1000000*(t_end.tv_sec - t_start.tv_sec) + t_end.tv_usec - t_start.tv_usec;
    //rt_kprintf("Mem copy size 0x%x bytes, Time consume: %d us.\n", st_dma_ctl->len, delta);

    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                    pst_dma_transfer))
    {
        rt_kprintf("Release dma device channel %d fail.\n", st_dma_ctl->channel);
        rt_free(pst_dma_transfer);
        gst_dma_device->ops->control(gst_dma_device,
                                        RT_DEVICE_CTRL_DMA_CLOSE,
                                        pst_dma_transfer);
        return -1;
        }

    return 0;
    }



void dma_m2m_thread_entry(void *argv)
{
    struct dma_control *st_dma_ctl=(struct dma_control *)argv;
    rt_uint32_t i, loop=1000;

    src_buffer_build(st_dma_ctl->src, st_dma_ctl->len);
    for(i=0; i<loop; i++)
    {
        //rt_kprintf("Dma channel %d loop %d\n", st_dma_ctl->channel, i);
        if(dma_m2m_copy(st_dma_ctl) != 0)
        {
            rt_kprintf("Dma m2m copy fail.\n");
            return;
            }
        if(dst_buffer_varify(st_dma_ctl->src, st_dma_ctl->dst, st_dma_ctl->len) != RT_EOK)
        {
            rt_kprintf("Dma buffer varify fail.\n");
            return;
            }

        }

    sem_i = sem_i - 1;
    return;
    }


void dma_m2m_bg(rt_uint32_t chn1, rt_uint32_t chn2)
{
    rt_thread_t dma_m2m_thread1, dma_m2m_thread2;
    struct dma_control st_dma_ctl1, st_dma_ctl2;
    unsigned char* src1, *dst1, *src2, *dst2;
    rt_uint32_t copy_size = 0x200000;

    src1 = (unsigned char*)rt_malloc(copy_size);
    if(src1 == RT_NULL)
    {
        rt_kprintf("Malloc fail.\n");
        return;
        }
    dst1 = (unsigned char*)rt_malloc(copy_size);
    if(dst1 == RT_NULL)
    {
        rt_kprintf("Malloc fail.\n");
        return;
        }
    st_dma_ctl1.channel = chn1;
    st_dma_ctl1.src = src1;
    st_dma_ctl1.dst = dst1;
    st_dma_ctl1.len = copy_size;
    st_dma_ctl1.cbk = dma_callback1;
    st_dma_ctl1.complete = &dma_complete1;
    dma_m2m_thread1 = rt_thread_create("dma_m2m_thread1", dma_m2m_thread_entry,
                                        (void *)&st_dma_ctl1, 10 * 1024, 80, 20);
    
    src2 = (unsigned char*)rt_malloc(copy_size);
    if(src2 == RT_NULL)
    {
        rt_kprintf("Malloc fail.\n");
        return;
        }
    dst2 = (unsigned char*)rt_malloc(copy_size);
    if(dst2 == RT_NULL)
    {
        rt_kprintf("Malloc fail.\n");
        return;
        }
    st_dma_ctl2.channel = chn2;
    st_dma_ctl2.src = src2;
    st_dma_ctl2.dst = dst2;
    st_dma_ctl2.len = copy_size;
    st_dma_ctl2.cbk = dma_callback2;
    st_dma_ctl2.complete = &dma_complete2;
    dma_m2m_thread2 = rt_thread_create("dma_m2m_thread2", dma_m2m_thread_entry,
                                        (void *)&st_dma_ctl2, 10 * 1024, 80, 20);
    sem_i = 1;
    if(dma_m2m_thread1 != RT_NULL)
        rt_thread_startup(dma_m2m_thread1);
    if(dma_m2m_thread2 != RT_NULL)
        rt_thread_startup(dma_m2m_thread2);
   
    while(sem_i > 0)
        rt_thread_delay(10);

    rt_free(src1);
    rt_free(src2);
    rt_free(dst1);
    rt_free(dst2);

    return;
    }


int dma_m2m()
{
    rt_uint32_t copy_size_set[] = {0x4, 0x1000, 0x100000, 0x400000};
    struct dma_control st_dma_ctl;
    int i=0, j=0, ret=0,flag=0;

    if(mem_init())
    {
        rt_kprintf("Mem poor init fail.\n");
        goto _error;
        }

    for(i=0; i<=MAX_DMA_CHAN_NUM; i++)
    {
        flag = 0;
        for(j=0; j<sizeof(copy_size_set)/4 && flag==0; j++)  
        {
            src_buffer_build(addr_src, copy_size_set[j]);
            st_dma_ctl.channel = i;
            st_dma_ctl.src = addr_src;
            st_dma_ctl.dst = addr_dst;
            st_dma_ctl.len = copy_size_set[j];
            st_dma_ctl.cbk = dma_callback0;
            st_dma_ctl.complete = &dma_complete0;
            if(dma_m2m_copy(&st_dma_ctl) != 0)
            {
                rt_kprintf("DMA channel %d test fail.\n", i);
                flag=1;
                continue;
                }
            if(dst_buffer_varify(addr_src, addr_dst, copy_size_set[j]))
            {
                rt_kprintf("Dst buffer varify fail.\n");
                goto _error;
                }
            }

        }

    if(addr_src)rt_free(addr_src);
    if(addr_dst)rt_free(addr_dst);
    return 0;

_error:
    if(addr_src)rt_free(addr_src);
    if(addr_dst)rt_free(addr_dst);
    return -1;
    }

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(dma_m2m, dma_m2m());
FINSH_FUNCTION_EXPORT(dma_m2m_bg, dma_m2m_bg(4, 5));
#endif



