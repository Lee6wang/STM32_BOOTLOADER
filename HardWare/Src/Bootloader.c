#include "Bootloader.h"

/********* 内部函数声明 *********/
static void Bootloader_CheckAndUpgrade(void);
static void Bootloader_JumpToApp(void);

/********* 对外主入口 *********/
void Bootloader_Run(void)
{
    // 此处假设 SystemClock_Config() 和 HAL_Init() 已经在 main.c 里完成
    // 如果你想，也可以在这里再做一些指示灯/日志输出

    Bootloader_CheckAndUpgrade();
    Bootloader_JumpToApp();

    // 如果能正常跳转，这里不会执行到
    while (1)
    {
        // 错误处理：比如闪灯、等待调试
    }
}

/********* 检查元数据并执行升级 *********/
static void Bootloader_CheckAndUpgrade(void)
{
    BootMeta_t meta;
    FlashCV_ReadMeta(&meta);

    // 没有有效升级，直接 return
    if (meta.flag != UPGRADE_FLAG_VALID)
        return;

    // 基本合法性检查
    if (meta.image_size == 0 ||
        (FLASH_DOWNLOAD_START_ADDR + meta.image_size) > (FLASH_DOWNLOAD_END_ADDR + 1) ||
        (FLASH_APP_START_ADDR + meta.image_size) > (FLASH_APP_END_ADDR + 1))
    {
        // 元数据不合法，忽略这次升级
        return;
    }

    // 先对下载区做一次校验
    uint32_t crc_calc = FlashCV_CalcCRC(FLASH_DOWNLOAD_START_ADDR, meta.image_size);
    if (crc_calc != meta.image_crc)
    {
        // CRC 不匹配，视为下载失败
        return;
    }

    // 搬运固件到App区
    if (FlashCV_CopyImageToApp(meta.image_size) != HAL_OK)
    {
        // 搬运失败，保留旧App
        return;
    }

    // （可选）再对App区做一次CRC校验
    crc_calc = FlashCV_CalcCRC(FLASH_APP_START_ADDR, meta.image_size);
    if (crc_calc != meta.image_crc)
    {
        // 拷贝后验证失败，同样不清除标志，方便上位机重新下发
        return;
    }

    // 一切正常，清除升级标志，避免下次再升级
    FlashCV_ClearMetaFlag();
}

/********* 跳转到应用 *********/
typedef void (*pFunction)(void);

static void Bootloader_JumpToApp(void)
{
    uint32_t appStack = *(uint32_t *)FLASH_APP_START_ADDR;
    uint32_t appResetHandler = *(uint32_t *)(FLASH_APP_START_ADDR + 4);

    // 简单检查栈顶地址是否在 SRAM 范围
    if ((appStack & 0x2FFE0000) != 0x20000000)
    {
        // 说明应用不存在或未正确烧录
        while (1)
        {
            // TODO: 这里可以闪灯报错或等待调试
        }
    }

    __disable_irq();

    // 如有需要，这里可以关闭外设、清中断挂起等

    // 重定位中断向量表
    SCB->VTOR = FLASH_APP_START_ADDR;

    // 设置 MSP 为应用程序的栈顶
    __set_MSP(appStack);

    // 跳转到应用复位向量
    pFunction Jump = (pFunction)appResetHandler;
    Jump();
}
