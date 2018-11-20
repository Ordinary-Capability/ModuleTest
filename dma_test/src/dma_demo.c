#include <rtdef.h>
#include <rtdevice.h>
#include <time.h>
#include <mmu.h>

#include <dma.h>
#include <fh_dma.h>


#define ALIGN_DMA(x) ((x + 3)&(~3))
#define MEM_POOL_SIZE 0x400000  //4M
#define MAX_DMA_CHAN_NUM 5


extern void do_gettimeofday(struct timeval *tv);

struct rt_dma_device *gst_dma_device = RT_NULL;
struct rt_completion dma_complete;

static void *addr_src = RT_NULL;
static void *addr_dst = RT_NULL;

void dma_callback()
{
    rt_completion_done(&dma_complete);
    }

struct dma_transfer *dma_transfer_construct(unsigned char *addr_src, unsigned char *addr_dst,
                          rt_uint32_t trans_len, int channel)
{
    struct dma_transfer * pst_dma_transfer = RT_NULL;
    
    if(addr_src == RT_NULL || addr_dst == RT_NULL
        || trans_len == 0)
    {
        rt_kprintf("Invalid input para.\n");
        return RT_NULL;
        }

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
    pst_dma_transfer->channel_number = channel;
    pst_dma_transfer->src_add      = (rt_uint32_t)addr_src;
    pst_dma_transfer->dst_add      = (rt_uint32_t)addr_dst;
    pst_dma_transfer->src_inc_mode = DW_DMA_SLAVE_INC;
    pst_dma_transfer->dst_inc_mode = DW_DMA_SLAVE_INC;
    pst_dma_transfer->complete_callback = dma_callback;
    pst_dma_transfer->complete_para = RT_NULL;
    pst_dma_transfer->trans_len    = (trans_len>>2);

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


int dma_m2m_copy(int channel, rt_uint32_t data_len)
{
    struct dma_transfer * pst_dma_transfer = RT_NULL;
    struct timeval t_start, t_end;
    rt_int32_t delta = 0;
    unsigned int i=0;

    rt_kprintf("================= DMA channel %d m2m test ========\n", channel);
    pst_dma_transfer = dma_transfer_construct((unsigned char*)addr_src, 
                                                (unsigned char*)addr_dst,
                                                data_len, channel);
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
        rt_kprintf("Request dma device channel %d fail.\n", channel);
        rt_free(pst_dma_transfer);
        gst_dma_device->ops->control(gst_dma_device,
                                        RT_DEVICE_CTRL_DMA_CLOSE,
                                        pst_dma_transfer);
        return -1;
        }

    rt_completion_init(&dma_complete);
    do_gettimeofday(&t_start);
    gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                    pst_dma_transfer); 
    rt_completion_wait(&dma_complete, RT_WAITING_FOREVER);
    do_gettimeofday(&t_end);
    delta = 1000000*(t_end.tv_sec - t_start.tv_sec) + t_end.tv_usec - t_start.tv_usec;
    rt_kprintf("Mem copy size 0x%x bytes, Time consume: %d ms.\n", data_len, delta);

    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                    pst_dma_transfer))
    {
        rt_kprintf("Release dma device channel %d fail.\n", channel);
        rt_free(pst_dma_transfer);
        gst_dma_device->ops->control(gst_dma_device,
                                        RT_DEVICE_CTRL_DMA_CLOSE,
                                        pst_dma_transfer);
        return -1;
        }

    return 0;
    }




int dma_m2m()
{
    rt_uint32_t copy_size_set[] = {0x4, 0x1000, 0x100000, 0x400000};
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
            if(dma_m2m_copy(i, copy_size_set[j]) != 0)
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

    return 0;

_error:
    if(addr_src)rt_free(addr_src);
    if(addr_dst)rt_free(addr_dst);
    return -1;
    }

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(dma_m2m, dma_m2m());
#endif



