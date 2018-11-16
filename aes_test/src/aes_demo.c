#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fh_crypto_api.h"

#define DEMO_ALIGN(x) ((x + 15)&(~15)) //16bytes align
#define MAX_BLOCK_LEN (1024)
#define MAX_AES_KEY_LEN (256)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct aes_crypto_sample_base {
    unsigned char raw_key[32];
    unsigned char plan_text[16];
    unsigned char crypto_text[16];
    unsigned char iv[16];
    CRYPTO_WORK_MODE_E mode;
    CRYPTO_KEY_LENGTH_E key_len;
    };

struct aes_crypto_sample_base  g_base[] = {
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x9c,0x58,0x24,0x0, 0xb6,0xa4,0x70,0x8d, 0xaa,0xdd,0x3c,0x2d, 0xe5,0xda,0x61,0xb7},
        .mode = CRYPTO_WORK_MODE_ECB,
        .iv = {0},
        .key_len = CRYPTO_KEY_AES_128BIT
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
                    0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x5f,0x53,0x1b,0xfb,0xb,0x9a,0xce,0x0,0xe,0x81,0x2e,0x26,0x72,0x66,0xbc,0x92},
        .mode = CRYPTO_WORK_MODE_ECB,
        .iv = {0},
        .key_len = CRYPTO_KEY_AES_192BIT
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x49,0xed,0x8b,0x6,0xc6,0xc3,0x2f,0x20,0x45,0x9d,0x15,0x81,0xe7,0x65,0x96,0x6d},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CBC,
        .key_len = CRYPTO_KEY_AES_128BIT
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0xae,0xc4,0x57,0x5b,0xe8,0xf8,0x34,0xf0,0x3,0x2b,0x81,0x62,0xa1,0xc8,0xd8,0x79},
        .iv = {0},
        .mode = CRYPTO_WORK_MODE_CTR,
        .key_len = CRYPTO_KEY_AES_128BIT
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x33,0x8d,0x12,0x42,0x34,0x33,0x13,0xe6,0x27,0x45,0xc9,0xaf,0x77,0x56,0xc0,0xd8},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_OFB,
        .key_len = CRYPTO_KEY_AES_128BIT
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x33,0xaf,0x3f,0x52,0xa8,0xd2,0x91,0xda,0x96,0x64,0x1c,0xa2,0xc,0xc6,0x8,0xe},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CFB,
        .key_len = CRYPTO_KEY_AES_128BIT
        },
    };

/*
static const unsigned char raw_key[] = {
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f
    };

static const unsigned char raw_iv[] = {
    0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33, 0x34,0x35,0x36,0x37, 0x38,0x39,0x3a,0x3b, 0x3c,0x3d,0x3e,0x3f
    };


static const unsigned char g_plantext[][16] = {
    0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0
    };

static const unsigned char g_cryptedtext[][16] = {
    0x9c,0x58,0x24,0x0, 0xb6,0xa4,0x70,0x8d, 0xaa,0xdd,0x3c,0x2d, 0xe5,0xda,0x61,0xb7
    };

*/
CRYPTO_CTRL_S * construct_crypto_ctl(CRYPTO_ALG_E crypto_alg,
                                    CRYPTO_KEY_SRC_E key_source,
                                    struct aes_crypto_sample_base *base)
{
    CRYPTO_CTRL_S *crypto_ctl;
    int i;

    crypto_ctl = (CRYPTO_CTRL_S *)rt_malloc(sizeof(CRYPTO_CTRL_S));
    rt_memset(crypto_ctl, 0 , sizeof(CRYPTO_CTRL_S));

    crypto_ctl->enAlg      = crypto_alg;
    crypto_ctl->enKeySrc = key_source;
    crypto_ctl->enWorkMode = base->mode;
    crypto_ctl->enKeyLen  = base->key_len;
    rt_memcpy(crypto_ctl->u32Key, base->raw_key, sizeof(base->raw_key));
    rt_kprintf("Aes crypto raw key:\n");
    for(i=0; i<sizeof(base->raw_key); i++)
        rt_kprintf("0x%x", base->raw_key[i]);
    rt_kprintf("\n");

