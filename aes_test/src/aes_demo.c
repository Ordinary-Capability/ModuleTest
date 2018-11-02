#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fh_crypto_api.h"

#define DEMO_ALIGN(x) ((x + 15)&(~15)) //16bit align
#define MAX_BLOCK_LEN (1024)
#define MAX_AES_KEY_LEN (256)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))



static const unsigned char raw_key[] = {
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e, 0x0f,
    0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e, 0x1f
    };

void aes_crypto(const char data[])
{
    char raw_data[MAX_BLOCK_LEN] = {0};
    int input_len = strlen(data);
    unsigned char * pResult;
    CRYPTO_HANDLE crypto_hd;
    CRYPTO_CTRL_S crypto_ctl;

    rt_memcpy(raw_data, data, MIN(input_len, MAX_BLOCK_LEN));
    rt_kprintf("============ Aes crypto input data ====================\n");
    rt_kprintf("%s\n", raw_data);
    rt_kprintf("Length: %d\n", MIN(input_len, MAX_BLOCK_LEN));
    if(FH_CRYPTO_Init() != 0){
        rt_kprintf("Crypto init fail.\n");
        return;
        }

    if(FH_CRYPTO_CreateHandle(&crypto_hd) != 0){
        rt_kprintf("Create crypto handle fail.\n");
        return;
        }

    rt_memset(&crypto_ctl, 0, sizeof(crypto_ctl));
    rt_memcpy(crypto_ctl.u32Key, raw_key, 16);
    int i;
    rt_kprintf("Aes crypto raw key:\n");
    for(i=0; i<sizeof(crypto_ctl.u32Key)/sizeof(unsigned int); i++)
    {
        rt_kprintf("0x%x, ", crypto_ctl.u32Key[i]);
    }
    rt_kprintf("\n");
    crypto_ctl.enAlg      = CRYPTO_ALG_AES;
    //crypto_ctl.enBitWidth = CRYPTO_BIT_WIDTH_128BIT;
    //crypto_ctl.enBitWidth = CRYPTO_BIT_WIDTH_64BIT;
    crypto_ctl.enBitWidth = CRYPTO_BIT_WIDTH_8BIT;
    crypto_ctl.enWorkMode = CRYPTO_WORK_MODE_ECB;
    crypto_ctl.enKeyLen  = CRYPTO_KEY_AES_128BIT;
    crypto_ctl.enIVLen = CRYPTO_IV_KEY_AES_0BIT;
    crypto_ctl.enKeySrc = CRYPTO_KEY_SRC_USER;

    rt_kprintf("AES block size: 8bit\n.");
    
    if(FH_CRYPTO_ConfigHandle(&crypto_hd, &crypto_ctl) != 0){
        rt_kprintf("Config crypto fail.\n");
        return;
        }
    
    pResult = (unsigned char *)rt_malloc(DEMO_ALIGN(input_len));
    rt_memset(pResult, 0, DEMO_ALIGN(input_len));
    //FH_CRYPTO_Encrypt(&crypto_hd, (unsigned int)raw_data, (unsigned int)pResult, DEMO_ALIGN(input_len));
    int ret;
    ret = FH_CRYPTO_Encrypt(&crypto_hd, (unsigned int)raw_data, (unsigned int)pResult, MAX(16, input_len));
    if(ret != 0){
        rt_kprintf("Crypto encrypt fail, ret: %d\n", ret);
        free(pResult);
        return;
    }
    rt_kprintf("============ Aes crypto result ====================\n");
    for(i=0; i<DEMO_ALIGN(input_len); i++){
        rt_kprintf("0x%x ", *(pResult+i));
        }
    rt_kprintf("\n");
    rt_kprintf("============ End ====================\n");
    free(pResult);
    }



#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(aes_crypto, aes_crypto("helloworld"))
#endif





