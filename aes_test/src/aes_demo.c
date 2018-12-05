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
    unsigned char plan_text[256];
    unsigned char crypto_text[256];
    unsigned char iv[16];
    CRYPTO_WORK_MODE_E mode;
    CRYPTO_KEY_LENGTH_E key_len;
    rt_uint32_t plan_text_len;
    };


static crypto_run = 0;



struct aes_crypto_sample_base  g_base[] = {
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x9c,0x58,0x24,0x0, 0xb6,0xa4,0x70,0x8d, 0xaa,0xdd,0x3c,0x2d, 0xe5,0xda,0x61,0xb7},
        .mode = CRYPTO_WORK_MODE_ECB,
        .iv = {0},
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
                    0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x5f,0x53,0x1b,0xfb,0xb,0x9a,0xce,0x0,0xe,0x81,0x2e,0x26,0x72,0x66,0xbc,0x92},
        .mode = CRYPTO_WORK_MODE_ECB,
        .iv = {0},
        .key_len = CRYPTO_KEY_AES_192BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text={0x7b,0x91,0x58,0xaa,0x9d,0x96,0x64,0x95,0x22,0x69,0x5d,0xb2,0x70,0x8b,0x6c,0x9f},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CBC,
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x49,0xed,0x8b,0x6,0xc6,0xc3,0x2f,0x20,0x45,0x9d,0x15,0x81,0xe7,0x65,0x96,0x6d},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CBC,
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x33,0x8d,0x12,0x42,0x34,0x33,0x13,0xe6,0x27,0x45,0xc9,0xaf,0x77,0x56,0xc0,0xd8},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CTR,
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x33,0x8d,0x12,0x42,0x34,0x33,0x13,0xe6,0x27,0x45,0xc9,0xaf,0x77,0x56,0xc0,0xd8},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_OFB,
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
    {
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
        .plan_text = {0x68,0x65,0x6c,0x6c, 0x6f,0x77,0x6f,0x72, 0x6c,0x64,0x0,0x0, 0x0,0x0,0x0,0x0},
        .crypto_text = {0x33,0xaf,0x3f,0x52,0xa8,0xd2,0x91,0xda,0x96,0x64,0x1c,0xa2,0xc,0xc6,0x8,0xe},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CFB,
        .key_len = CRYPTO_KEY_AES_128BIT,
        .plan_text_len = 16,
        },
	{	
        .raw_key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        			0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f},
		.plan_text = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
				0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
				0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
				0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
				0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
				0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
				0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
				0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 
				0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 
				0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 
				0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 
				0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 
				0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 
				0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
				0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 
				},
		.crypto_text = {
                0x72, 0xd7, 0x5c, 0x29, 0x41, 0x59, 0xfa, 0xc2, 0x8a, 0x43, 0xf6, 0x7c, 0x6e, 0x07, 0xee, 0x4b, 
                0x9c, 0x6d, 0xcf, 0xf1, 0xfb, 0x62, 0xd2, 0x85, 0xff, 0x59, 0x86, 0x4a, 0xd9, 0xcf, 0x94, 0x11, 
                0x9e, 0x17, 0x5a, 0x14, 0xca, 0x7d, 0x64, 0xd5, 0x09, 0x79, 0xf9, 0xe0, 0x83, 0x82, 0x25, 0x7c, 
                0x65, 0x48, 0x20, 0x57, 0x13, 0xea, 0x4e, 0xaa, 0x53, 0x77, 0xa0, 0x28, 0xb7, 0xc2, 0x75, 0x37, 
                0x0c, 0x60, 0x15, 0x0a, 0xbb, 0x67, 0x82, 0x9d, 0x1b, 0x70, 0x41, 0xcd, 0x55, 0x3e, 0x8c, 0xcb, 
                0x86, 0xc2, 0x03, 0xeb, 0x88, 0xd2, 0x97, 0x81, 0x6f, 0xc4, 0xf2, 0x52, 0xb4, 0xb8, 0xf4, 0x51, 
                0x83, 0xf5, 0x07, 0xba, 0x9f, 0x96, 0x0d, 0x2b, 0xfc, 0x31, 0xd6, 0xd5, 0xf4, 0xbf, 0x8d, 0x25, 
                0x6c, 0xbd, 0xe2, 0xa2, 0xc9, 0x53, 0x53, 0x36, 0x4e, 0x02, 0x95, 0x89, 0x68, 0xa7, 0x63, 0x7a, 
                0x4a, 0x54, 0xe0, 0xbe, 0x49, 0x1b, 0x22, 0xea, 0x26, 0x24, 0x55, 0xc8, 0xa4, 0xc4, 0x4e, 0x75, 
                0xfe, 0x06, 0x50, 0x41, 0x37, 0x51, 0xe3, 0x42, 0xe8, 0x00, 0x54, 0xff, 0x0b, 0xad, 0xab, 0x9e, 
                0x22, 0x1d, 0x12, 0x72, 0xf4, 0xb9, 0x46, 0x53, 0xee, 0x9b, 0xbf, 0x1b, 0x9f, 0xd9, 0xb9, 0x63, 
                0x0e, 0x2d, 0x6f, 0x3b, 0xa2, 0x3f, 0xa2, 0xa5, 0x4b, 0x57, 0x8b, 0x74, 0x27, 0x12, 0x26, 0xd9, 
                0x11, 0xe5, 0x43, 0x0f, 0xf9, 0x67, 0xa7, 0x80, 0xe4, 0xe8, 0xe9, 0xcc, 0xec, 0x47, 0x49, 0x32, 
                0x57, 0x59, 0xe9, 0x40, 0xef, 0x69, 0xf4, 0x48, 0x06, 0xe7, 0xee, 0xc6, 0xab, 0x5d, 0x2a, 0xab, 
                0xc3, 0x0f, 0xe6, 0x5e, 0x2b, 0xb3, 0x6d, 0xf3, 0x32, 0x0e, 0x0b, 0x70, 0xb3, 0x5f, 0x0b, 0x58, 
                0x85, 0xe5, 0xa6, 0x57, 0x6e, 0x51, 0x4a, 0x98, 0x92, 0x1d, 0xf7, 0x30, 0xcf, 0xa6, 0xe7, 0x37, 
				},
        .iv = {0x20,0x21,0x22,0x23, 0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f},
        .mode = CRYPTO_WORK_MODE_CBC,
        .key_len = CRYPTO_KEY_AES_256BIT,
        .plan_text_len = 256,
		}
    };

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

    return crypto_ctl;
    }


