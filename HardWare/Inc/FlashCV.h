#ifndef __FLASH_CV_H
#define __FLASH_CV_H

#include "stm32f4xx_hal.h"

/********* Flash 分区地址（可按需要微调） *********/
#define FLASH_BOOT_START_ADDR      0x08000000UL      // 扇区0~1给Bootloader
#define FLASH_BOOT_END_ADDR        0x08007FFFUL

#define FLASH_APP_START_ADDR       0x08008000UL      // 扇区2~4给App
#define FLASH_APP_END_ADDR         0x0801FFFFUL

#define FLASH_META_ADDR            0x08007F00UL      // 扇区1结尾，256字节存储元数据
#define FLASH_DOWNLOAD_START_ADDR  0x08020000UL      // 扇区5~6作为下载区
#define FLASH_DOWNLOAD_END_ADDR    0x0805FFFFUL

/********* 升级标志 *********/
#define UPGRADE_FLAG_EMPTY   0xFFFFFFFFUL
#define UPGRADE_FLAG_VALID   0xA5A5A5A5UL   // 有新固件
#define UPGRADE_FLAG_DONE    0x55AA55AAUL   // 已搬运完成

/********* 升级元数据结构体 *********/
typedef struct
{
    uint32_t flag;         // 升级标志
    uint32_t image_size;   // 新固件实际字节数
    uint32_t image_crc;    // CRC32
    uint32_t version;      // 固件版本号
    uint32_t reserved[4];  // 预留
} BootMeta_t;

/********* 对外接口 *********/

// 读取/写入 元数据
void FlashCV_ReadMeta(BootMeta_t *meta);
HAL_StatusTypeDef FlashCV_WriteMeta(const BootMeta_t *meta);
HAL_StatusTypeDef FlashCV_ClearMetaFlag(void);   // 将flag改为 DONE

// 擦除 App 区
HAL_StatusTypeDef FlashCV_EraseAppArea(void);

// 将下载区固件搬运到 App 区
HAL_StatusTypeDef FlashCV_CopyImageToApp(uint32_t img_size);

// 计算某段Flash的“CRC”，采用CRC32算法
uint32_t FlashCV_CalcCRC(uint32_t start_addr, uint32_t length);

#endif /* __FLASH_CV_H */
