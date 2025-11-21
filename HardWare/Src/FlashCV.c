//
// Created by vgc on 2025/11/20.
//

#include "FlashCV.h"
#include <string.h>

/********* 内部辅助：擦除某个扇区 *********/
static HAL_StatusTypeDef FlashCV_EraseSectors(uint32_t first_sector, uint32_t nb_sectors)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0;

    erase.TypeErase     = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    erase.Sector        = first_sector;
    erase.NbSectors     = nb_sectors;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    return status;
}

/********* 读取元数据 *********/
void FlashCV_ReadMeta(BootMeta_t *meta)
{
    if (meta == NULL) return;
    const BootMeta_t *p = (const BootMeta_t *)FLASH_META_ADDR;
    memcpy(meta, p, sizeof(BootMeta_t));
}

/********* 写入元数据（会先擦除Sector4） *********/
HAL_StatusTypeDef FlashCV_WriteMeta(const BootMeta_t *meta)
{
    if (meta == NULL) return HAL_ERROR;

    HAL_StatusTypeDef status;

    HAL_FLASH_Unlock();

    // 擦除元数据所在扇区：Sector 4
    status = FlashCV_EraseSectors(FLASH_SECTOR_4, 1);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    // 逐字写入
    uint32_t addr = FLASH_META_ADDR;
    const uint32_t *p = (const uint32_t *)meta;
    for (uint32_t i = 0; i < sizeof(BootMeta_t) / 4; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, p[i]);
        if (status != HAL_OK) break;
        addr += 4;
    }

    HAL_FLASH_Lock();
    return status;
}

/********* 将 flag 从 VALID 改为 DONE *********/
HAL_StatusTypeDef FlashCV_ClearMetaFlag(void)
{
    BootMeta_t meta;
    FlashCV_ReadMeta(&meta);

    if (meta.flag != UPGRADE_FLAG_VALID)
        return HAL_OK; // 本来就不是有效升级，直接返回

    meta.flag = UPGRADE_FLAG_DONE;
    return FlashCV_WriteMeta(&meta);
}

/********* 擦除 App 区：Sector 2~4 *********/
HAL_StatusTypeDef FlashCV_EraseAppArea(void)
{
    HAL_StatusTypeDef status;

    HAL_FLASH_Unlock();

    status = FlashCV_EraseSectors(FLASH_SECTOR_2, 3); // 2,3,4

    HAL_FLASH_Lock();

    return status;
}

/********* 从下载区搬运到 App 区 *********/
HAL_StatusTypeDef FlashCV_CopyImageToApp(uint32_t img_size)
{
    HAL_StatusTypeDef status;

    // 基本保护
    if (img_size == 0) return HAL_ERROR;
    if ((FLASH_DOWNLOAD_START_ADDR + img_size) > (FLASH_DOWNLOAD_END_ADDR + 1))
        return HAL_ERROR;
    if ((FLASH_APP_START_ADDR + img_size) > (FLASH_APP_END_ADDR + 1))
        return HAL_ERROR;

    // 先擦除App区
    status = FlashCV_EraseAppArea();
    if (status != HAL_OK) return status;

    HAL_FLASH_Unlock();

    uint32_t src = FLASH_DOWNLOAD_START_ADDR;
    uint32_t dst = FLASH_APP_START_ADDR;
    uint32_t remaining = img_size;
    uint32_t data;

    while (remaining > 0)
    {
        if (remaining >= 4)
        {
            data = *(uint32_t *)src;
        }
        else
        {
            uint8_t tmp[4] = {0};
            for (uint32_t i = 0; i < remaining; i++)
                tmp[i] = *(uint8_t *)(src + i);
            data = *(uint32_t *)tmp;
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dst, data);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }

        src += 4;
        dst += 4;
        if (remaining >= 4) remaining -= 4;
        else remaining = 0;
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

/********* 简单“CRC”示例：字节累加和 *********/
/* 之后你可以换成：硬件CRC 或 标准CRC32 */
uint32_t FlashCV_CalcCRC(uint32_t start_addr, uint32_t length)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        sum += *(uint8_t *)(start_addr + i);
    }
    return sum;
}