int aes_crypto()
{
    CRYPTO_HANDLE crypto_hd;
    CRYPTO_CTRL_S *pCrypto_ctl;
    unsigned char * pResult;
    unsigned char * decrypted_data;
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
        len = (base+i)->plan_text_len;
        rt_kprintf("%s\n", (base+i)->plan_text);
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

        decrypted_data= (unsigned char *)rt_malloc(DEMO_ALIGN(len));
        rt_memset(decrypted_data, 0, DEMO_ALIGN(len));

        ret = FH_CRYPTO_Decrypt(&crypto_hd, (unsigned int)pResult, (unsigned int)decrypted_data, len);
        if(ret != 0)
        {
            rt_kprintf("Decrypt fail.\n");
            free(pResult);
            free(pCrypto_ctl);
            free(decrypted_data);
            return -1;
            }

        if(rt_memcmp(decrypted_data, (base+i)->plan_text, len))
        {
            rt_kprintf("Cypto result do not match.\n");
            free(pResult);
            free(pCrypto_ctl);
            free(decrypted_data);
            return -1;
            }

        free(pCrypto_ctl);
        free(decrypted_data);
        free(pResult);
        }
    return 0;
    }


void crypto_entry()
{
    rt_uint32_t i=0, loop = 1000;

    while(crypto_run && i<loop)
    {
        if(aes_crypto() != RT_EOK)crypto_run = 0;
        i++;
        }
    }

void crypto_stress()
{
    rt_thread_t crypto_thread1 =
        rt_thread_create("aes_crypto", crypto_entry, RT_NULL, 10*1024, 80, 20);

    rt_thread_t crypto_thread2 =
        rt_thread_create("aes_crypto", crypto_entry, RT_NULL, 10*1024, 80, 20);

    crypto_run = 1;

    if(crypto_thread1 != RT_NULL)
        rt_thread_startup(crypto_thread1);

    if(crypto_thread2 != RT_NULL)
        rt_thread_startup(crypto_thread2);
    
    
    }





#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(aes_crypto, aes_crypto())
FINSH_FUNCTION_EXPORT(crypto_stress, crypto_stress())
#endif