    if(base->mode == CRYPTO_WORK_MODE_CBC
        || base->mode == CRYPTO_WORK_MODE_OFB
        || base->mode == CRYPTO_WORK_MODE_CTR
        || base->mode == CRYPTO_WORK_MODE_CFB)
    {
        crypto_ctl->enIVLen = CRYPTO_IV_KEY_AES_128BIT;
        rt_memcpy(crypto_ctl->u32IV, base->iv, sizeof(base->iv));
        }
    //crypto_ctl->enBitWidth = CRYPTO_BIT_WIDTH_128BIT; //useless item
    //crypto_ctl.enWorkMode = CRYPTO_WORK_MODE_CBC;
    //crypto_ctl.enWorkMode = CRYPTO_WORK_MODE_OFB;
    //crypto_ctl->enIVLen = CRYPTO_IV_KEY_AES_0BIT;
    //crypto_ctl.enIVLen = CRYPTO_IV_KEY_AES_64BIT;

    return crypto_ctl;
    }


int aes_crypto()
{
    CRYPTO_HANDLE crypto_hd;
    CRYPTO_CTRL_S *pCrypto_ctl;
    unsigned char * pResult;
    int i, j, len;
    struct aes_crypto_sample_base *base = g_base;
    int sample_count = sizeof(g_base)/sizeof(struct aes_crypto_sample_base);

    if(FH_CRYPTO_Init() != 0){
        rt_kprintf("Crypto init fail.\n");
        return -1;
        }

    if(FH_CRYPTO_CreateHandle(&crypto_hd) != 0){
        rt_kprintf("Create crypto handle fail.\n");
        return -1;
        }

    for(i=0; i<sample_count; i++)
    {
        rt_kprintf("============ Aes crypto input data %d ====================\n", i);
        len = sizeof((base+i)->plan_text);
        rt_kprintf("%s\n", base->plan_text);
        rt_kprintf("Length: %d\n", len);
        
        pCrypto_ctl = construct_crypto_ctl(CRYPTO_ALG_AES,
                                            CRYPTO_KEY_SRC_USER,
                                            base+i);
        if(FH_CRYPTO_ConfigHandle(&crypto_hd, pCrypto_ctl) != 0){
            rt_kprintf("Config crypto fail.\n");
            free(pCrypto_ctl);
            return -1;
            }
        pResult = (unsigned char *)rt_malloc(DEMO_ALIGN(len));
        rt_memset(pResult, 0, DEMO_ALIGN(len));
        int ret;
        ret = FH_CRYPTO_Encrypt(&crypto_hd,
                                (unsigned int)(base+i)->plan_text,
                                (unsigned int)pResult,
                                len);

        if(ret != 0)
        {
            rt_kprintf("Crypto encrypt fail, ret: %d\n", ret);
            free(pResult);
            free(pCrypto_ctl);
            return -1;
            }

        rt_kprintf("Crypto result:\n");
        for(j=0; j<len; j++)
            rt_kprintf("0x%x",*(pResult+j));
        rt_kprintf("\n");
        if(rt_memcmp(pResult, (base+i)->crypto_text, len))
        {
            rt_kprintf("Cypto result do not match.\n");
            free(pResult);
            free(pCrypto_ctl);
            return -1;
            }
        free(pResult);
    }
    free(pCrypto_ctl);
    return 0;
    /*
    ret = FH_CRYPTO_Decrypt(&crypto_hd, (unsigned int)pResult, (unsigned int)decrypted_data, DEMO_ALIGN(input_len));
    if(ret != 0)
    {
        rt_kprintf("Decrypt fail.\n");
        free(pResult);
        free(pCrypto_ctl);
        return;
        }
    */
    }


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(aes_crypto, aes_crypto())
#endif
