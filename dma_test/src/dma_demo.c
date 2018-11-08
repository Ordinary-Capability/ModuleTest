#include <rtdef.h>
#include <rtdevice.h>
#include <mmu.h>

#include <dma.h>
#include <fh_dma.h>


#define ALIGN_DMA(x) ((x + 3)&(~3))


struct rt_dma_device *gst_dma_device = RT_NULL;
struct rt_completion dma_complete;



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
    pst_dma_transfer->trans_len    = trans_len;

    return pst_dma_transfer;
    }

/*
void data_tranafer_via_dma(struct dma_transfer * pTransfer,  unsigned char *addr_src,
                            unsigned char *addr_dst, int data_len)
{
    rt_uint32_t i = 0;

    if(addr_src == RT_NULL || addr_dst == RT_NULL
        || data_len == 0 || pTransfer == RT_NULL)
    {
        rt_kprintf("Invalid input para.\n");
        return;
        }



    }
*/
int dma_m2m(int channel)
{
    struct dma_transfer * pst_dma_transfer = RT_NULL;
    unsigned char *addr_src = RT_NULL;
    unsigned char *addr_dst = RT_NULL;
    int data_len = ALIGN_DMA(0x20000);

    addr_src = (unsigned char*)rt_malloc(data_len);
    if(addr_src == RT_NULL)
    {
        rt_kprintf("Rt malloc mem for addr_src fail.\n");
        goto Error;
        }
    addr_dst = (unsigned char*)rt_malloc(data_len);
    if(addr_dst == RT_NULL)
    {
        rt_kprintf("Rt malloc mem for addr_dst fail.\n");
        goto Error;
        }

    pst_dma_transfer = dma_transfer_construct(addr_src, addr_dst,
                                            data_len>>2, channel);
    if(pst_dma_transfer == RT_NULL)
    {
        rt_kprintf("Construct dma transfer fail.\n");
        goto Error;
        }

    gst_dma_device =  (struct rt_dma_device*)rt_device_find("fh_dma0");
    if(gst_dma_device == RT_NULL)
    {
        rt_kprintf("Can not find device: fh_dma0.\n");
        goto Error;
        }

    if(gst_dma_device->ops->init(gst_dma_device))
    {
        rt_kprintf("Rt dma device init fail.\n");
        goto Error;
        }

    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_OPEN,
                                    RT_NULL))
    {
        rt_kprintf("Open dma device fail.\n");
        goto Error;
        }
    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                    pst_dma_transfer))
    {
        rt_kprintf("Request dma device channel fail.\n");
        goto Close;
        }

   
    /*Data transfer*/
    unsigned int i=0;
    rt_kprintf("Start data transfer...\n");
    for(i=0; i<data_len; i++){addr_src[i] = i;}
    mmu_clean_dcache((rt_uint32_t)addr_src, data_len);
    rt_completion_init(&dma_complete);
    gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                    pst_dma_transfer); 
    rt_completion_wait(&dma_complete, RT_WAITING_FOREVER);
    rt_kprintf("Dst memory content:\n");
    for(i=0; i<0xff; i++)
    {
        rt_kprintf("0x%x", addr_dst[i]);
        }


    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                    pst_dma_transfer))
    {
        rt_kprintf("Release dma device channel fail.\n");
        goto Error;
        }

Close:
    if(gst_dma_device->ops->control(gst_dma_device,
                                    RT_DEVICE_CTRL_DMA_CLOSE,
                                    pst_dma_transfer))
    {
        rt_kprintf("Close dma device fail.\n");
        goto Error;
        }
Error:
    if(addr_src)rt_free(addr_src);
    if(addr_dst)rt_free(addr_dst);
    if(pst_dma_transfer)rt_free(pst_dma_transfer);
    //if(gst_dma_device)rt_free(gst_dma_device);
    return 0;


    }


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(dma_m2m, dma_m2m());
#endif



